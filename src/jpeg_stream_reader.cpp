// SPDX-FileCopyrightText: © 2009 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "jpeg_stream_reader.hpp"

#include "color_transform.hpp"
#include "constants.hpp"
#include "jpeg_marker_code.hpp"
#include "jpegls_preset_coding_parameters.hpp"
#include "jpegls_preset_parameters_type.hpp"
#include "util.hpp"

#include <algorithm>
#include <array>

namespace charls {

using impl::throw_jpegls_error;
using std::array;
using std::byte;
using std::equal;

namespace {

constexpr bool is_known_jpeg_sof_marker(const jpeg_marker_code marker_code)
{
    // The following start of frame (SOF) markers are defined in ISO/IEC 10918-1 | ITU T.81 (general JPEG standard).
    constexpr uint8_t sof_baseline_jpeg = 0xC0;            // SOF_0: Baseline jpeg encoded frame.
    constexpr uint8_t sof_extended_sequential = 0xC1;      // SOF_1: Extended sequential Huffman encoded frame.
    constexpr uint8_t sof_progressive = 0xC2;              // SOF_2: Progressive Huffman encoded frame.
    constexpr uint8_t sof_lossless = 0xC3;                 // SOF_3: Lossless Huffman encoded frame.
    constexpr uint8_t sof_differential_sequential = 0xC5;  // SOF_5: Differential sequential Huffman encoded frame.
    constexpr uint8_t sof_differential_progressive = 0xC6; // SOF_6: Differential progressive Huffman encoded frame.
    constexpr uint8_t sof_differential_lossless = 0xC7;    // SOF_7: Differential lossless Huffman encoded frame.
    constexpr uint8_t sof_extended_arithmetic = 0xC9;      // SOF_9: Extended sequential arithmetic encoded frame.
    constexpr uint8_t sof_progressive_arithmetic = 0xCA;   // SOF_10: Progressive arithmetic encoded frame.
    constexpr uint8_t sof_lossless_arithmetic = 0xCB;      // SOF_11: Lossless arithmetic encoded frame.
    constexpr uint8_t sof_jpegls_extended = 0xF9;          // SOF_57: JPEG-LS extended (ISO/IEC 14495-2) encoded frame.

    switch (to_underlying_type(marker_code))
    {
    case sof_baseline_jpeg:
    case sof_extended_sequential:
    case sof_progressive:
    case sof_lossless:
    case sof_differential_sequential:
    case sof_differential_progressive:
    case sof_differential_lossless:
    case sof_extended_arithmetic:
    case sof_progressive_arithmetic:
    case sof_lossless_arithmetic:
    case sof_jpegls_extended:
        return true;
    default:
        return false;
    }
}

[[nodiscard]]
constexpr bool is_restart_marker_code(const jpeg_marker_code marker_code) noexcept
{
    return static_cast<uint8_t>(marker_code) >= jpeg_restart_marker_base &&
           static_cast<uint8_t>(marker_code) < jpeg_restart_marker_base + jpeg_restart_marker_range;
}

[[nodiscard]]
constexpr int32_t to_application_data_id(const jpeg_marker_code marker_code) noexcept
{
    return static_cast<int32_t>(marker_code) - static_cast<int32_t>(jpeg_marker_code::application_data0);
}

} // namespace


void jpeg_stream_reader::source(const span<const byte> source) noexcept
{
    ASSERT(state_ == state::before_start_of_image);

    position_ = source.begin();
    end_position_ = source.end();
}


void jpeg_stream_reader::read_header(spiff_header* header, bool* spiff_header_found)
{
    ASSERT(state_ != state::scan_section);

    if (state_ == state::before_start_of_image)
    {
        if (UNLIKELY(read_next_marker_code() != jpeg_marker_code::start_of_image))
            throw_jpegls_error(jpegls_errc::start_of_image_marker_not_found);

        component_infos_.reserve(4); // expect 4 components or fewer.
        state_ = state::header_section;
    }

    for (;;)
    {
        const jpeg_marker_code marker_code{read_next_marker_code()};
        if (marker_code == jpeg_marker_code::end_of_image)
        {
            if (is_abbreviated_format_for_table_specification_data())
            {
                state_ = state::after_end_of_image;
                compressed_data_format_ = compressed_data_format::abbreviated_table_specification;
                return;
            }

            throw_jpegls_error(jpegls_errc::unexpected_end_of_image_marker);
        }

        validate_marker_code(marker_code);
        read_segment_size();

        switch (state_)
        {
        case state::spiff_header_section:
            read_spiff_directory_entry(marker_code);
            break;

        default:
            read_marker_segment(marker_code, header, spiff_header_found);
            break;
        }

        ASSERT(segment_data_.end() - position_ == 0); // All segment data should be processed.

        if (state_ == state::header_section && spiff_header_found && *spiff_header_found)
        {
            state_ = state::spiff_header_section;
            return;
        }

        if (state_ == state::bit_stream_section)
        {
            if (frame_info_.height == 0)
            {
                find_and_read_define_number_of_lines_segment();
            }

            check_width();
            check_coding_parameters();
            return;
        }
    }
}


void jpeg_stream_reader::read_end_of_image()
{
    ASSERT(state_ == state::bit_stream_section);

    if (const jpeg_marker_code marker_code{read_next_marker_code()}; UNLIKELY(marker_code != jpeg_marker_code::end_of_image))
        throw_jpegls_error(jpegls_errc::end_of_image_marker_not_found);

    ASSERT(compressed_data_format_ == compressed_data_format::unknown);
    compressed_data_format_ = has_external_mapping_table_ids() ? compressed_data_format::abbreviated_image_data
                                                               : compressed_data_format::interchange;

    state_ = state::after_end_of_image;
}


void jpeg_stream_reader::read_next_start_of_scan()
{
    ASSERT(state_ == state::bit_stream_section);
    state_ = state::scan_section;

    do // NOLINT(cppcoreguidelines-avoid-do-while): the loop must be executed at least once.
    {
        const jpeg_marker_code marker_code{read_next_marker_code()};
        validate_marker_code(marker_code);
        read_segment_size();
        read_marker_segment(marker_code);

        ASSERT(segment_data_.end() - position_ == 0); // All segment data should be processed.
    } while (state_ == state::scan_section);
}


jpeg_marker_code jpeg_stream_reader::read_next_marker_code()
{
    auto value{read_byte_checked()};
    if (UNLIKELY(value != jpeg_marker_start_byte))
        throw_jpegls_error(jpegls_errc::jpeg_marker_start_byte_not_found);

    // Read all preceding 0xFF fill values until a non 0xFF value has been found. (see ISO/IEC 10918-1, B.1.1.2)
    do // NOLINT(cppcoreguidelines-avoid-do-while): the loop must be executed at least once.
    {
        value = read_byte_checked();
    } while (value == jpeg_marker_start_byte);

    return static_cast<jpeg_marker_code>(value);
}


void jpeg_stream_reader::validate_marker_code(const jpeg_marker_code marker_code) const
{
    // ISO/IEC 14495-1, C.1.1. defines the following markers as valid for a JPEG-LS byte stream:
    // SOF55, LSE, SOI, EOI, SOS, DNL, DRI, RSTm, APPn and COM.
    // All other markers shall not be present.
    switch (marker_code)
    {
    case jpeg_marker_code::start_of_scan:
        if (UNLIKELY(state_ != state::scan_section))
            throw_jpegls_error(jpegls_errc::unexpected_start_of_scan_marker);

        return;

    case jpeg_marker_code::start_of_frame_jpegls:
        if (UNLIKELY(state_ == state::scan_section))
            throw_jpegls_error(jpegls_errc::duplicate_start_of_frame_marker);

        return;

    case jpeg_marker_code::define_restart_interval:
    case jpeg_marker_code::jpegls_preset_parameters:
    case jpeg_marker_code::comment:
    case jpeg_marker_code::application_data0:
    case jpeg_marker_code::application_data1:
    case jpeg_marker_code::application_data2:
    case jpeg_marker_code::application_data3:
    case jpeg_marker_code::application_data4:
    case jpeg_marker_code::application_data5:
    case jpeg_marker_code::application_data6:
    case jpeg_marker_code::application_data7:
    case jpeg_marker_code::application_data8:
    case jpeg_marker_code::application_data9:
    case jpeg_marker_code::application_data10:
    case jpeg_marker_code::application_data11:
    case jpeg_marker_code::application_data12:
    case jpeg_marker_code::application_data13:
    case jpeg_marker_code::application_data14:
    case jpeg_marker_code::application_data15:
        return;

    case jpeg_marker_code::define_number_of_lines:
        if (!dnl_marker_expected_)
            throw_jpegls_error(jpegls_errc::unexpected_define_number_of_lines_marker);

        return;

    case jpeg_marker_code::start_of_image:
        throw_jpegls_error(jpegls_errc::duplicate_start_of_image_marker);

    case jpeg_marker_code::end_of_image:
        unreachable();
    }

    // Check explicit for one of the other common JPEG encodings.
    if (is_known_jpeg_sof_marker(marker_code))
        throw_jpegls_error(jpegls_errc::encoding_not_supported);

    if (is_restart_marker_code(marker_code))
        throw_jpegls_error(jpegls_errc::unexpected_restart_marker);

    throw_jpegls_error(jpegls_errc::unknown_jpeg_marker_found);
}


jpegls_pc_parameters jpeg_stream_reader::get_validated_preset_coding_parameters() const
{
    jpegls_pc_parameters preset_coding_parameters;

    if (UNLIKELY(!is_valid(preset_coding_parameters_, calculate_maximum_sample_value(frame_info_.bits_per_sample),
                           parameters_.near_lossless, &preset_coding_parameters)))
        throw_jpegls_error(jpegls_errc::invalid_parameter_jpegls_preset_parameters);

    return preset_coding_parameters;
}


int32_t jpeg_stream_reader::get_near_lossless(const size_t component_index) const noexcept
{
    return component_infos_[component_index].near_lossless;
}


interleave_mode jpeg_stream_reader::get_interleave_mode(const size_t component_index) const noexcept
{
    return component_infos_[component_index].interleave_mode;
}


int32_t jpeg_stream_reader::get_mapping_table_id(const size_t component_index) const noexcept
{
    return component_infos_[component_index].table_id;
}


int32_t jpeg_stream_reader::find_mapping_table_index(const uint8_t mapping_table_id) const noexcept
{
    const auto it{find_mapping_table_entry(mapping_table_id)};
    return it == mapping_tables_.cend() ? mapping_table_missing : static_cast<int32_t>(it - mapping_tables_.cbegin());
}


mapping_table_info jpeg_stream_reader::get_mapping_table_info(const size_t index) const
{
    const auto& entry{mapping_tables_[index]};
    return {entry.table_id(), entry.entry_size(), static_cast<uint32_t>(entry.data_size())};
}


void jpeg_stream_reader::get_mapping_table_data(const size_t index, const span<byte> table) const
{
    const auto& mapping_table{mapping_tables_[index]};
    if (mapping_table.data_size() > table.size())
        throw_jpegls_error(jpegls_errc::destination_too_small);

    mapping_table.copy(table.data());
}


void jpeg_stream_reader::read_marker_segment(const jpeg_marker_code marker_code, spiff_header* header,
                                             bool* spiff_header_found)
{
    switch (marker_code)
    {
    case jpeg_marker_code::start_of_frame_jpegls:
        read_start_of_frame_segment();
        break;

    case jpeg_marker_code::start_of_scan:
        read_start_of_scan_segment();
        break;

    case jpeg_marker_code::jpegls_preset_parameters:
        read_preset_parameters_segment();
        break;

    case jpeg_marker_code::define_restart_interval:
        read_define_restart_interval_segment();
        break;

    case jpeg_marker_code::define_number_of_lines:
        read_define_number_of_lines_segment();
        dnl_marker_expected_ = false;
        break;

    case jpeg_marker_code::application_data8:
        try_read_application_data8_segment(header, spiff_header_found);
        break;

    case jpeg_marker_code::comment:
        read_comment_segment();
        break;

    case jpeg_marker_code::application_data0:
    case jpeg_marker_code::application_data1:
    case jpeg_marker_code::application_data2:
    case jpeg_marker_code::application_data3:
    case jpeg_marker_code::application_data4:
    case jpeg_marker_code::application_data5:
    case jpeg_marker_code::application_data6:
    case jpeg_marker_code::application_data7:
    case jpeg_marker_code::application_data9:
    case jpeg_marker_code::application_data10:
    case jpeg_marker_code::application_data11:
    case jpeg_marker_code::application_data12:
    case jpeg_marker_code::application_data13:
    case jpeg_marker_code::application_data14:
    case jpeg_marker_code::application_data15:
        read_application_data_segment(marker_code);
        break;

    default:
        unreachable();
    }
}

void jpeg_stream_reader::read_spiff_directory_entry(const jpeg_marker_code marker_code)
{
    if (UNLIKELY(marker_code != jpeg_marker_code::application_data8))
        throw_jpegls_error(jpegls_errc::missing_end_of_spiff_directory);

    check_minimal_segment_size(4);
    if (const uint32_t spiff_directory_type{read_uint32()}; spiff_directory_type == spiff_end_of_directory_entry_type)
    {
        check_segment_size(6); // 4 + 2 for dummy SOI.
        state_ = state::frame_section;
    }

    skip_remaining_segment_data();
}


void jpeg_stream_reader::read_start_of_frame_segment()
{
    // A JPEG-LS Start of Frame (SOF) segment is documented in ISO/IEC 14495-1, C.2.2
    // This section references ISO/IEC 10918-1, B.2.2, which defines the normal JPEG SOF,
    // with some modifications.
    check_minimal_segment_size(6);

    frame_info_.bits_per_sample = read_uint8();
    if (UNLIKELY(frame_info_.bits_per_sample < minimum_bits_per_sample ||
                 frame_info_.bits_per_sample > maximum_bits_per_sample))
        throw_jpegls_error(jpegls_errc::invalid_parameter_bits_per_sample);

    frame_info_height(read_uint16(), false);
    frame_info_width(read_uint16());

    frame_info_.component_count = read_uint8();
    if (UNLIKELY(frame_info_.component_count == 0))
        throw_jpegls_error(jpegls_errc::invalid_parameter_component_count);

    check_segment_size((static_cast<size_t>(frame_info_.component_count) * 3) + 6);
    for (int32_t i{}; i != frame_info_.component_count; ++i)
    {
        // Component specification parameters
        add_component(read_uint8()); // Ci = Component identifier
        if (const uint8_t horizontal_vertical_sampling_factor{
                read_uint8()}; // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
            UNLIKELY(horizontal_vertical_sampling_factor != 0x11))
            throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

        skip_byte(); // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
    }

    state_ = state::scan_section;
}


void jpeg_stream_reader::read_comment_segment()
{
    if (at_comment_callback_.handler &&
        UNLIKELY(static_cast<bool>(at_comment_callback_.handler(segment_data_.empty() ? nullptr : to_address(position_),
                                                                segment_data_.size(), at_comment_callback_.user_context))))
        throw_jpegls_error(jpegls_errc::callback_failed);

    skip_remaining_segment_data();
}


void jpeg_stream_reader::read_application_data_segment(const jpeg_marker_code marker_code)
{
    call_application_data_callback(marker_code);
    skip_remaining_segment_data();
}


uint32_t jpeg_stream_reader::read_define_number_of_lines_segment()
{
    // Note: The JPEG-LS standard supports a 2,3 or 4 byte DNL segments (see ISO/IEC 14495-1, C.2.6)
    //       The original JPEG standard only supports 2 bytes (16 bit big endian).
    switch (segment_data_.size())
    {
    case 2:
        return read_uint16();

    case 3:
        return read_uint24();

    case 4:
        return read_uint32();

    default:
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);
    }
}


