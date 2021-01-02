// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "jpeg_stream_reader.h"

#include "constants.h"
#include "decoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_marker_code.h"
#include "jpegls_preset_parameters_type.h"
#include "util.h"

#include <algorithm>
#include <memory>

namespace charls {

using impl::throw_jpegls_error;
using std::find;
using std::unique_ptr;
using std::vector;

jpeg_stream_reader::jpeg_stream_reader(const byte_span source) noexcept : source_{source}
{
}


void jpeg_stream_reader::read(byte_span source, size_t stride)
{
    ASSERT(state_ == state::bit_stream_section);

    check_parameter_coherent();

    if (rect_.Width <= 0)
    {
        rect_.Width = static_cast<int32_t>(frame_info_.width);
        rect_.Height = static_cast<int32_t>(frame_info_.height);
    }

    if (stride == 0)
    {
        const uint32_t width{rect_.Width != 0 ? static_cast<uint32_t>(rect_.Width) : frame_info_.width};
        const uint32_t component_count{
            parameters_.interleave_mode == interleave_mode::none ? 1U : static_cast<uint32_t>(frame_info_.component_count)};
        stride =
            static_cast<size_t>(component_count) * width * ((static_cast<size_t>(frame_info_.bits_per_sample) + 7U) / 8U);
    }

    const int64_t bytes_per_plane{static_cast<int64_t>(rect_.Width) * rect_.Height *
                                  bit_to_byte_count(frame_info_.bits_per_sample)};

    if (static_cast<int64_t>(source.size) < bytes_per_plane * frame_info_.component_count)
        throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

    int component_index{};
    while (component_index < frame_info_.component_count)
    {
        if (state_ == state::scan_section)
        {
            read_next_start_of_scan();
        }

        unique_ptr<decoder_strategy> codec{
            jls_codec_factory<decoder_strategy>().create_codec(frame_info_, parameters_, preset_coding_parameters_)};
        unique_ptr<process_line> process_line(codec->create_process_line(source, stride));
        codec->decode_scan(move(process_line), rect_, source_);
        skip_bytes(source, static_cast<size_t>(bytes_per_plane));
        state_ = state::scan_section;

        if (parameters_.interleave_mode != interleave_mode::none)
            return;

        component_index++;
    }
}


void jpeg_stream_reader::read_bytes(std::vector<char>& destination, const int byte_count)
{
    for (int i{}; i < byte_count; ++i)
    {
        destination.push_back(static_cast<char>(read_byte()));
    }
}


void jpeg_stream_reader::read_header(spiff_header* header, bool* spiff_header_found)
{
    ASSERT(state_ != state::scan_section);

    if (state_ == state::before_start_of_image)
    {
        if (read_next_marker_code() != jpeg_marker_code::start_of_image)
            throw_jpegls_error(jpegls_errc::start_of_image_marker_not_found);

        state_ = state::header_section;
    }

    for (;;)
    {
        const jpeg_marker_code marker_code = read_next_marker_code();
        validate_marker_code(marker_code);

        if (marker_code == jpeg_marker_code::start_of_scan)
        {
            if (!is_maximum_sample_value_valid())
                throw_jpegls_error(jpegls_errc::invalid_parameter_jpegls_pc_parameters);

            state_ = state::scan_section;
            return;
        }

        const int32_t segment_size{read_segment_size()};
        int bytes_read;
        switch (state_)
        {
        case state::spiff_header_section:
            bytes_read = read_spiff_directory_entry(marker_code, segment_size - 2) + 2;
            break;

        default:
            bytes_read = read_marker_segment(marker_code, segment_size - 2, header, spiff_header_found) + 2;
            break;
        }

        const int padding_to_read{segment_size - bytes_read};
        if (padding_to_read < 0)
            throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

        for (int i{}; i < padding_to_read; ++i)
        {
            read_byte();
        }

        if (state_ == state::header_section && spiff_header_found && *spiff_header_found)
        {
            state_ = state::spiff_header_section;
            return;
        }
    }
}


void jpeg_stream_reader::read_next_start_of_scan()
{
    ASSERT(state_ == state::scan_section);

    for (;;)
    {
        const jpeg_marker_code marker_code{read_next_marker_code()};
        validate_marker_code(marker_code);

        if (marker_code == jpeg_marker_code::start_of_scan)
        {
            read_start_of_scan();
            return;
        }

        const int32_t segment_size{read_segment_size()};
        const int bytes_read{read_marker_segment(marker_code, segment_size - 2) + 2};

        const int padding_to_read{segment_size - bytes_read};
        if (padding_to_read < 0)
            throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

        for (int i{}; i < padding_to_read; ++i)
        {
            read_byte();
        }
    }
}


jpeg_marker_code jpeg_stream_reader::read_next_marker_code()
{
    auto byte{read_byte()};
    if (byte != jpeg_marker_start_byte)
        throw_jpegls_error(jpegls_errc::jpeg_marker_start_byte_not_found);

    // Read all preceding 0xFF fill values until a non 0xFF value has been found. (see T.81, B.1.1.2)
    do
    {
        byte = read_byte();
    } while (byte == jpeg_marker_start_byte);

    return static_cast<jpeg_marker_code>(byte);
}


void jpeg_stream_reader::validate_marker_code(const jpeg_marker_code marker_code) const
{
    // ISO/IEC 14495-1, C.1.1. defines the following markers as valid for a JPEG-LS byte stream:
    // SOF55, LSE, SOI, EOI, SOS, DNL, DRI, RSTm, APPn and COM.
    // All other markers shall not be present.
    switch (marker_code)
    {
    case jpeg_marker_code::start_of_scan:
        if (state_ != state::scan_section)
            throw_jpegls_error(jpegls_errc::unexpected_marker_found);

        return;

    case jpeg_marker_code::start_of_frame_jpegls:
        if (state_ == state::scan_section)
            throw_jpegls_error(jpegls_errc::duplicate_start_of_frame_marker);

        return;

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

    // Check explicit for one of the other common JPEG encodings.
    case jpeg_marker_code::start_of_frame_baseline_jpeg:
    case jpeg_marker_code::start_of_frame_extended_sequential:
    case jpeg_marker_code::start_of_frame_progressive:
    case jpeg_marker_code::start_of_frame_lossless:
    case jpeg_marker_code::start_of_frame_differential_sequential:
    case jpeg_marker_code::start_of_frame_differential_progressive:
    case jpeg_marker_code::start_of_frame_differential_lossless:
    case jpeg_marker_code::start_of_frame_extended_arithmetic:
    case jpeg_marker_code::start_of_frame_progressive_arithmetic:
    case jpeg_marker_code::start_of_frame_lossless_arithmetic:
    case jpeg_marker_code::start_of_frame_jpegls_extended:
        throw_jpegls_error(jpegls_errc::encoding_not_supported);

    case jpeg_marker_code::start_of_image:
        throw_jpegls_error(jpegls_errc::duplicate_start_of_image_marker);

    case jpeg_marker_code::end_of_image:
        throw_jpegls_error(jpegls_errc::unexpected_end_of_image_marker);
    }

    throw_jpegls_error(jpegls_errc::unknown_jpeg_marker_found);
}


int jpeg_stream_reader::read_marker_segment(const jpeg_marker_code marker_code, const int32_t segment_size,
                                            spiff_header* header, bool* spiff_header_found)
{
    switch (marker_code)
    {
    case jpeg_marker_code::start_of_frame_jpegls:
        return read_start_of_frame_segment(segment_size);

    case jpeg_marker_code::comment:
        return read_comment();

    case jpeg_marker_code::jpegls_preset_parameters:
        return read_preset_parameters_segment(segment_size);

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
        return 0;

    case jpeg_marker_code::application_data8:
        return try_read_application_data8_segment(segment_size, header, spiff_header_found);

    // Other tags not supported (among which DNL DRI)
    default:
        ASSERT(false);
        return 0;
    }
}

int jpeg_stream_reader::read_spiff_directory_entry(const jpeg_marker_code marker_code, const int32_t segment_size)
{
    if (marker_code != jpeg_marker_code::application_data8)
        throw_jpegls_error(jpegls_errc::missing_end_of_spiff_directory);

    if (segment_size < 4)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    const uint32_t spiff_directory_type{read_uint32()};
    if (spiff_directory_type == spiff_end_of_directory_entry_type)
    {
        state_ = state::image_section;
    }

    return 4;
}

int jpeg_stream_reader::read_start_of_frame_segment(const int32_t segment_size)
{
    // A JPEG-LS Start of Frame (SOF) segment is documented in ISO/IEC 14495-1, C.2.2
    // This section references ISO/IEC 10918-1, B.2.2, which defines the normal JPEG SOF,
    // with some modifications.

    if (segment_size < 6)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    frame_info_.bits_per_sample = read_byte();
    if (frame_info_.bits_per_sample < minimum_bits_per_sample || frame_info_.bits_per_sample > maximum_bits_per_sample)
        throw_jpegls_error(jpegls_errc::invalid_parameter_bits_per_sample);

    frame_info_.height = read_uint16();
    if (frame_info_.height < 1)
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    frame_info_.width = read_uint16();
    if (frame_info_.width < 1)
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    frame_info_.component_count = read_byte();
    if (frame_info_.component_count < 1)
        throw_jpegls_error(jpegls_errc::invalid_parameter_component_count);

    if (segment_size != 6 + (frame_info_.component_count * 3))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    for (int32_t i{}; i < frame_info_.component_count; ++i)
    {
        // Component specification parameters
        add_component(read_byte()); // Ci = Component identifier
        const uint8_t horizontal_vertical_sampling_factor{
            read_byte()}; // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
        if (horizontal_vertical_sampling_factor != 0x11)
            throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

        skip_byte(); // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
    }

    state_ = state::scan_section;

    return segment_size;
}


int jpeg_stream_reader::read_comment() noexcept
{
    return 0;
}


int jpeg_stream_reader::read_preset_parameters_segment(const int32_t segment_size)
{
    if (segment_size < 1)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    const auto type{static_cast<jpegls_preset_parameters_type>(read_byte())};
    switch (type)
    {
    case jpegls_preset_parameters_type::preset_coding_parameters: {
        constexpr int32_t coding_parameter_segment_size = 11;
        if (segment_size != coding_parameter_segment_size)
            throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

        preset_coding_parameters_.maximum_sample_value = read_uint16();
        preset_coding_parameters_.threshold1 = read_uint16();
        preset_coding_parameters_.threshold2 = read_uint16();
        preset_coding_parameters_.threshold3 = read_uint16();
        preset_coding_parameters_.reset_value = read_uint16();

        return coding_parameter_segment_size;
    }

    case jpegls_preset_parameters_type::mapping_table_specification:
    case jpegls_preset_parameters_type::mapping_table_continuation:
    case jpegls_preset_parameters_type::extended_width_and_height:
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    case jpegls_preset_parameters_type::coding_method_specification:
    case jpegls_preset_parameters_type::near_lossless_error_re_specification:
    case jpegls_preset_parameters_type::visually_oriented_quantization_specification:
    case jpegls_preset_parameters_type::extended_prediction_specification:
    case jpegls_preset_parameters_type::start_of_fixed_length_coding:
    case jpegls_preset_parameters_type::end_of_fixed_length_coding:
    case jpegls_preset_parameters_type::extended_preset_coding_parameters:
    case jpegls_preset_parameters_type::inverse_color_transform_specification:
        throw_jpegls_error(jpegls_errc::jpegls_preset_extended_parameter_type_not_supported);
    }

    throw_jpegls_error(jpegls_errc::invalid_jpegls_preset_parameter_type);
}


void jpeg_stream_reader::read_start_of_scan()
{
    const int32_t segment_size{read_segment_size()};
    if (segment_size < 3)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    const int component_count_in_scan{read_byte()};
    if (component_count_in_scan != 1 && component_count_in_scan != frame_info_.component_count)
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    if (segment_size != 6 + (2 * component_count_in_scan))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    for (int i{}; i < component_count_in_scan; ++i)
    {
        read_byte(); // Read Scan component selector
        const uint8_t mapping_table_selector{read_byte()};
        if (mapping_table_selector != 0)
            throw_jpegls_error(jpegls_errc::parameter_value_not_supported);
    }

    parameters_.near_lossless = read_byte(); // Read NEAR parameter
    if (parameters_.near_lossless > compute_maximum_near_lossless(static_cast<int>(maximum_sample_value())))
        throw_jpegls_error(jpegls_errc::invalid_parameter_near_lossless);

    const auto mode{static_cast<interleave_mode>(read_byte())}; // Read ILV parameter
    if (!(mode == interleave_mode::none || mode == interleave_mode::line || mode == interleave_mode::sample))
        throw_jpegls_error(jpegls_errc::invalid_parameter_interleave_mode);
    parameters_.interleave_mode = mode;

    if ((read_byte() & 0xFU) != 0) // Read Ah (no meaning) and Al (point transform).
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    state_ = state::bit_stream_section;
}


uint8_t jpeg_stream_reader::read_byte()
{
    if (source_.size == 0)
        throw_jpegls_error(jpegls_errc::source_buffer_too_small);

    const uint8_t value{source_.data[0]};
    skip_bytes(source_, 1);
    return value;
}


void jpeg_stream_reader::skip_byte()
{
    static_cast<void>(read_byte());
}


uint16_t jpeg_stream_reader::read_uint16()
{
    const uint16_t value = read_byte() * 256U;
    return static_cast<uint16_t>(value + read_byte());
}

uint32_t jpeg_stream_reader::read_uint32()
{
    uint32_t value{read_uint16()};
    value = value << 16U;
    value += read_uint16();

    return value;
}

int32_t jpeg_stream_reader::read_segment_size()
{
    const int32_t segment_size{read_uint16()};
    if (segment_size < 2)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    return segment_size;
}

int jpeg_stream_reader::try_read_application_data8_segment(const int32_t segment_size, spiff_header* header,
                                                           bool* spiff_header_found)
{
    if (spiff_header_found)
    {
        ASSERT(header);
        *spiff_header_found = false;
    }

    if (segment_size == 5)
        return try_read_hp_color_transform_segment();

    if (header && spiff_header_found && segment_size >= 30)
        return try_read_spiff_header_segment(*header, *spiff_header_found);

    return 0;
}


int jpeg_stream_reader::try_read_hp_color_transform_segment()
{
    vector<char> source_tag;
    read_bytes(source_tag, 4);
    if (strncmp(source_tag.data(), "mrfx", 4) != 0) // mrfx = xfrm (in big endian) = colorXFoRM
        return 4;

    const auto transformation{read_byte()};
    switch (transformation)
    {
    case static_cast<uint8_t>(color_transformation::none):
    case static_cast<uint8_t>(color_transformation::hp1):
    case static_cast<uint8_t>(color_transformation::hp2):
    case static_cast<uint8_t>(color_transformation::hp3):
        parameters_.transformation = static_cast<color_transformation>(transformation);
        return 5;

    case 4: // RgbAsYuvLossy (The standard lossy RGB to YCbCr transform used in JPEG.)
    case 5: // Matrix (transformation is controlled using a matrix that is also stored in the segment.
        throw_jpegls_error(jpegls_errc::color_transform_not_supported);

    default:
        throw_jpegls_error(jpegls_errc::invalid_encoded_data);
    }
}

template<typename EnumType, EnumType Low, EnumType High>
EnumType enum_cast(uint8_t value)
{
    if (value < static_cast<uint8_t>(Low))
        throw_jpegls_error(jpegls_errc::invalid_encoded_data);

    if (value > static_cast<uint8_t>(High))
        throw_jpegls_error(jpegls_errc::invalid_encoded_data);

    return static_cast<EnumType>(value);
}

int jpeg_stream_reader::try_read_spiff_header_segment(OUT_ spiff_header& header, OUT_ bool& spiff_header_found)
{
    vector<char> source_tag;
    read_bytes(source_tag, 6);
    if (strncmp(source_tag.data(), "SPIFF\0", 6) != 0)
    {
        header = {};
        spiff_header_found = false;
        return 6;
    }

    const auto high_version{read_byte()};
    if (high_version > spiff_major_revision_number)
    {
        header = {};
        spiff_header_found = false;
        return 7; // Treat unknown versions as if the SPIFF header doesn't exists.
    }

    skip_byte(); // low version

    header.profile_id = static_cast<spiff_profile_id>(read_byte());
    header.component_count = read_byte();
    header.height = read_uint32();
    header.width = read_uint32();
    header.color_space = static_cast<spiff_color_space>(read_byte());
    header.bits_per_sample = read_byte();
    header.compression_type = static_cast<spiff_compression_type>(read_byte());
    header.resolution_units = static_cast<spiff_resolution_units>(read_byte());
    header.vertical_resolution = read_uint32();
    header.horizontal_resolution = read_uint32();

    spiff_header_found = true;
    return 30;
}


void jpeg_stream_reader::add_component(const uint8_t component_id)
{
    if (find(component_ids_.cbegin(), component_ids_.cend(), component_id) != component_ids_.cend())
        throw_jpegls_error(jpegls_errc::duplicate_component_id_in_sof_segment);

    component_ids_.push_back(component_id);
}


void jpeg_stream_reader::check_parameter_coherent() const
{
    switch (frame_info_.component_count)
    {
    case 4:
    case 3:
        break;
    default:
        if (parameters_.interleave_mode != interleave_mode::none)
            throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

        break;
    }
}


bool jpeg_stream_reader::is_maximum_sample_value_valid() const noexcept
{
    return preset_coding_parameters_.maximum_sample_value == 0 ||
           static_cast<uint32_t>(preset_coding_parameters_.maximum_sample_value) <=
               calculate_maximum_sample_value(frame_info_.bits_per_sample);
}


uint32_t jpeg_stream_reader::maximum_sample_value() const noexcept
{
    ASSERT(is_maximum_sample_value_valid());

    if (preset_coding_parameters_.maximum_sample_value != 0)
        return static_cast<uint32_t>(preset_coding_parameters_.maximum_sample_value);

    return calculate_maximum_sample_value(frame_info_.bits_per_sample);
}


} // namespace charls
