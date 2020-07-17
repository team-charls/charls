// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "jpeg_stream_writer.h"

#include "constants.h"
#include "jpeg_marker_code.h"
#include "jpeg_stream_reader.h"
#include "jpegls_preset_parameters_type.h"
#include "util.h"

#include <array>
#include <cassert>
#include <vector>

using std::array;
using std::vector;

namespace charls {

jpeg_stream_writer::jpeg_stream_writer(const byte_stream_info& destination) noexcept :
    destination_{destination}
{
}


void jpeg_stream_writer::write_start_of_image()
{
    write_marker(jpeg_marker_code::start_of_image);
}


void jpeg_stream_writer::write_end_of_image()
{
    write_marker(jpeg_marker_code::end_of_image);
}


void jpeg_stream_writer::write_spiff_header_segment(const spiff_header& header)
{
    ASSERT(header.height > 0);
    ASSERT(header.width > 0);

    // Create a JPEG APP8 segment in Still Picture Interchange File Format (SPIFF), v2.0
    vector<uint8_t> segment{'S', 'P', 'I', 'F', 'F', '\0'};
    segment.push_back(spiff_major_revision_number);
    segment.push_back(spiff_minor_revision_number);
    segment.push_back(static_cast<uint8_t>(header.profile_id));
    segment.push_back(static_cast<uint8_t>(header.component_count));
    push_back(segment, header.height);
    push_back(segment, header.width);
    segment.push_back(static_cast<uint8_t>(header.color_space));
    segment.push_back(static_cast<uint8_t>(header.bits_per_sample));
    segment.push_back(static_cast<uint8_t>(header.compression_type));
    segment.push_back(static_cast<uint8_t>(header.resolution_units));
    push_back(segment, header.vertical_resolution);
    push_back(segment, header.horizontal_resolution);

    write_segment(jpeg_marker_code::application_data8, segment.data(), segment.size());
}


void jpeg_stream_writer::write_spiff_directory_entry(const uint32_t entry_tag,
                                                     IN_READS_BYTES_(entry_data_size_bytes) const void* entry_data,
                                                     const size_t entry_data_size_bytes)
{
    write_marker(jpeg_marker_code::application_data8);
    write_uint16(static_cast<uint16_t>(sizeof(uint16_t) + sizeof(uint32_t) + entry_data_size_bytes));
    write_uint32(entry_tag);
    write_bytes(entry_data, entry_data_size_bytes);
}


void jpeg_stream_writer::write_spiff_end_of_directory_entry()
{
    // Note: ISO/IEC 10918-3, Annex F.2.2.3 documents that the EOD entry segment should have a length of 8
    // but only 6 data bytes. This approach allows to wrap existing bit streams\encoders with a SPIFF header.
    // In this implementation the SOI marker is added as data bytes to simplify the design.

    array<uint8_t, 6> segment{0, 0, 0, spiff_end_of_directory_entry_type, 0xFF, static_cast<uint8_t>(jpeg_marker_code::start_of_image)};
    write_segment(jpeg_marker_code::application_data8, segment.data(), segment.size());
}


void jpeg_stream_writer::write_start_of_frame_segment(const uint32_t width, const uint32_t height, const int bits_per_sample, const int component_count)
{
    ASSERT(width <= UINT16_MAX);
    ASSERT(height <= UINT16_MAX);
    ASSERT(bits_per_sample >= minimum_bits_per_sample && bits_per_sample <= maximum_bits_per_sample);
    ASSERT(component_count > 0 && component_count <= UINT8_MAX);

    // Create a Frame Header as defined in ISO/IEC 14495-1, C.2.2 and T.81, B.2.2
    vector<uint8_t> segment;
    segment.push_back(static_cast<uint8_t>(bits_per_sample)); // P = Sample precision
    push_back(segment, static_cast<uint16_t>(height));      // Y = Number of lines
    push_back(segment, static_cast<uint16_t>(width));       // X = Number of samples per line

    // Components
    segment.push_back(static_cast<uint8_t>(component_count)); // Nf = Number of image components in frame

    // Use by default 1 as the start component identifier to remain compatible with the
    // code sample of ISO/IEC 14495-1, H.4 and the JPEG-LS ISO conformance sample files.
    for (auto component_id = 1; component_id <= component_count; ++component_id)
    {
        // Component Specification parameters
        segment.push_back(static_cast<uint8_t>(component_id)); // Ci = Component identifier
        segment.push_back(0x11);                              // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
        segment.push_back(0);                                 // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
    }

    write_segment(jpeg_marker_code::start_of_frame_jpegls, segment.data(), segment.size());
}


void jpeg_stream_writer::write_color_transform_segment(const color_transformation transformation)
{
    array<uint8_t, 5> segment{'m', 'r', 'f', 'x', static_cast<uint8_t>(transformation)};
    write_segment(jpeg_marker_code::application_data8, segment.data(), segment.size());
}


void jpeg_stream_writer::write_jpegls_preset_parameters_segment(const jpegls_pc_parameters& preset_coding_parameters)
{
    vector<uint8_t> segment;

    segment.push_back(static_cast<uint8_t>(jpegls_preset_parameters_type::preset_coding_parameters));

    push_back(segment, static_cast<uint16_t>(preset_coding_parameters.maximum_sample_value));
    push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold1));
    push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold2));
    push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold3));
    push_back(segment, static_cast<uint16_t>(preset_coding_parameters.reset_value));

    write_segment(jpeg_marker_code::jpegls_preset_parameters, segment.data(), segment.size());
}


void jpeg_stream_writer::write_start_of_scan_segment(const int component_count,
                                                     const int near_lossless,
                                                     const interleave_mode interleave_mode)
{
    ASSERT(component_count > 0 && component_count <= UINT8_MAX);
    ASSERT(near_lossless >= 0 && near_lossless <= UINT8_MAX);
    ASSERT(interleave_mode == interleave_mode::none ||
           interleave_mode == interleave_mode::line ||
           interleave_mode == interleave_mode::sample);

    // Create a Scan Header as defined in T.87, C.2.3 and T.81, B.2.3
    vector<uint8_t> segment;

    segment.push_back(static_cast<uint8_t>(component_count));
    for (auto i = 0; i < component_count; ++i)
    {
        segment.push_back(static_cast<uint8_t>(component_id_));
        ++component_id_;
        segment.push_back(0); // Mapping table selector (0 = no table)
    }

    segment.push_back(static_cast<uint8_t>(near_lossless));   // NEAR parameter
    segment.push_back(static_cast<uint8_t>(interleave_mode)); // ILV parameter
    segment.push_back(0);                                     // transformation

    write_segment(jpeg_marker_code::start_of_scan, segment.data(), segment.size());
}


void jpeg_stream_writer::write_segment(const jpeg_marker_code marker_code,
                                       IN_READS_BYTES_(size) const void* data,
                                       const size_t size)
{
    ASSERT(size <= UINT16_MAX - sizeof(uint16_t));

    write_marker(marker_code);
    write_uint16(static_cast<uint16_t>(size + sizeof(uint16_t)));
    write_bytes(data, size);
}

} // namespace charls