void jpeg_stream_reader::read_preset_parameters_segment()
{
    check_minimal_segment_size(1);
    const byte preset_parameter{read_byte()};
    switch (static_cast<jpegls_preset_parameters_type>(preset_parameter))
    {
    case jpegls_preset_parameters_type::preset_coding_parameters:
        read_preset_coding_parameters();
        return;

    case jpegls_preset_parameters_type::mapping_table_specification:
        read_mapping_table_specification();
        return;

    case jpegls_preset_parameters_type::mapping_table_continuation:
        read_mapping_table_continuation();
        return;

    case jpegls_preset_parameters_type::oversize_image_dimension:
        read_oversize_image_dimension();
        return;
    }

    constexpr byte jpegls_extended_preset_parameter_last{0xD}; // defined in JPEG-LS Extended (ISO/IEC 14495-2) (first = 0x5)
    throw_jpegls_error(preset_parameter <= jpegls_extended_preset_parameter_last
                           ? jpegls_errc::jpegls_preset_extended_parameter_type_not_supported
                           : jpegls_errc::invalid_jpegls_preset_parameter_type);
}


void jpeg_stream_reader::read_preset_coding_parameters()
{
    check_segment_size(1 + (5 * sizeof(uint16_t)));

    // Note: validation will be done, just before decoding as more info is needed for validation.
    preset_coding_parameters_.maximum_sample_value = read_uint16();
    preset_coding_parameters_.threshold1 = read_uint16();
    preset_coding_parameters_.threshold2 = read_uint16();
    preset_coding_parameters_.threshold3 = read_uint16();
    preset_coding_parameters_.reset_value = read_uint16();
}


