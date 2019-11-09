// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "decoder_strategy.h"
#include "process_line.h"

namespace charls {

// Purpose: Implements encoding to stream of bits. In encoding mode JpegLsCodec inherits from EncoderStrategy
class EncoderStrategy
{
public:
    explicit EncoderStrategy(const JlsParameters& params) :
        params_{params}
    {
    }

    virtual ~EncoderStrategy() = default;

    EncoderStrategy(const EncoderStrategy&) = delete;
    EncoderStrategy(EncoderStrategy&&) = delete;
    EncoderStrategy& operator=(const EncoderStrategy&) = delete;
    EncoderStrategy& operator=(EncoderStrategy&&) = delete;

    virtual std::unique_ptr<ProcessLine> CreateProcess(ByteStreamInfo rawStreamInfo) = 0;
    virtual void SetPresets(const jpegls_pc_parameters& preset_coding_parameters) = 0;
    virtual std::size_t EncodeScan(std::unique_ptr<ProcessLine> rawData, ByteStreamInfo& compressedData) = 0;

    int32_t PeekByte();

    void OnLineBegin(int32_t cpixel, void* ptypeBuffer, int32_t pixelStride) const
    {
        processLine_->NewLineRequested(ptypeBuffer, cpixel, pixelStride);
    }

    static void OnLineEnd(int32_t /*cpixel*/, void* /*ptypeBuffer*/, int32_t /*pixelStride*/) noexcept
    {
    }

protected:
    void Init(ByteStreamInfo& compressedStream)
    {
        freeBitCount_ = sizeof(bitBuffer_) * 8;
        bitBuffer_ = 0;

        if (compressedStream.rawStream)
        {
            compressedStream_ = compressedStream.rawStream;
            buffer_.resize(4000);
            position_ = buffer_.data();
            compressedLength_ = buffer_.size();
        }
        else
        {
            position_ = compressedStream.rawData;
            compressedLength_ = compressedStream.count;
        }
    }

    void AppendToBitStream(int32_t bits, int32_t bitCount)
    {
        ASSERT(bitCount < 32 && bitCount >= 0);
        ASSERT((!decoder_) || (bitCount == 0 && bits == 0) || (decoder_->ReadLongValue(bitCount) == bits));
#ifndef NDEBUG
        const int mask = (1U << (bitCount)) - 1;
        ASSERT((bits | mask) == mask); // Not used bits must be set to zero.
#endif

        freeBitCount_ -= bitCount;
        if (freeBitCount_ >= 0)
        {
            bitBuffer_ |= bits << freeBitCount_;
        }
        else
        {
            // Add as much bits in the remaining space as possible and flush.
            bitBuffer_ |= bits >> -freeBitCount_;
            Flush();

            // A second flush may be required if extra marker detect bits were needed and not all bits could be written.
            if (freeBitCount_ < 0)
            {
                bitBuffer_ |= bits >> -freeBitCount_;
                Flush();
            }

            ASSERT(freeBitCount_ >= 0);
            bitBuffer_ |= bits << freeBitCount_;
        }
    }

    void EndScan()
    {
        Flush();

        // if a 0xff was written, Flush() will force one unset bit anyway
        if (isFFWritten_)
            AppendToBitStream(0, (freeBitCount_ - 1) % 8);
        else
            AppendToBitStream(0, freeBitCount_ % 8);

        Flush();
        ASSERT(freeBitCount_ == 0x20);

        if (compressedStream_)
        {
            OverFlow();
        }
    }

    void OverFlow()
    {
        if (!compressedStream_)
            throw jpegls_error{jpegls_errc::destination_buffer_too_small};

        const std::size_t bytesCount = position_ - buffer_.data();
        const auto bytesWritten = static_cast<std::size_t>(compressedStream_->sputn(reinterpret_cast<char*>(buffer_.data()), position_ - buffer_.data()));

        if (bytesWritten != bytesCount)
            throw jpegls_error{jpegls_errc::destination_buffer_too_small};

        position_ = buffer_.data();
        compressedLength_ = buffer_.size();
    }

    void Flush()
    {
        if (compressedLength_ < 4)
        {
            OverFlow();
        }

        for (int i = 0; i < 4; ++i)
        {
            if (freeBitCount_ >= 32)
                break;

            if (isFFWritten_)
            {
                // JPEG-LS requirement (T.87, A.1) to detect markers: after a xFF value a single 0 bit needs to be inserted.
                *position_ = static_cast<uint8_t>(bitBuffer_ >> 25);
                bitBuffer_ = bitBuffer_ << 7;
                freeBitCount_ += 7;
            }
            else
            {
                *position_ = static_cast<uint8_t>(bitBuffer_ >> 24);
                bitBuffer_ = bitBuffer_ << 8;
                freeBitCount_ += 8;
            }

            isFFWritten_ = *position_ == JpegMarkerStartByte;
            position_++;
            compressedLength_--;
            bytesWritten_++;
        }
    }

    std::size_t GetLength() const noexcept
    {
        return bytesWritten_ - (freeBitCount_ - 32) / 8;
    }

    FORCE_INLINE void AppendOnesToBitStream(int32_t length)
    {
        AppendToBitStream((1 << length) - 1, length);
    }

    std::unique_ptr<DecoderStrategy> decoder_;
    JlsParameters params_;
    std::unique_ptr<ProcessLine> processLine_;

private:
    unsigned int bitBuffer_{};
    int32_t freeBitCount_{sizeof bitBuffer_ * 8};
    std::size_t compressedLength_{};

    // encoding
    uint8_t* position_{};
    bool isFFWritten_{};
    std::size_t bytesWritten_{};

    std::vector<uint8_t> buffer_;
    std::basic_streambuf<char>* compressedStream_{};
};

} // namespace charls
