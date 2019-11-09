// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/jpegls_error.h>

#include "util.h"
#include "process_line.h"
#include "jpeg_marker_code.h"

#include <memory>
#include <cassert>

namespace charls {

// Purpose: Implements encoding to stream of bits. In encoding mode JpegLsCodec inherits from EncoderStrategy
class DecoderStrategy
{
public:
    explicit DecoderStrategy(const JlsParameters& params) :
        params_{params}
    {
    }

    virtual ~DecoderStrategy() = default;

    DecoderStrategy(const DecoderStrategy&) = delete;
    DecoderStrategy(DecoderStrategy&&) = delete;
    DecoderStrategy& operator=(const DecoderStrategy&) = delete;
    DecoderStrategy& operator=(DecoderStrategy&&) = delete;

    virtual std::unique_ptr<ProcessLine> CreateProcess(ByteStreamInfo rawStreamInfo) = 0;
    virtual void SetPresets(const jpegls_pc_parameters& preset_coding_parameters) = 0;
    virtual void DecodeScan(std::unique_ptr<ProcessLine> outputData, const JlsRect& size, ByteStreamInfo& compressedData) = 0;

    void Init(ByteStreamInfo& compressedStream)
    {
        validBits_ = 0;
        readCache_ = 0;

        if (compressedStream.rawStream)
        {
            buffer_.resize(40000);
            position_ = buffer_.data();
            endPosition_ = position_;
            byteStream_ = compressedStream.rawStream;
            AddBytesFromStream();
        }
        else
        {
            byteStream_ = nullptr;
            position_ = compressedStream.rawData;
            endPosition_ = position_ + compressedStream.count;
        }

        nextFFPosition_ = FindNextFF();
        MakeValid();
    }

    void AddBytesFromStream()
    {
        if (!byteStream_ || byteStream_->sgetc() == std::char_traits<char>::eof())
            return;

        const auto count = endPosition_ - position_;

        if (count > 64)
            return;

        for (std::size_t i = 0; i < static_cast < std::size_t>(count); ++i)
        {
            buffer_[i] = position_[i];
        }
        const auto offset = buffer_.data() - position_;

        position_ += offset;
        endPosition_ += offset;
        nextFFPosition_ += offset;

        const std::streamsize readBytes = byteStream_->sgetn(reinterpret_cast<char*>(endPosition_),
            static_cast<std::streamsize>(buffer_.size()) - count);
        endPosition_ += readBytes;
    }

    FORCE_INLINE void Skip(int32_t length) noexcept
    {
        validBits_ -= length;
        readCache_ = readCache_ << length;
    }

    static void OnLineBegin(int32_t /*cpixel*/, void* /*ptypeBuffer*/, int32_t /*pixelStride*/) noexcept
    {
    }

    void OnLineEnd(int32_t pixelCount, const void* ptypeBuffer, int32_t pixelStride) const
    {
        processLine_->NewLineDecoded(ptypeBuffer, pixelCount, pixelStride);
    }

    void EndScan()
    {
        if (*position_ != JpegMarkerStartByte)
        {
            ReadBit();

            if (*position_ != JpegMarkerStartByte)
                throw jpegls_error{jpegls_errc::too_much_encoded_data};
        }

        if (readCache_ != 0)
            throw jpegls_error{jpegls_errc::too_much_encoded_data};
    }

    FORCE_INLINE bool OptimizedRead() noexcept
    {
        // Easy & fast: if there is no 0xFF byte in sight, we can read without bit stuffing
        if (position_ < nextFFPosition_ - (sizeof(bufType)-1))
        {
            readCache_ |= FromBigEndian<sizeof(bufType)>::Read(position_) >> validBits_;
            const int bytesToRead = (bufType_bit_count - validBits_) >> 3;
            position_ += bytesToRead;
            validBits_ += bytesToRead * 8;
            ASSERT(validBits_ >= bufType_bit_count - 8);
            return true;
        }
        return false;
    }