void jpeg_stream_reader::read_oversize_image_dimension()
{
    // Note: The JPEG-LS standard supports a 2,3 or 4 bytes for the size.
    constexpr size_t pc_and_dimension_bytes{2};
    check_minimal_segment_size(pc_and_dimension_bytes);
    const uint8_t dimension_size{read_uint8()};

    uint32_t height;
    uint32_t width;
    switch (dimension_size)
    {
    case 2:
        check_segment_size(pc_and_dimension_bytes + (sizeof(uint16_t) * 2));
        height = read_uint16();
        width = read_uint16();
        break;

    case 3:
        check_segment_size(pc_and_dimension_bytes + ((sizeof(uint16_t) + 1) * 2));
        height = read_uint24();
        width = read_uint24();
        break;

    case 4:
        check_segment_size(pc_and_dimension_bytes + (sizeof(uint32_t) * 2));
        height = read_uint32();
        width = read_uint32();
        break;

    default:
        throw_jpegls_error(jpegls_errc::invalid_parameter_jpegls_preset_parameters);
    }

    frame_info_height(height, false);
    frame_info_width(width);
}


void jpeg_stream_reader::read_mapping_table_specification()
{
    constexpr size_t pc_table_id_entry_size_bytes{3};
    check_minimal_segment_size(pc_table_id_entry_size_bytes);
    const uint8_t table_id{read_uint8()};
    const uint8_t entry_size{read_uint8()};

    add_mapping_table(table_id, entry_size, segment_data_.subspan(pc_table_id_entry_size_bytes));
    skip_remaining_segment_data();
}


