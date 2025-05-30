// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpeg_marker_code.hpp"
#include "copy_to_line_buffer.hpp"
#include "scan_codec.hpp"
#include "span.hpp"

namespace charls {

/// <summary>
/// Interface and primary class for scan_encoder.
/// The actual instance will be a template derived implementation class.
/// This class can encode pixels to entropy data.
/// </summary>
class scan_encoder : protected scan_codec
{
public:
    virtual ~scan_encoder() = default;

    scan_encoder(const scan_encoder&) = delete;
    scan_encoder(scan_encoder&&) = delete;
    scan_encoder& operator=(const scan_encoder&) = delete;
    scan_encoder& operator=(scan_encoder&&) = delete;

    virtual size_t encode_scan(const std::byte* source, size_t stride, span<std::byte> destination) = 0;

protected:
    scan_encoder(const charls::frame_info& frame_info, const jpegls_pc_parameters& pc_parameters,
                 const coding_parameters& parameters, const copy_to_line_buffer_fn copy_to_line_buffer) noexcept :
        scan_codec(frame_info, pc_parameters, parameters),
        copy_to_line_buffer_{copy_to_line_buffer},
        mask_{(1U << frame_info.bits_per_sample) - 1}
    {
    }

    void copy_source_to_line_buffer(const std::byte* source, void* destination, const size_t pixel_count) const noexcept
    {
        copy_to_line_buffer_(source, destination, pixel_count, mask_);
    }

    void initialize(const span<std::byte> destination) noexcept
    {
        free_bit_count_ = sizeof(bit_buffer_) * 8;
        bit_buffer_ = 0;

        position_ = destination.data();
        compressed_length_ = destination.size();
    }

    void encode_run_pixels(size_t run_length, const bool end_of_line)
    {
        while (run_length >= size_t{1} << J[run_index_])
        {
            append_ones_to_bit_stream(1);
            run_length = run_length - (size_t{1} << J[run_index_]);
            increment_run_index();
        }

        if (end_of_line)
        {
            if (run_length != 0)
            {
                append_ones_to_bit_stream(1);
            }
        }
        else
        {
            append_to_bit_stream(static_cast<uint32_t>(run_length), J[run_index_] + 1); // leading 0 + actual remaining length
        }
    }

    void append_to_bit_stream(const uint32_t bits, const int32_t bit_count)
    {
        ASSERT(0 <= bit_count && bit_count < 32);
        ASSERT((bits | ((1U << bit_count) - 1U)) == ((1U << bit_count) - 1U)); // Not used bits must be set to zero.

        free_bit_count_ -= bit_count;
        if (free_bit_count_ >= 0)
        {
            bit_buffer_ |= bits << free_bit_count_;
        }
        else
        {
            // Add as many bits in the remaining space as possible and flush.
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

        flush();
        ASSERT(free_bit_count_ == 32);
    }

    void flush()
    {
        if (UNLIKELY(compressed_length_ < 4))
            impl::throw_jpegls_error(jpegls_errc::destination_too_small);

        for (int i{}; i < 4; ++i)
        {
            if (free_bit_count_ >= 32)
            {
                free_bit_count_ = 32;
                break;
            }

            if (is_ff_written_)
            {
                // JPEG-LS requirement (T.87, A.1) to detect markers: after a xFF value a single 0 bit needs to be inserted.
                *position_ = static_cast<std::byte>(bit_buffer_ >> 25);
                bit_buffer_ = bit_buffer_ << 7;
                free_bit_count_ += 7;
            }
            else
            {
                *position_ = static_cast<std::byte>(bit_buffer_ >> 24);
                bit_buffer_ = bit_buffer_ << 8;
                free_bit_count_ += 8;
            }

            is_ff_written_ = *position_ == jpeg_marker_start_byte;
            ++position_;
            --compressed_length_;
            ++bytes_written_;
        }
    }

    [[nodiscard]]
    size_t get_length() const noexcept
    {
        return bytes_written_ - (static_cast<size_t>(free_bit_count_) - 32U) / 8U;
    }

    FORCE_INLINE void append_ones_to_bit_stream(const int32_t bit_count)
    {
        append_to_bit_stream((1U << bit_count) - 1U, bit_count);
    }

    copy_to_line_buffer_fn copy_to_line_buffer_{};

private:
    unsigned int bit_buffer_{};
    int32_t free_bit_count_{sizeof bit_buffer_ * 8};
    size_t compressed_length_{};
    uint32_t mask_;

    // encoding
    std::byte* position_{};
    bool is_ff_written_{};
    size_t bytes_written_{};
};

} // namespace charls