    void MakeValid()
    {
        ASSERT(validBits_ <= bufType_bit_count - 8);

        if (OptimizedRead())
            return;

        AddBytesFromStream();

        do
        {
            if (position_ >= endPosition_)
            {
                if (validBits_ <= 0)
                    throw jpegls_error{jpegls_errc::invalid_encoded_data};

                return;
            }

            const bufType valueNew = position_[0];

            if (valueNew == JpegMarkerStartByte)
            {
                // JPEG bit stream rule: no FF may be followed by 0x80 or higher
                if (position_ == endPosition_ - 1 || (position_[1] & 0x80) != 0)
                {
                    if (validBits_ <= 0)
                        throw jpegls_error{jpegls_errc::invalid_encoded_data};

                    return;
                }
            }

            readCache_ |= valueNew << (bufType_bit_count - 8 - validBits_);
            position_ += 1;
            validBits_ += 8;

            if (valueNew == JpegMarkerStartByte)
            {
                validBits_--;
            }
        }
        while (validBits_ < bufType_bit_count - 8);

        nextFFPosition_ = FindNextFF();
    }

    uint8_t* FindNextFF() const noexcept
    {
        auto positionNextFF = position_;

        while (positionNextFF < endPosition_)
        {
            if (*positionNextFF == JpegMarkerStartByte)
                break;

            positionNextFF++;
        }

        return positionNextFF;
    }

    uint8_t* GetCurBytePos() const noexcept
    {
        int32_t validBits = validBits_;
        uint8_t* compressedBytes = position_;

        for (;;)
        {
            const int32_t lastBitsCount = compressedBytes[-1] == JpegMarkerStartByte ? 7 : 8;

            if (validBits < lastBitsCount)
                return compressedBytes;

            validBits -= lastBitsCount;
            compressedBytes--;
        }
    }

    FORCE_INLINE int32_t ReadValue(int32_t length)
    {
        if (validBits_ < length)
        {
            MakeValid();
            if (validBits_ < length)
                throw jpegls_error{jpegls_errc::invalid_encoded_data};
        }

        ASSERT(length != 0 && length <= validBits_);
        ASSERT(length < 32);
        const auto result = static_cast<int32_t>(readCache_ >> (bufType_bit_count - length));
        Skip(length);
        return result;
    }

    FORCE_INLINE int32_t PeekByte()
    {
        if (validBits_ < 8)
        {
            MakeValid();
        }

        return static_cast<int32_t>(readCache_ >> (bufType_bit_count - 8));
    }

    FORCE_INLINE bool ReadBit()
    {
        if (validBits_ <= 0)
        {
            MakeValid();
        }

        const bool bSet = (readCache_ & (static_cast<bufType>(1) << (bufType_bit_count - 1))) != 0;
        Skip(1);
        return bSet;
    }

    FORCE_INLINE int32_t Peek0Bits()
    {
        if (validBits_ < 16)
        {
            MakeValid();
        }
        bufType valTest = readCache_;

        for (int32_t count = 0; count < 16; count++)
        {
            if ((valTest & (static_cast<bufType>(1) << (bufType_bit_count - 1))) != 0)
                return count;

            valTest <<= 1;
        }
        return -1;
    }

    FORCE_INLINE int32_t ReadHighBits()
    {
        const int32_t count = Peek0Bits();
        if (count >= 0)
        {
            Skip(count + 1);
            return count;
        }
        Skip(15);

        for (int32_t highBitsCount = 15; ; highBitsCount++)
        {
            if (ReadBit())
                return highBitsCount;
        }
    }

    int32_t ReadLongValue(int32_t length)
    {
        if (length <= 24)
            return ReadValue(length);

        return (ReadValue(length - 24) << 24) + ReadValue(24);
    }

protected:
    JlsParameters params_;
    std::unique_ptr<ProcessLine> processLine_;

private:
    using bufType = std::size_t;
    static constexpr auto bufType_bit_count = static_cast<int32_t>(sizeof(bufType) * 8);

    std::vector<uint8_t> buffer_;
    std::basic_streambuf<char>* byteStream_{};

    // decoding
    bufType readCache_{};
    int32_t validBits_{};
    uint8_t* position_{};
    uint8_t* nextFFPosition_{};
    uint8_t* endPosition_{};
};

} // namespace charls
