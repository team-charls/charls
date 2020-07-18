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

    virtual std::unique_ptr<process_line> create_process_line(byte_stream_info stream_info, uint32_t stride) = 0;
    virtual void set_presets(const jpegls_pc_parameters& preset_coding_parameters) = 0;
    virtual std::size_t encode_scan(std::unique_ptr<process_line> raw_data, byte_stream_info& compressed_data) = 0;

    int32_t peek_byte();

    void on_line_begin(const size_t pixel_count, void* destination, const int32_t pixel_stride) const
    {
        process_line_->new_line_requested(destination, pixel_count, pixel_stride);
    }

    static void on_line_end(size_t /*pixel_count*/, void* /*destination*/, int32_t /*pixel_stride*/) noexcept
    {
    }

protected:
    void initialize(byte_stream_info& compressed_stream)
    {
        free_bit_count_ = sizeof(bit_buffer_) * 8;
        bit_buffer_ = 0;

        if (compressed_stream.rawStream)
        {
            compressed_stream_ = compressed_stream.rawStream;
            buffer_.resize(4000);
            position_ = buffer_.data();
            compressed_length_ = buffer_.size();
        }
        else
        {
            position_ = compressed_stream.rawData;
            compressed_length_ = compressed_stream.count;
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

        free_bit_count_ -= bit_count;
        if (free_bit_count_ >= 0)
        {
            bit_buffer_ |= bits << free_bit_count_; // NOLINT
        }
        else
        {
            // Add as much bits in the remaining space as possible and flush.
            bit_buffer_ |= bits >> -free_bit_count_;
            flush();

            // A second flush may be required if extra marker detect bits were needed and not all bits could be written.
            if (free_bit_count_ < 0)
            {
                bit_buffer_ |= bits >> -free_bit_count_;
                flush();
            }

            ASSERT(free_bit_count_ >= 0);
            bit_buffer_ |= bits << free_bit_count_;
        }
    }

    void end_scan()
    {
        flush();

        // if a 0xff was written, Flush() will force one unset bit anyway
        if (is_ff_written_)
        {
            append_to_bit_stream(0, (free_bit_count_ - 1) % 8);
        }
        else
        {
            append_to_bit_stream(0, free_bit_count_ % 8);
        }

        flush();
        ASSERT(free_bit_count_ == 0x20);

        if (compressed_stream_)
        {
            overflow();
        }
    }

    void overflow()
    {
        if (!compressed_stream_)
            impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

        const auto bytes_count = static_cast<size_t>(position_ - buffer_.data());
        const auto bytes_written = static_cast<size_t>(compressed_stream_->sputn(reinterpret_cast<char*>(buffer_.data()), position_ - buffer_.data()));

        if (bytes_written != bytes_count)
            impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

        position_ = buffer_.data();
        compressed_length_ = buffer_.size();
    }

    void flush()
    {
        if (compressed_length_ < 4)
        {
            overflow();
        }

        for (int i = 0; i < 4; ++i)
        {
            if (free_bit_count_ >= 32)
                break;

            if (is_ff_written_)
            {
                // JPEG-LS requirement (T.87, A.1) to detect markers: after a xFF value a single 0 bit needs to be inserted.
                *position_ = static_cast<uint8_t>(bit_buffer_ >> 25);
                bit_buffer_ = bit_buffer_ << 7;
                free_bit_count_ += 7;
            }
            else
            {
                *position_ = static_cast<uint8_t>(bit_buffer_ >> 24);
                bit_buffer_ = bit_buffer_ << 8;
                free_bit_count_ += 8;
            }

            is_ff_written_ = *position_ == jpeg_marker_start_byte;
            ++position_;
            --compressed_length_;
            ++bytes_written_;
        }
    }

    std::size_t get_length() const noexcept
    {
        return bytes_written_ - (static_cast<uint32_t>(free_bit_count_) - 32U) / 8U;
    }

    FORCE_INLINE void append_ones_to_bit_stream(const int32_t length)
    {
        append_to_bit_stream((1 << length) - 1, length);
    }

    frame_info frame_info_;
    coding_parameters parameters_;
    std::unique_ptr<decoder_strategy> decoder_;
    std::unique_ptr<process_line> process_line_;

private:
    unsigned int bit_buffer_{};
    int32_t free_bit_count_{sizeof bit_buffer_ * 8};
    std::size_t compressed_length_{};

    // encoding
    uint8_t* position_{};
    bool is_ff_written_{};
    std::size_t bytes_written_{};

    std::vector<uint8_t> buffer_;
    std::basic_streambuf<char>* compressed_stream_{};
};

} // namespace charls