void jpeg_stream_reader::read_mapping_table_continuation()
{
    constexpr size_t pc_table_id_entry_size_bytes{3};
    check_minimal_segment_size(pc_table_id_entry_size_bytes);
    const uint8_t table_id{read_uint8()};
    const uint8_t entry_size{read_uint8()};

    extend_mapping_table(table_id, entry_size, segment_data_.subspan(pc_table_id_entry_size_bytes));
    skip_remaining_segment_data();
}


void jpeg_stream_reader::read_define_restart_interval_segment()
{
    // Note: The JPEG-LS standard supports a 2,3 or 4 byte restart interval (see ISO/IEC 14495-1, C.2.5)
    //       The original JPEG standard only supports 2 bytes (16 bit big endian).
    switch (segment_data_.size())
    {
    case 2:
        parameters_.restart_interval = read_uint16();
        break;

    case 3:
        parameters_.restart_interval = read_uint24();
        break;

    case 4:
        parameters_.restart_interval = read_uint32();
        break;

    default:
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);
    }
}


void jpeg_stream_reader::read_start_of_scan_segment()
{
    check_minimal_segment_size(1);

    // ISO 10918-1, B2.3. defines the limits for the number of image components parameter in an SOS.
    const uint32_t scan_component_count{read_uint8()};
    if (UNLIKELY(scan_component_count < 1 || scan_component_count > maximum_component_count_in_scan ||
                 scan_component_count > frame_info_.component_count - read_component_count_))
        throw_jpegls_error(jpegls_errc::invalid_parameter_component_count);

    scan_component_count_ = scan_component_count;
    read_component_count_ += scan_component_count;

    array<uint8_t, maximum_component_count_in_scan> component_ids{};
    array<uint8_t, maximum_component_count_in_scan> mapping_table_ids{};

    check_segment_size((scan_component_count * size_t{2}) + 4);

    for (size_t i{}; i != scan_component_count; ++i)
    {
        component_ids[i] = read_uint8();
        mapping_table_ids[i] = read_uint8();
    }

    parameters_.near_lossless = read_uint8(); // Read NEAR parameter
    if (UNLIKELY(parameters_.near_lossless > compute_maximum_near_lossless(static_cast<int>(maximum_sample_value()))))
        throw_jpegls_error(jpegls_errc::invalid_parameter_near_lossless);

    scan_interleave_mode_ = static_cast<interleave_mode>(read_byte()); // Read ILV parameter
    check_interleave_mode(scan_interleave_mode_, scan_component_count);
    parameters_.interleave_mode = scan_interleave_mode_;

    for (size_t i{}; i != scan_component_count; ++i)
    {
        store_component_info(component_ids[i], mapping_table_ids[i], static_cast<uint8_t>(parameters_.near_lossless),
                             scan_interleave_mode_);
    }

    if (UNLIKELY((read_byte() & byte{0xFU}) != byte{})) // Read Ah (no meaning) and Al (point transform).
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    state_ = state::bit_stream_section;
}


