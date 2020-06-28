// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "../src/jpeg_marker_code.h"
#include "../src/jpegls_preset_parameters_type.h"
#include "../src/util.h"

namespace charls {
namespace test {

class JpegTestStreamWriter final
{
public:
    void write_start_of_image()
    {
        WriteMarker(JpegMarkerCode::StartOfImage);
    }

    void write_start_of_frame_segment(const int width, const int height, const int bitsPerSample, const int componentCount)
    {
        // Create a Frame Header as defined in T.87, C.2.2 and T.81, B.2.2
        std::vector<uint8_t> segment;
        segment.push_back(static_cast<uint8_t>(bitsPerSample));    // P = Sample precision
        push_back(segment, static_cast<uint16_t>(height)); // Y = Number of lines
        push_back(segment, static_cast<uint16_t>(width));  // X = Number of samples per line

        // Components
        segment.push_back(static_cast<uint8_t>(componentCount)); // Nf = Number of image components in frame
        for (auto componentId = 0; componentId < componentCount; ++componentId)
        {
            // Component Specification parameters
            if (componentIdOverride == 0)
            {
                segment.push_back(static_cast<uint8_t>(componentId)); // Ci = Component identifier
            }
            else
            {
                segment.push_back(static_cast<uint8_t>(componentIdOverride)); // Ci = Component identifier
            }
            segment.push_back(0x11); // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
            segment.push_back(0);    // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
        }

        write_segment(charls::JpegMarkerCode::StartOfFrameJpegLS, segment.data(), segment.size());
    }

    void write_jpegls_preset_parameters_segment(const jpegls_pc_parameters& preset_coding_parameters)
    {
        std::vector<uint8_t> segment;

        segment.push_back(static_cast<uint8_t>(charls::JpegLSPresetParametersType::PresetCodingParameters));

        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.maximum_sample_value));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold1));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold2));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.threshold3));
        push_back(segment, static_cast<uint16_t>(preset_coding_parameters.reset_value));

        write_segment(charls::JpegMarkerCode::JpegLSPresetParameters, segment.data(), segment.size());
    }

    void WriteStartOfScanSegment(int component_id,
                                 const int component_count,
                                 const int near_lossless,
                                 const charls::interleave_mode interleave_mode)
    {
        // Create a Scan Header as defined in T.87, C.2.3 and T.81, B.2.3
        std::vector<uint8_t> segment;

        segment.push_back(static_cast<uint8_t>(component_count));
        for (auto i = 0; i < component_count; ++i)
        {
            segment.push_back(static_cast<uint8_t>(component_id));
            ++component_id;
            segment.push_back(0); // Mapping table selector (0 = no table)
        }

        segment.push_back(static_cast<uint8_t>(near_lossless));   // NEAR parameter
        segment.push_back(static_cast<uint8_t>(interleave_mode)); // ILV parameter
        segment.push_back(0);                                     // transformation

        write_segment(JpegMarkerCode::StartOfScan, segment.data(), segment.size());
    }

    void write_segment(const JpegMarkerCode markerCode, const void* data, const size_t dataSize)
    {
        WriteMarker(markerCode);
        write_uint16(static_cast<uint16_t>(dataSize + 2));
        write_bytes(data, dataSize);
    }

    void WriteMarker(JpegMarkerCode markerCode)
    {
        write_byte(charls::JpegMarkerStartByte);
        write_byte(static_cast<uint8_t>(markerCode));
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

    void write_bytes(const void* data, const size_t dataSize)
    {
        const auto* const bytes = static_cast<const uint8_t*>(data);

        for (std::size_t i = 0; i < dataSize; ++i)
        {
            write_byte(bytes[i]);
        }
    }

    int componentIdOverride{};
    std::vector<uint8_t> buffer;
};

}
} // namespace charls::test
