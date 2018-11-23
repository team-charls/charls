// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include "jpeg_segment.h"

#include <charls/jpegls_error.h>

#include <vector>
#include <memory>

namespace charls {

enum class JpegMarkerCode : uint8_t;

// Purpose: 'Writer' class that can generate JPEG-LS file streams.
class JpegStreamWriter final
{
    friend class JpegMarkerSegment;
    friend class JpegImageDataSegment;

public:
    JpegStreamWriter() noexcept;

    void AddSegment(std::unique_ptr<JpegSegment> segment)
    {
        segments_.push_back(std::move(segment));
    }

    void AddScan(const ByteStreamInfo& info, const JlsParameters& params);

    void AddColorTransform(charls::ColorTransformation transformation);

    std::size_t GetBytesWritten() const noexcept
    {
        return byteOffset_;
    }

    std::size_t GetLength() const noexcept
    {
        return data_.count - byteOffset_;
    }

    std::size_t Write(const ByteStreamInfo& info);

private:
    uint8_t* GetPos() const noexcept
    {
        return data_.rawData + byteOffset_;
    }

    ByteStreamInfo OutputStream() const noexcept
    {
        ByteStreamInfo data = data_;
        data.count -= byteOffset_;
        data.rawData += byteOffset_;
        return data;
    }

    void WriteByte(uint8_t val)
    {
        if (data_.rawStream)
        {
            data_.rawStream->sputc(val);
        }
        else
        {
            if (byteOffset_ >= data_.count)
                throw jpegls_error(jpegls_errc::destination_buffer_too_small);

            data_.rawData[byteOffset_++] = val;
        }
    }

    void WriteBytes(const std::vector<uint8_t>& bytes)
    {
        for (std::size_t i = 0; i < bytes.size(); ++i)
        {
            WriteByte(bytes[i]);
        }
    }

    void WriteWord(uint16_t value)
    {
        WriteByte(static_cast<uint8_t>(value / 0x100));
        WriteByte(static_cast<uint8_t>(value % 0x100));
    }

    void WriteMarker(JpegMarkerCode marker)
    {
        WriteByte(0xFF);
        WriteByte(static_cast<uint8_t>(marker));
    }

    void Seek(std::size_t byteCount) noexcept
    {
        if (data_.rawStream)
            return;

        byteOffset_ += byteCount;
    }

    ByteStreamInfo data_;
    std::size_t byteOffset_;
    int32_t lastComponentIndex_;
    std::vector<std::unique_ptr<JpegSegment>> segments_;
};

} // namespace charls