byte jpeg_stream_reader::read_byte_checked()
{
    if (UNLIKELY(position_ == end_position_))
        throw_jpegls_error(jpegls_errc::need_more_data);

    return read_byte();
}


uint16_t jpeg_stream_reader::read_uint16_checked()
{
    if (UNLIKELY(position_ + sizeof(uint16_t) > end_position_))
        throw_jpegls_error(jpegls_errc::need_more_data);

    return read_uint16();
}


byte jpeg_stream_reader::read_byte() noexcept
{
    ASSERT(position_ != end_position_);

    const byte value{*position_};
    advance_position(1);
    return value;
}


void jpeg_stream_reader::skip_byte() noexcept
{
    advance_position(1);
}


uint16_t jpeg_stream_reader::read_uint16() noexcept
{
    ASSERT(position_ + sizeof(uint16_t) <= end_position_);

    const auto value{read_big_endian_unaligned<uint16_t>(to_address(position_))};
    advance_position(2);
    return value;
}


uint32_t jpeg_stream_reader::read_uint24() noexcept
{
    const uint32_t value{static_cast<uint32_t>(read_uint8()) << 16U};
    return value + read_uint16();
}


uint32_t jpeg_stream_reader::read_uint32() noexcept
{
    ASSERT(position_ + sizeof(uint32_t) <= end_position_);

    const auto value{read_big_endian_unaligned<uint32_t>(to_address(position_))};
    advance_position(4);
    return value;
}


