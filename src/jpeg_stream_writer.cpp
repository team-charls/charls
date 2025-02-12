// SPDX-FileCopyrightText: © 2009 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include "jpeg_stream_writer.hpp"

#include "constants.hpp"
#include "jpeg_marker_code.hpp"
#include "util.hpp"

#include <array>

namespace charls {

using std::array;
using std::byte;
using std::numeric_limits;

void jpeg_stream_writer::write_start_of_image()
{
    write_segment_without_data(jpeg_marker_code::start_of_image);
}


void jpeg_stream_writer::write_end_of_image(const bool even_destination_size)
{
    if (even_destination_size && bytes_written() % 2 != 0)
    {
        // Write an additional 0xFF byte to ensure that the encoded bit stream has an even size.
        write_byte(jpeg_marker_start_byte);
    }

    write_segment_without_data(jpeg_marker_code::end_of_image);
}


void jpeg_stream_writer::write_spiff_header_segment(const spiff_header& header)
{
    ASSERT(header.height > 0);
    ASSERT(header.width > 0);

    static constexpr array spiff_magic_id{byte{'S'}, byte{'P'}, byte{'I'}, byte{'F'}, byte{'F'}, byte{'\0'}};

    // Create a JPEG APP8 segment in Still Picture Interchange File Format (SPIFF), v2.0
    write_segment_header(jpeg_marker_code::application_data8, 30);
    write_bytes(spiff_magic_id);
    write_uint8(spiff_major_revision_number);
    write_uint8(spiff_minor_revision_number);
    write_uint8(to_underlying_type(header.profile_id));
    write_uint8(header.component_count);
    write_uint32(header.height);
    write_uint32(header.width);
    write_uint8(to_underlying_type(header.color_space));
    write_uint8(header.bits_per_sample);
    write_uint8(to_underlying_type(header.compression_type));
    write_uint8(to_underlying_type(header.resolution_units));
    write_uint32(header.vertical_resolution);
    write_uint32(header.horizontal_resolution);
}


void jpeg_stream_writer::write_spiff_directory_entry(const uint32_t entry_tag, const span<const byte> entry_data)
{
    write_segment_header(jpeg_marker_code::application_data8, sizeof(uint32_t) + entry_data.size());
    write_uint32(entry_tag);
    write_bytes(entry_data);
}


void jpeg_stream_writer::write_spiff_end_of_directory_entry()
{
    // Note: ISO/IEC 10918-3, Annex F.2.2.3 documents that the EOD entry segment should have a length of 8
    // but only 6 data bytes. This approach allows to wrap existing bit streams\encoders with a SPIFF header.
    // In this implementation the SOI marker is added as data bytes to simplify the stream writer design.
    static constexpr array spiff_end_of_directory{byte{0},    byte{0},
                                                  byte{0},    byte{spiff_end_of_directory_entry_type},
                                                  byte{0xFF}, byte{to_underlying_type(jpeg_marker_code::start_of_image)}};
    write_segment(jpeg_marker_code::application_data8, spiff_end_of_directory);
}


bool jpeg_stream_writer::write_start_of_frame_segment(const frame_info& frame)
{
    ASSERT(frame.width > 0);
    ASSERT(frame.height > 0);
    ASSERT(frame.bits_per_sample >= minimum_bits_per_sample && frame.bits_per_sample <= maximum_bits_per_sample);
    ASSERT(frame.component_count > 0 && frame.component_count <= numeric_limits<uint8_t>::max());

    // Create a Frame Header as defined in ISO/IEC 14495-1, C.2.2 and T.81, B.2.2
    const size_t data_size{6 + (static_cast<size_t>(frame.component_count) * 3)};
    write_segment_header(jpeg_marker_code::start_of_frame_jpegls, data_size);
    write_uint8(frame.bits_per_sample); // P = Sample precision

    const bool oversized_image{frame.width > numeric_limits<uint16_t>::max() ||
                               frame.height > numeric_limits<uint16_t>::max()};
    write_uint16(oversized_image ? 0 : frame.height); // Y = Number of lines
    write_uint16(oversized_image ? 0 : frame.width);  // X = Number of samples per line

    // Components
    write_uint8(frame.component_count); // Nf = Number of image components in frame

    // Use by default 1 as the start component identifier to remain compatible with the
    // code sample of ISO/IEC 14495-1, H.4 and the JPEG-LS ISO conformance sample files.
    for (auto component_id{1}; component_id <= frame.component_count; ++component_id)
    {
        // Component Specification parameters
        write_uint8(component_id); // Ci = Component identifier
        write_uint8(0x11);         // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
        write_uint8(0); // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
    }

    return oversized_image;
}


void jpeg_stream_writer::write_color_transform_segment(const color_transformation transformation)
{
    const array segment{byte{'m'}, byte{'r'}, byte{'f'}, byte{'x'}, static_cast<byte>(transformation)};
    write_segment(jpeg_marker_code::application_data8, segment);
}


void jpeg_stream_writer::write_comment_segment(const span<const byte> comment)
{
    write_segment(jpeg_marker_code::comment, comment);
}


void jpeg_stream_writer::write_application_data_segment(const int32_t application_data_id,
                                                        const span<const byte> application_data)
{
    ASSERT(application_data_id >= minimum_application_data_id && application_data_id <= maximum_application_data_id);
    write_segment(
        static_cast<jpeg_marker_code>(static_cast<int32_t>(jpeg_marker_code::application_data0) + application_data_id),
        application_data);
}


void jpeg_stream_writer::write_jpegls_preset_parameters_segment(const jpegls_pc_parameters& preset_coding_parameters)
{
    write_segment_header(jpeg_marker_code::jpegls_preset_parameters, 1 + (5 * sizeof(uint16_t)));
    write_uint8(to_underlying_type(jpegls_preset_parameters_type::preset_coding_parameters));
    write_uint16(preset_coding_parameters.maximum_sample_value);
    write_uint16(preset_coding_parameters.threshold1);
    write_uint16(preset_coding_parameters.threshold2);
    write_uint16(preset_coding_parameters.threshold3);
    write_uint16(preset_coding_parameters.reset_value);
}


void jpeg_stream_writer::write_jpegls_preset_parameters_segment(const uint32_t height, const uint32_t width)
{
    // Format is defined in ISO/IEC 14495-1, C.2.4.1.4
    write_segment_header(jpeg_marker_code::jpegls_preset_parameters, size_t{1} + 1 + (2 * sizeof(uint32_t)));
    write_uint8(to_underlying_type(jpegls_preset_parameters_type::oversize_image_dimension));
    write_uint8(sizeof(uint32_t)); // Wxy: number of bytes used to represent Ye and Xe [2..4]. Always 4 for simplicity.
    write_uint32(height);          // Ye: number of lines in the image.
    write_uint32(width);           // Xe: number of columns in the image.
}


void jpeg_stream_writer::write_jpegls_preset_parameters_segment(const int32_t table_id, const int32_t entry_size,
                                                                const span<const std::byte> table_data)
{
    // Write the first 65530 bytes as mapping table specification LSE segment.
    constexpr size_t max_table_data_size{segment_max_data_size - 3};
    const byte* table_position{table_data.begin()};
    size_t table_size_to_write{std::min(table_data.size(), max_table_data_size)};
    write_jpegls_preset_parameters_segment(jpegls_preset_parameters_type::mapping_table_specification, table_id, entry_size,
                                           {table_position, table_size_to_write});

    // Write the remaining bytes as mapping table continuation LSE segments.
    table_position += table_size_to_write;
    while (table_position < table_data.end())
    {
        table_size_to_write = std::min(static_cast<size_t>(table_data.end() - table_position), max_table_data_size);
        write_jpegls_preset_parameters_segment(jpegls_preset_parameters_type::mapping_table_continuation, table_id,
                                               entry_size, {table_position, table_size_to_write});
        table_position += table_size_to_write;
    }
}


void jpeg_stream_writer::write_start_of_scan_segment(const int32_t component_count, const int32_t near_lossless,
                                                     const interleave_mode interleave_mode)
{
    ASSERT(component_count > 0 && component_count <= numeric_limits<uint8_t>::max());
    ASSERT(near_lossless >= 0 && near_lossless <= numeric_limits<uint8_t>::max());
    ASSERT(interleave_mode == interleave_mode::none || interleave_mode == interleave_mode::line ||
           interleave_mode == interleave_mode::sample);

    // Create a Scan Header as defined in T.87, C.2.3 and T.81, B.2.3
    write_segment_header(jpeg_marker_code::start_of_scan, 1 + (static_cast<size_t>(component_count) * 2) + 3);
    write_uint8(component_count);

    for (int32_t i{}; i != component_count; ++i)
    {
        write_uint8(component_index_ + 1); // follow the JPEG-LS standard samples and start with component ID 1.
        write_uint8(mapping_table_selector());
        ++component_index_;
    }

    write_uint8(near_lossless);                       // NEAR parameter
    write_uint8(to_underlying_type(interleave_mode)); // ILV parameter
    write_uint8(0);                                   // transformation
}


void jpeg_stream_writer::write_jpegls_preset_parameters_segment(const jpegls_preset_parameters_type preset_parameters_type,
                                                                const int32_t table_id, const int32_t entry_size,
                                                                const span<const std::byte> table_data)
{
    ASSERT(preset_parameters_type == jpegls_preset_parameters_type::mapping_table_specification ||
           preset_parameters_type == jpegls_preset_parameters_type::mapping_table_continuation);
    ASSERT(table_id > 0);
    ASSERT(entry_size > 0);
    ASSERT(table_data.size() >= static_cast<size_t>(entry_size)); // Need to contain at least 1 entry.
    ASSERT(table_data.size() <= segment_max_data_size - 3);

    // Format is defined in ISO/IEC 14495-1, C.2.4.1.2 and C.2.4.1.3
    write_segment_header(jpeg_marker_code::jpegls_preset_parameters, size_t{1} + 1 + 1 + table_data.size());
    write_uint8(to_underlying_type(preset_parameters_type));
    write_uint8(table_id);
    write_uint8(entry_size);
    write_bytes(table_data);
}


void jpeg_stream_writer::write_segment_header(const jpeg_marker_code marker_code, const size_t data_size)
{
    ASSERT(data_size <= segment_max_data_size);

    // Check if there is enough room in the destination to write the complete segment.
    // Other methods assume that the checking in done here and don't check again.
    constexpr size_t marker_code_size{2};
    if (const size_t total_segment_size{marker_code_size + segment_length_size + data_size};
        UNLIKELY(byte_offset_ + total_segment_size > destination_.size()))
        impl::throw_jpegls_error(jpegls_errc::destination_too_small);

    write_marker(marker_code);
    write_uint16(static_cast<uint16_t>(segment_length_size + data_size));
}

} // namespace charls
