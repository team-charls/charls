// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "decoder_strategy.h"
#include "process_line.h"

namespace charls {

// Purpose: Implements encoding to stream of bits. In encoding mode JpegLsCodec inherits from EncoderStrategy
class encoder_strategy
{
public:
    explicit encoder_strategy(const frame_info& frame, const coding_parameters& parameters) noexcept :
        frame_info_{frame},
        parameters_{parameters}
    {
    }

    virtual ~encoder_strategy() = default;

    encoder_strategy(const encoder_strategy&) = delete;
    encoder_strategy(encoder_strategy&&) = delete;
    encoder_strategy& operator=(const encoder_strategy&) = delete;
    encoder_strategy& operator=(encoder_strategy&&) = delete;

    virtual std::unique_ptr<process_line> CreateProcess(byte_stream_info stream_info, uint32_t stride) = 0;
    virtual void SetPresets(const jpegls_pc_parameters& preset_coding_parameters) = 0;
    virtual std::size_t EncodeScan(std::unique_ptr<process_line> raw_data, byte_stream_info& compressed_data) = 0;

    int32_t PeekByte();

    void OnLineBegin(const size_t pixel_count, void* destination, const int32_t pixel_stride) const
    {
        processLine_->NewLineRequested(destination, pixel_count, pixel_stride);
    }

    static void OnLineEnd(size_t /*pixel_count*/, void* /*destination*/, int32_t /*pixel_stride*/) noexcept
    {
    }

protected:
    void Init(byte_stream_info& compressed_stream)
    {
        freeBitCount_ = sizeof(bitBuffer_) * 8;
        bitBuffer_ = 0;

        if (compressed_stream.rawStream)
        {
            compressedStream_ = compressed_stream.rawStream;
            buffer_.resize(4000);
            position_ = buffer_.data();
            compressedLength_ = buffer_.size();
        }
        else
        {
            position_ = compressed_stream.rawData;
            compressedLength_ = compressed_stream.count;
        }
    }

    void AppendToBitStream(const uint32_t bits, const int32_t bit_count)
    {
        ASSERT(bit_count < 32 && bit_count >= 0);
        ASSERT((!decoder_) || (bit_count == 0 && bits == 0) || (static_cast<uint32_t>(decoder_->ReadLongValue(bit_count)) == bits));
#ifndef NDEBUG
        const uint32_t mask = (1U << bit_count) - 1U;
        ASSERT((bits | mask) == mask); // Not used bits must be set to zero.
#endif

        freeBitCount_ -= bit_count;
        if (freeBitCount_ >= 0)
        {
            bitBuffer_ |= bits << freeBitCount_; // NOLINT
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
        {
            AppendToBitStream(0, (freeBitCount_ - 1) % 8);
        }
        else
        {
            AppendToBitStream(0, freeBitCount_ % 8);
        }

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
            impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

        const auto bytesCount = static_cast<size_t>(position_ - buffer_.data());
        const auto bytesWritten = static_cast<std::size_t>(compressedStream_->sputn(reinterpret_cast<char*>(buffer_.data()), position_ - buffer_.data()));

        if (bytesWritten != bytesCount)
            impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

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
            ++position_;
            --compressedLength_;
            ++bytesWritten_;
        }
    }

    std::size_t GetLength() const noexcept
    {
        return bytesWritten_ - (static_cast<uint32_t>(freeBitCount_) - 32U) / 8U;
    }

    FORCE_INLINE void AppendOnesToBitStream(const int32_t length)
    {
        AppendToBitStream((1 << length) - 1, length);
    }

    frame_info frame_info_;
    coding_parameters parameters_;
    std::unique_ptr<decoder_strategy> decoder_;
    std::unique_ptr<process_line> processLine_;

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