span<const byte> jpeg_stream_reader::read_bytes(const size_t byte_count) noexcept
{
    ASSERT(position_ + byte_count <= end_position_);

    const span bytes{position_, byte_count};
    advance_position(byte_count);
    return bytes;
}


void jpeg_stream_reader::read_segment_size()
{
    constexpr size_t segment_length{2}; // The segment size also includes the length of the segment length bytes.
    const size_t segment_size{read_uint16_checked()};
    segment_data_ = {position_, segment_size - segment_length};

    if (UNLIKELY(segment_size < segment_length || position_ + segment_data_.size() > end_position_))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);
}


void jpeg_stream_reader::check_minimal_segment_size(const size_t minimum_size) const
{
    if (UNLIKELY(minimum_size > segment_data_.size()))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);
}


void jpeg_stream_reader::check_segment_size(const size_t expected_size) const
{
    if (UNLIKELY(expected_size != segment_data_.size()))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);
}


void jpeg_stream_reader::try_read_application_data8_segment(spiff_header* header, bool* spiff_header_found)
{
    call_application_data_callback(jpeg_marker_code::application_data8);

    if (spiff_header_found)
    {
        ASSERT(header);
        *spiff_header_found = false;
    }

    if (segment_data_.size() == 5)
    {
        try_read_hp_color_transform_segment();
    }
    else if (header && spiff_header_found && segment_data_.size() >= 30)
    {
        try_read_spiff_header_segment(*header, *spiff_header_found);
    }

    skip_remaining_segment_data();
}


