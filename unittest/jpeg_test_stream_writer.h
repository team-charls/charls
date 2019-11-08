// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "../src/jpeg_marker_code.h"
#include "../src/util.h"

namespace CharLSUnitTest {

class JpegTestStreamWriter final
{
public:
    void WriteStartOfImage()
    {
        WriteMarker(charls::JpegMarkerCode::StartOfImage);
    }

    void WriteStartOfFrameSegment(int width, int height, int bitsPerSample, int componentCount)
    {
        // Create a Frame Header as defined in T.87, C.2.2 and T.81, B.2.2
        std::vector<uint8_t> segment;
        segment.push_back(static_cast<uint8_t>(bitsPerSample));    // P = Sample precision
        charls::push_back(segment, static_cast<uint16_t>(height)); // Y = Number of lines
        charls::push_back(segment, static_cast<uint16_t>(width));  // X = Number of samples per line

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

        WriteSegment(charls::JpegMarkerCode::StartOfFrameJpegLS, segment.data(), segment.size());
    }

    void WriteSegment(charls::JpegMarkerCode markerCode, const void* data, size_t dataSize)
    {
        WriteMarker(markerCode);
        WriteUInt16(static_cast<uint16_t>(dataSize + 2));
        WriteBytes(data, dataSize);
    }

    void WriteMarker(charls::JpegMarkerCode markerCode)
    {
        WriteByte(charls::JpegMarkerStartByte);
        WriteByte(static_cast<uint8_t>(markerCode));
    }

    void WriteUInt16(uint16_t value)
    {
        WriteByte(static_cast<uint8_t>(value / 0x100));
        WriteByte(static_cast<uint8_t>(value % 0x100));
    }

    void WriteByte(uint8_t value)
    {
        data_.push_back(value);
    }

    void WriteBytes(const void* data, const size_t dataSize)
    {
        const auto bytes = static_cast<const uint8_t*>(data);

        for (std::size_t i = 0; i < dataSize; ++i)
        {
            WriteByte(bytes[i]);
        }
    }

    int componentIdOverride{};
    std::vector<uint8_t> data_;
};

} // namespace CharLSUnitTest
