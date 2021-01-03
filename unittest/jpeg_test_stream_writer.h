// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "../src/jpeg_marker_code.h"
#include "../src/jpegls_preset_parameters_type.h"
#include "../src/util.h"

namespace charls { namespace test {

class jpeg_test_stream_writer final
{
public:
    void write_start_of_image()
    {
        write_marker(jpeg_marker_code::start_of_image);
    }

    void write_start_of_frame_segment(const int width, const int height, const int bits_per_sample,
                                      const int component_count)
    {
        // Create a Frame Header as defined in T.87, C.2.2 and T.81, B.2.2
        std::vector<uint8_t> segment;
        segment.push_back(static_cast<uint8_t>(bits_per_sample)); // P = Sample precision
        push_back(segment, static_cast<uint16_t>(height));        // Y = Number of lines
        push_back(segment, static_cast<uint16_t>(width));         // X = Number of samples per line

        // Components
        segment.push_back(static_cast<uint8_t>(component_count)); // Nf = Number of image components in frame
        for (int component_id{}; component_id < component_count; ++component_id)
        {
            // Component Specification parameters
            if (componentIdOverride == 0)
            {
                segment.push_back(static_cast<uint8_t>(component_id)); // Ci = Component identifier
            }
            else
            {
                segment.push_back(static_cast<uint8_t>(componentIdOverride)); // Ci = Component identifier
            }
            segment.push_back(0x11); // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
            segment.push_back(0); // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
        }

        write_segment(jpeg_marker_code::start_of_frame_jpegls, segment.data(), segment.size());
    }

    void write_jpegls_preset_parameters_segment(const jpegls_pc_parameters& preset_coding_parameters)
    {
        std::vector<uint8_t> segment;

        segment.push_back(static_cast<uint8_t>(jpegls_preset_parameters_type::preset_coding_parameters));

        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.maximum_sample_value));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold1));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold2));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold3));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.reset_value));

        write_segment(jpeg_marker_code::jpegls_preset_parameters, segment.data(), segment.size());
    }

    void write_start_of_scan_segment(int component_id, const int component_count, const int near_lossless,
                                     const interleave_mode interleave_mode)
    {
        // Create a Scan Header as defined in T.87, C.2.3 and T.81, B.2.3
        std::vector<uint8_t> segment;

        segment.push_back(static_cast<uint8_t>(component_count));
        for (int i{}; i < component_count; ++i)
        {
            segment.push_back(static_cast<uint8_t>(component_id));
            ++component_id;
            segment.push_back(mapping_table_selector); // Mapping table selector (0 = no table)
        }

        segment.push_back(static_cast<uint8_t>(near_lossless));   // NEAR parameter
        segment.push_back(static_cast<uint8_t>(interleave_mode)); // ILV parameter
        segment.push_back(0);                                     // transformation

        write_segment(jpeg_marker_code::start_of_scan, segment.data(), segment.size());
    }

    void write_segment(const jpeg_marker_code marker_code, const void* data, const size_t data_size)
    {
        write_marker(marker_code);
        write_uint16(static_cast<uint16_t>(data_size + 2));
        write_bytes(data, data_size);
    }

    void write_marker(jpeg_marker_code marker_code)
    {
        write_byte(jpeg_marker_start_byte);
        write_byte(static_cast<uint8_t>(marker_code));
    }

    void write_uint16(const uint16_t value)
    {
        write_byte(static_cast<uint8_t>(value / 0x100));
        write_byte(static_cast<uint8_t>(value % 0x100));
    }

    void write_byte(const uint8_t value)
    {
        buffer.push_back(value);
    }

    void write_bytes(const void* data, const size_t data_size)
    {
        const auto* const bytes{static_cast<const uint8_t*>(data)};

        for (size_t i{}; i < data_size; ++i)
        {
            write_byte(bytes[i]);
        }
    }

    int componentIdOverride{};
    uint8_t mapping_table_selector{};
    std::vector<uint8_t> buffer;
};

}} // namespace charls::test