void jpeg_stream_reader::try_read_hp_color_transform_segment()
{
    ASSERT(segment_data_.size() == 5);

    if (constexpr array mrfx_tag{byte{'m'}, byte{'r'}, byte{'f'}, byte{'x'}}; // mrfx = xfrm (in big endian) = colorXFoRM
        !equal(mrfx_tag.cbegin(), mrfx_tag.cend(), read_bytes(mrfx_tag.size()).begin()))
        return;

    switch (const auto transformation{read_uint8()})
    {
    case static_cast<uint8_t>(color_transformation::none):
    case static_cast<uint8_t>(color_transformation::hp1):
    case static_cast<uint8_t>(color_transformation::hp2):
    case static_cast<uint8_t>(color_transformation::hp3):
        parameters_.transformation = static_cast<color_transformation>(transformation);
        return;

    case 4: // RgbAsYuvLossy: the standard lossy RGB to YCbCr transform used in JPEG.
    case 5: // Matrix: transformation is controlled using a matrix that is also stored in the segment.
        throw_jpegls_error(jpegls_errc::color_transform_not_supported);

    default:
        throw_jpegls_error(jpegls_errc::invalid_parameter_color_transformation);
    }
}


USE_DECL_ANNOTATIONS void jpeg_stream_reader::try_read_spiff_header_segment(spiff_header& header, bool& spiff_header_found)
{
    ASSERT(segment_data_.size() >= 30);

    if (constexpr array spiff_tag{byte{'S'}, byte{'P'}, byte{'I'}, byte{'F'}, byte{'F'}, byte{0}};
        !equal(spiff_tag.cbegin(), spiff_tag.cend(), read_bytes(spiff_tag.size()).begin()))
    {
        header = {};
        spiff_header_found = false;
        return;
    }

    if (const auto high_version{read_uint8()}; high_version > spiff_major_revision_number)
    {
        header = {};
        spiff_header_found = false;
        return; // Treat unknown versions as if the SPIFF header doesn't exist.
    }
    skip_byte(); // low version

    header.profile_id = static_cast<spiff_profile_id>(read_byte());
    header.component_count = read_uint8();
    header.height = read_uint32();
    header.width = read_uint32();
    header.color_space = static_cast<spiff_color_space>(read_byte());
    header.bits_per_sample = read_uint8();
    header.compression_type = static_cast<spiff_compression_type>(read_byte());
    header.resolution_units = static_cast<spiff_resolution_units>(read_byte());
    header.vertical_resolution = read_uint32();
    header.horizontal_resolution = read_uint32();

    spiff_header_found = true;
}


void jpeg_stream_reader::add_component(const uint8_t component_id)
{
    if (UNLIKELY(find_if(component_infos_.cbegin(), component_infos_.cend(),
                         [component_id](const component_info& info) noexcept { return info.id == component_id; }) !=
                 component_infos_.cend()))
        throw_jpegls_error(jpegls_errc::duplicate_component_id_in_sof_segment);

    component_infos_.push_back({component_id, 0, 0, interleave_mode::none});
}


void jpeg_stream_reader::check_interleave_mode(const interleave_mode mode, const uint32_t scan_component_count)
{
    constexpr auto errc{jpegls_errc::invalid_parameter_interleave_mode};
    charls::check_interleave_mode(mode, errc);
    if (UNLIKELY(scan_component_count == 1U && mode != interleave_mode::none))
        throw_jpegls_error(errc);
}


uint32_t jpeg_stream_reader::maximum_sample_value() const noexcept
{
    if (preset_coding_parameters_.maximum_sample_value != 0)
        return static_cast<uint32_t>(preset_coding_parameters_.maximum_sample_value);

    return static_cast<uint32_t>(calculate_maximum_sample_value(frame_info_.bits_per_sample));
}


void jpeg_stream_reader::skip_remaining_segment_data() noexcept
{
    const auto bytes_still_to_read{static_cast<size_t>(segment_data_.end() - position_)};
    advance_position(bytes_still_to_read);
}


void jpeg_stream_reader::check_width() const
{
    if (UNLIKELY(frame_info_.width < 1))
        throw_jpegls_error(jpegls_errc::invalid_parameter_width);
}


void jpeg_stream_reader::check_coding_parameters() const
{
    if (parameters_.transformation != color_transformation::none && !color_transformation_possible(frame_info_))
        throw_jpegls_error(jpegls_errc::invalid_parameter_color_transformation);
}


void jpeg_stream_reader::frame_info_height(const uint32_t height, const bool final_update)
{
    if (height == 0 && !final_update)
        return;

    if (UNLIKELY(frame_info_.height != 0 || height == 0))
        throw_jpegls_error(jpegls_errc::invalid_parameter_height);

    frame_info_.height = height;
}


