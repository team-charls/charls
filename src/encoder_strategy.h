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

    virtual std::unique_ptr<process_line> create_process(byte_stream_info stream_info, uint32_t stride) = 0;
    virtual void set_presets(const jpegls_pc_parameters& preset_coding_parameters) = 0;
    virtual std::size_t encode_scan(std::unique_ptr<process_line> raw_data, byte_stream_info& compressed_data) = 0;

    int32_t peek_byte();

    void on_line_begin(const size_t pixel_count, void* destination, const int32_t pixel_stride) const
    {
        processLine_->new_line_requested(destination, pixel_count, pixel_stride);
    }

    static void on_line_end(size_t /*pixel_count*/, void* /*destination*/, int32_t /*pixel_stride*/) noexcept
    {
    }

protected:
    void initialize(byte_stream_info& compressed_stream)
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

    void append_to_bit_stream(const uint32_t bits, const int32_t bit_count)
    {
        ASSERT(bit_count < 32 && bit_count >= 0);
        ASSERT((!decoder_) || (bit_count == 0 && bits == 0) || (static_cast<uint32_t>(decoder_->read_long_value(bit_count)) == bits));
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
            flush();

            // A second flush may be required if extra marker detect bits were needed and not all bits could be written.
            if (freeBitCount_ < 0)
            {
                bitBuffer_ |= bits >> -freeBitCount_;
                flush();
            }

            ASSERT(freeBitCount_ >= 0);
            bitBuffer_ |= bits << freeBitCount_;
        }
    }

    void end_scan()
    {
        flush();

        // if a 0xff was written, Flush() will force one unset bit anyway
        if (isFFWritten_)
        {
            append_to_bit_stream(0, (freeBitCount_ - 1) % 8);
        }
        else
        {
            append_to_bit_stream(0, freeBitCount_ % 8);
        }

        flush();
        ASSERT(freeBitCount_ == 0x20);

        if (compressedStream_)
        {
            overflow();
        }
    }

    void overflow()
    {
        if (!compressedStream_)
            impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

        const auto bytes_count = static_cast<size_t>(position_ - buffer_.data());
        const auto bytes_written = static_cast<size_t>(compressedStream_->sputn(reinterpret_cast<char*>(buffer_.data()), position_ - buffer_.data()));

        if (bytes_written != bytes_count)
            impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

        position_ = buffer_.data();
        compressedLength_ = buffer_.size();
    }

    void flush()
    {
        if (compressedLength_ < 4)
        {
            overflow();
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

    std::size_t get_length() const noexcept
    {
        return bytesWritten_ - (static_cast<uint32_t>(freeBitCount_) - 32U) / 8U;
    }

    FORCE_INLINE void append_ones_to_bit_stream(const int32_t length)
    {
        append_to_bit_stream((1 << length) - 1, length);
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
