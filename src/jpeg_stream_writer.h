// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include <charls/jpegls_error.h>

#include "jpeg_marker_code.h"

#include <vector>

namespace charls {

enum class JpegMarkerCode : uint8_t;

// Purpose: 'Writer' class that can generate JPEG-LS file streams.
class JpegStreamWriter final
{
public:
    JpegStreamWriter() noexcept;
    explicit JpegStreamWriter(const ByteStreamInfo& destination) noexcept;

    void WriteStartOfImage();

    /// <summary>
    /// Write a JPEG File Interchange (APP1 + jfif) segment.
    /// </summary>
    /// <param name="params">Parameters to write into the JFIF segment.</param>
    void WriteJpegFileInterchangeFormatSegment(const JfifParameters& params);

    /// <summary>
    /// Writes a HP color transformation (APP8) segment.
    /// </summary>
    /// <param name="transformation">Color transformation to put into the segment.</param>
    void WriteColorTransformSegment(ColorTransformation transformation);

    /// <summary>
    /// Writes a JPEG-LS preset parameters (LSE) segment.
    /// </summary>
    /// <param name="params">Parameters to write into the JPEG-LS preset segment.</param>
    void WriteJpegLSPresetParametersSegment(const JpegLSPresetCodingParameters& params);

    /// <summary>
    /// Writes a JPEG-LS Start Of Frame (SOF-55) segment.
    /// </summary>
    /// <param name="width">The width of the frame.</param>
    /// <param name="height">The height of the frame.</param>
    /// <param name="bitsPerSample">The bits per sample.</param>
    /// <param name="componentCount">The component count.</param>
    void WriteStartOfFrameSegment(int width, int height, int bitsPerSample, int componentCount);

    /// <summary>
    /// Writes a JPEG-LS Start Of Scan (SOS) segment.
    /// </summary>
    /// <param name="componentCount">The number of components in the scan segment. Can only be > 1 when the components are interleaved.</param>
    /// <param name="allowedLossyError">The allowed lossy error. 0 means lossless.</param>
    /// <param name="interleaveMode">The interleave mode of the components.</param>
    void WriteStartOfScanSegment(int componentCount, int allowedLossyError, InterleaveMode interleaveMode);

    void WriteEndOfImage();

    std::size_t GetBytesWritten() const noexcept
    {
        return byteOffset_;
    }

    std::size_t GetLength() const noexcept
    {
        return destination_.count - byteOffset_;
    }

    ByteStreamInfo OutputStream() const noexcept
    {
        ByteStreamInfo data = destination_;
        data.count -= byteOffset_;
        data.rawData += byteOffset_;
        return data;
    }

    void Seek(std::size_t byteCount) noexcept
    {
        if (destination_.rawStream)
            return;

        byteOffset_ += byteCount;
    }

private:
    uint8_t* GetPos() const noexcept
    {
        return destination_.rawData + byteOffset_;
    }

    void WriteSegment(JpegMarkerCode markerCode, const void* data, size_t dataSize);

    void WriteByte(uint8_t value)
    {
        if (destination_.rawStream)
        {
            destination_.rawStream->sputc(value);
        }
        else
        {
            if (byteOffset_ >= destination_.count)
                throw jpegls_error{jpegls_errc::destination_buffer_too_small};

            destination_.rawData[byteOffset_++] = value;
        }
    }

    void WriteBytes(const std::vector<uint8_t>& bytes)
    {
        for (std::size_t i = 0; i < bytes.size(); ++i)
        {
            WriteByte(bytes[i]);
        }
    }

    void WriteBytes(const void* data, const size_t dataSize)
    {
        const auto bytes = static_cast<const uint8_t*>(data);

        for (std::size_t i = 0; i < dataSize; ++i)
        {
            WriteByte(bytes[i]);
        }
    }

    void WriteUInt16(uint16_t value)
    {
        WriteByte(static_cast<uint8_t>(value / 0x100));
        WriteByte(static_cast<uint8_t>(value % 0x100));
    }

    void WriteMarker(JpegMarkerCode markerCode)
    {
        WriteByte(JpegMarkerStartByte);
        WriteByte(static_cast<uint8_t>(markerCode));
    }

    ByteStreamInfo destination_;
    std::size_t byteOffset_{};
    int8_t componentId_{1};
};

} // namespace charls