void jpeg_stream_reader::frame_info_width(const uint32_t width)
{
    if (width == 0)
        return;

    if (UNLIKELY(frame_info_.width != 0))
        throw_jpegls_error(jpegls_errc::invalid_parameter_width);

    frame_info_.width = width;
}


void jpeg_stream_reader::call_application_data_callback(const jpeg_marker_code marker_code) const
{
    if (at_application_data_callback_.handler &&
        UNLIKELY(static_cast<bool>(at_application_data_callback_.handler(
            to_application_data_id(marker_code), segment_data_.empty() ? nullptr : to_address(position_),
            segment_data_.size(), at_application_data_callback_.user_context))))
        throw_jpegls_error(jpegls_errc::callback_failed);
}


void jpeg_stream_reader::find_and_read_define_number_of_lines_segment()
{
    for (const byte* position{position_}; position < end_position_ - 1; ++position)
    {
        if (*position != jpeg_marker_start_byte)
            continue;

        const byte optional_marker_code{*(position + 1)};
        if (optional_marker_code < byte{128} || optional_marker_code == jpeg_marker_start_byte)
            continue;

        // Found a marker, ISO / IEC 10918 - 1 B .2.5 requires that if DNL is used it must be at the end of the first scan.
        if (static_cast<jpeg_marker_code>(optional_marker_code) != jpeg_marker_code::define_number_of_lines)
            break;

        const byte* current_position{position_};
        position_ = position + 2;
        read_segment_size();
        frame_info_height(read_define_number_of_lines_segment(), true);
        dnl_marker_expected_ = true;
        position_ = current_position;
        return;
    }

    throw_jpegls_error(jpegls_errc::define_number_of_lines_marker_not_found);
}


void jpeg_stream_reader::add_mapping_table(const uint8_t table_id, const uint8_t entry_size,
                                           const span<const byte> table_data)
{
    if (table_id == 0 || find_mapping_table_entry(table_id) != mapping_tables_.cend())
        throw_jpegls_error(jpegls_errc::invalid_parameter_mapping_table_id);

    mapping_tables_.emplace_back(table_id, entry_size, table_data);
}


void jpeg_stream_reader::extend_mapping_table(const uint8_t table_id, const uint8_t entry_size,
                                              const span<const byte> table_data)
{
    const auto entry{find_mapping_table_entry(table_id)};
    if (entry == mapping_tables_.cend() || entry->entry_size() != entry_size)
        throw_jpegls_error(jpegls_errc::invalid_parameter_mapping_table_continuation);

    entry->add_fragment(table_data);
}


void jpeg_stream_reader::store_component_info(const uint8_t component_id, const uint8_t table_id,
                                              const uint8_t near_lossless, const interleave_mode interleave_mode)
{
    // Ignore when info is default, prevent search and ID mismatch issues.
    if (table_id == 0 && near_lossless == 0 && interleave_mode == interleave_mode::none)
        return; // default is already 0, no need to search and update.

    const auto it{find_if(component_infos_.begin(), component_infos_.end(),
                          [component_id](const component_info& info) noexcept { return info.id == component_id; })};
    if (it == component_infos_.end())
        throw_jpegls_error(jpegls_errc::unknown_component_id);

    it->near_lossless = near_lossless;
    it->table_id = table_id;
    it->interleave_mode = interleave_mode;
}


bool jpeg_stream_reader::has_external_mapping_table_ids() const noexcept
{
    const auto it{find_if(component_infos_.cbegin(), component_infos_.cend(), [this](const component_info& info) noexcept {
        return info.table_id != 0 && find_mapping_table_entry(info.table_id) == mapping_tables_.cend();
    })};

    return it != component_infos_.cend();
}


std::vector<jpeg_stream_reader::mapping_table_entry>::const_iterator
jpeg_stream_reader::find_mapping_table_entry(uint8_t table_id) const noexcept
{
    return find_if(mapping_tables_.cbegin(), mapping_tables_.cend(),
                   [table_id](const mapping_table_entry& entry) noexcept { return entry.table_id() == table_id; });
}


std::vector<jpeg_stream_reader::mapping_table_entry>::iterator
jpeg_stream_reader::find_mapping_table_entry(uint8_t table_id) noexcept
{
    return find_if(mapping_tables_.begin(), mapping_tables_.end(),
                   [table_id](const mapping_table_entry& entry) noexcept { return entry.table_id() == table_id; });
}


} // namespace charls
