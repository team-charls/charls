// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "../src/jpeg_marker_code.hpp"
#include "../src/jpegls_preset_parameters_type.hpp"
#include "../src/util.hpp"
#include "../src/assert.hpp"

#include <vector>
#include <array>

namespace charls::test {

inline void push_back(std::vector<std::byte>& values, const uint16_t value)
{
    values.push_back(static_cast<std::byte>(value >> 8));
    values.push_back(static_cast<std::byte>(value));
}


inline void push_back(std::vector<std::byte>& values, const uint32_t value)
{
    values.push_back(static_cast<std::byte>(value >> 24));
    values.push_back(static_cast<std::byte>(value >> 16));
    values.push_back(static_cast<std::byte>(value >> 8));
    values.push_back(static_cast<std::byte>(value));
}


class jpeg_test_stream_writer final
{
public:
    void write_start_of_image()
    {
        write_marker(jpeg_marker_code::start_of_image);
    }

    void write_spiff_header_segment(const spiff_header& header)
    {
        ASSERT(header.height > 0);
        ASSERT(header.width > 0);

        static constexpr std::array spiff_magic_id{std::byte{'S'}, std::byte{'P'}, std::byte{'I'}, std::byte{'F'}, std::byte{'F'}, std::byte{'\0'}};

        // Create a JPEG APP8 segment in Still Picture Interchange File Format (SPIFF), v2.0
        write_marker(jpeg_marker_code::application_data8);
        write_uint16(30 + 2);
        write_bytes(spiff_magic_id.data(), spiff_magic_id.size());
        write_uint8(2);
        write_uint8(0);
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

    void write_spiff_end_of_directory_entry()
    {
        constexpr uint8_t spiff_end_of_directory_entry_type{1};

        // Note: ISO/IEC 10918-3, Annex F.2.2.3 documents that the EOD entry segment should have a length of 8
        // but only 6 data bytes. This approach allows to wrap existing bit streams\encoders with a SPIFF header.
        // In this implementation the SOI marker is added as data bytes to simplify the design.
        static constexpr std::array spiff_end_of_directory{
            std::byte{0},    std::byte{0},
            std::byte{0},    std::byte{spiff_end_of_directory_entry_type},
            std::byte{0xFF}, std::byte{to_underlying_type(jpeg_marker_code::start_of_image)}};
        write_segment(jpeg_marker_code::application_data8, spiff_end_of_directory.data(), spiff_end_of_directory.size());
    }

    void write_start_of_frame_segment(const int width, const int height, const int bits_per_sample,
                                      const int component_count)
    {
        // Create a Frame Header as defined in T.87, C.2.2 and T.81, B.2.2
        std::vector<std::byte> segment;
        segment.push_back(static_cast<std::byte>(bits_per_sample)); // P = Sample precision
        push_back(segment, static_cast<uint16_t>(height));          // Y = Number of lines
        push_back(segment, static_cast<uint16_t>(width));           // X = Number of samples per line

        // Components
        segment.push_back(static_cast<std::byte>(component_count)); // Nf = Number of image components in frame
        for (int component_id{}; component_id < component_count; ++component_id)
        {
            // Component Specification parameters
            if (componentIdOverride == 0)
            {
                segment.push_back(static_cast<std::byte>(component_id)); // Ci = Component identifier
            }
            else
            {
                segment.push_back(static_cast<std::byte>(componentIdOverride)); // Ci = Component identifier
            }
            segment.push_back(std::byte{0x11}); // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
            segment.push_back(
                std::byte{0}); // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
        }

        write_segment(jpeg_marker_code::start_of_frame_jpegls, segment.data(), segment.size());
    }

    void write_jpegls_preset_parameters_segment(const jpegls_pc_parameters& preset_coding_parameters)
    {
        std::vector<std::byte> segment;

        segment.push_back(static_cast<std::byte>(jpegls_preset_parameters_type::preset_coding_parameters));

        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.maximum_sample_value));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold1));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold2));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold3));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.reset_value));

        write_segment(jpeg_marker_code::jpegls_preset_parameters, segment.data(), segment.size());
    }

    void write_jpegls_preset_parameters_segment(const int32_t table_id, const int32_t entry_size,
                                                const std::vector<std::byte>& table_data, const bool continuation)
    {
        // Format is defined in ISO/IEC 14495-1, C.2.4.1.2 and C.2.4.1.3
        std::vector<std::byte> segment;

        segment.push_back(static_cast<std::byte>(continuation ? jpegls_preset_parameters_type::mapping_table_continuation
                                                              : jpegls_preset_parameters_type::mapping_table_specification));
        segment.push_back(static_cast<std::byte>(table_id));
        segment.push_back(static_cast<std::byte>(entry_size));
        segment.insert(end(segment), begin(table_data), end(table_data));

        write_segment(jpeg_marker_code::jpegls_preset_parameters, segment.data(), segment.size());
    }

    void write_oversize_image_dimension(const uint32_t number_of_bytes, const uint32_t width, const uint32_t height,
                                        const bool extra_byte = false)
    {
        // Format is defined in ISO/IEC 14495-1, C.2.4.1.4
        std::vector<std::byte> segment;

        segment.push_back(static_cast<std::byte>(jpegls_preset_parameters_type::oversize_image_dimension));
        segment.push_back(
            static_cast<std::byte>(number_of_bytes)); // Wxy: number of bytes used to represent Ye and Xe [2..4].
        switch (number_of_bytes)
        {
        case 2:
            push_back(segment, static_cast<uint16_t>(height));
            push_back(segment, static_cast<uint16_t>(width));
            break;

        case 3:
            push_back_uint24(segment, height);
            push_back_uint24(segment, width);
            break;

        default:
            push_back(segment, height);
            push_back(segment, width);
            break;
        }

        if (extra_byte)
        {
            // This will make the segment non-conforming.
            segment.push_back({});
        }

        write_segment(jpeg_marker_code::jpegls_preset_parameters, segment.data(), segment.size());
    }

    void write_start_of_scan_segment(int component_id, const int component_count, const int near_lossless,
                                     const interleave_mode interleave_mode)
    {
        // Create a Scan Header as defined in T.87, C.2.3 and T.81, B.2.3
        std::vector<std::byte> segment;

        segment.push_back(static_cast<std::byte>(component_count));
        for (int i{}; i < component_count; ++i)
        {
            segment.push_back(static_cast<std::byte>(component_id));
            ++component_id;
            segment.push_back(static_cast<std::byte>(mapping_table_selector)); // Mapping table selector (0 = no table)
        }

        segment.push_back(static_cast<std::byte>(near_lossless));   // NEAR parameter
        segment.push_back(static_cast<std::byte>(interleave_mode)); // ILV parameter
        segment.push_back({});                                      // transformation

        write_segment(jpeg_marker_code::start_of_scan, segment.data(), segment.size());
    }

    void write_define_restart_interval(const uint32_t restart_interval, const int32_t size)
    {
        // Segment is documented in ISO/IEC 14495-1, C.2.5
        std::vector<std::byte> segment;
        switch (size)
        {
        case 2:
            push_back(segment, static_cast<uint16_t>(restart_interval));
            break;

        case 3:
            push_back_uint24(segment, restart_interval);
            break;

        case 4:
            push_back(segment, restart_interval);
            break;

        default:
            assert(false);
            break;
        }

        write_segment(jpeg_marker_code::define_restart_interval, segment.data(), segment.size());
    }

    void write_define_number_of_lines(const uint32_t height, const int32_t size)
    {
        // Segment is documented in ISO/IEC 14495-1, C.2.6
        std::vector<std::byte> segment;
        switch (size)
        {
        case 2:
            push_back(segment, static_cast<uint16_t>(height));
            break;

        case 3:
            push_back_uint24(segment, height);
            break;

        case 4:
            push_back(segment, height);
            break;

        case 5:
            push_back(segment, height);
            // This will make the segment non-conforming.
            segment.push_back({});
            break;

        default:
            assert(false);
            break;
        }

        write_segment(jpeg_marker_code::define_number_of_lines, segment.data(), segment.size());
    }

    void write_restart_marker(const uint8_t interval_index)
    {
        write_marker(static_cast<jpeg_marker_code>(jpeg_restart_marker_base + interval_index));
    }

    void write_segment(const jpeg_marker_code marker_code, const void* data, const size_t data_size)
    {
        write_marker(marker_code);
        write_uint16(static_cast<uint16_t>(data_size + 2));
        write_bytes(static_cast<const std::byte*>(data), data_size);
    }

    void write_marker(jpeg_marker_code marker_code)
    {
        write_byte(jpeg_marker_start_byte);
        write_byte(static_cast<std::byte>(marker_code));
    }

    void write_uint8(const uint32_t value)
    {
        write_byte(static_cast<std::byte>(value));
    }

    void write_uint8(const int32_t value)
    {
        write_byte(static_cast<std::byte>(value));
    }

    void write_uint16(const uint16_t value)
    {
        write_byte(static_cast<std::byte>(value / 0x100));
        write_byte(static_cast<std::byte>(value % 0x100));
    }

    void write_uint32(const uint32_t value)
    {
        write_byte(static_cast<std::byte>(value > 3));
        write_byte(static_cast<std::byte>(value > 2));
        write_byte(static_cast<std::byte>(value > 1));
        write_byte(static_cast<std::byte>(value));
    }

    void write_byte(const std::byte value)
    {
        buffer.push_back(value);
    }

    void write_bytes(const std::byte* data, const size_t data_size)
    {
        for (size_t i{}; i != data_size; ++i)
        {
            write_byte(data[i]);
        }
    }

    int componentIdOverride{};
    uint8_t mapping_table_selector{};
    std::vector<std::byte> buffer;

private:
    static void push_back_uint24(std::vector<std::byte>& values, const uint32_t value)
    {
        values.push_back(static_cast<std::byte>(value >> 16));
        values.push_back(static_cast<std::byte>(value >> 8));
        values.push_back(static_cast<std::byte>(value));
    }
};

} // namespace charls::test
