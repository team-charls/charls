// SPDX-FileCopyrightText: © 2009 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/jpegls_error.hpp>

#include "copy_from_line_buffer.hpp"
#include "jpeg_marker_code.hpp"
#include "scan_codec.hpp"
#include "span.hpp"
#include "util.hpp"
#include "assert.hpp"

namespace charls {

/// <summary>
/// Interface and primary class for scan_decoder.
/// The actual instance will be a template implementation class.
/// This class can decode the encoded entropy data to pixels.
/// </summary>
class scan_decoder : protected scan_codec
{
public:
    virtual ~scan_decoder() = default;

    scan_decoder(const scan_decoder&) = delete;
    scan_decoder(scan_decoder&&) = delete;
    scan_decoder& operator=(const scan_decoder&) = delete;
    scan_decoder& operator=(scan_decoder&&) = delete;

    [[nodiscard]]
    virtual size_t decode_scan(span<const std::byte> source, std::byte* destination, size_t stride) = 0;

protected:
    using scan_codec::scan_codec;

    void initialize(const span<const std::byte> source)
    {
        position_ = source.data();
        end_position_ = to_address(source.end());

        find_jpeg_marker_start_byte();
        fill_read_cache();
    }

    void re_initialize_read_cache()
    {
        valid_bits_ = 0;
        read_cache_ = 0;

        find_jpeg_marker_start_byte();
        fill_read_cache();
    }

    FORCE_INLINE void skip_bits(const int32_t bit_count) noexcept
    {
        ASSERT(bit_count);

        valid_bits_ -= bit_count; // Note: valid_bits_ may become negative to indicate that extra bits are needed.
        read_cache_ = read_cache_ << bit_count;
    }

    void copy_line_buffer_to_destination(const void* source, void* destination, const size_t pixel_count) const noexcept
    {
        copy_from_line_buffer_(source, static_cast<std::byte*>(destination), pixel_count);
    }

    void end_scan()
    {
        if (UNLIKELY(position_ >= end_position_))
            impl::throw_jpegls_error(jpegls_errc::need_more_data);

        if (*position_ != jpeg_marker_start_byte)
        {
            read_bit();

            if (UNLIKELY(position_ >= end_position_))
                impl::throw_jpegls_error(jpegls_errc::need_more_data);

            if (UNLIKELY(*position_ != jpeg_marker_start_byte))
                impl::throw_jpegls_error(jpegls_errc::invalid_data);
        }

        if (UNLIKELY(read_cache_ != 0))
            impl::throw_jpegls_error(jpegls_errc::invalid_data);
    }

    [[nodiscard]]
    const std::byte* get_actual_position() const noexcept
    {
        int32_t valid_bits{valid_bits_};
        const std::byte* compressed_bytes{position_};

        for (;;)
        {
            const int32_t last_bits_count{compressed_bytes[-1] == jpeg_marker_start_byte ? 7 : 8};

            if (valid_bits < last_bits_count)
                return compressed_bytes;

            valid_bits -= last_bits_count;
            --compressed_bytes;
        }
    }

    /// <summary>
    /// Step F.1, 9: decode the mapped error value MErrval from the limited golomb code stored in the bitstream.
    /// </summary>
    [[nodiscard]]
    int32_t decode_mapped_error_value(const int32_t k, const int32_t limit, const int32_t quantized_bits_per_pixel)
    {
        if (const int32_t unary_code{read_unary_code()}; unary_code < limit - quantized_bits_per_pixel - 1)
        {
            // Option a: mapped error value is stored as golomb code.
            return k == 0 ? unary_code : (unary_code << k) + read_value(k);
        }

        // Option b: unary code was escape code as mapped error value was too large,
        // read mapped error value - 1 from bitstream.
        return read_value(quantized_bits_per_pixel) + 1;
    }

    FORCE_INLINE int32_t read_value(const int32_t bit_count)
    {
        ASSERT(0 < bit_count && bit_count < 32);

        if (valid_bits_ < bit_count)
        {
            fill_read_cache();
            if (UNLIKELY(valid_bits_ < bit_count))
                impl::throw_jpegls_error(jpegls_errc::invalid_data);
        }

        ASSERT(bit_count <= valid_bits_);
        const auto result{static_cast<int32_t>(read_cache_ >> (cache_t_bit_count - bit_count))};
        skip_bits(bit_count);
        return result;
    }

    /// <summary>
    /// Reads a byte from the bitstream without removing it.
    /// This byte is used to check if there is a pre-computed Golomb code available.
    /// </summary>
    FORCE_INLINE size_t peek_byte()
    {
        if (valid_bits_ < 8)
        {
            fill_read_cache();
        }

        return read_cache_ >> max_readable_cache_bits;
    }

    FORCE_INLINE uintptr_t read_bit()
    {
        if (valid_bits_ <= 0)
        {
            fill_read_cache();
        }

        const uintptr_t bit{read_cache_ >> (cache_t_bit_count - 1)};
        skip_bits(1);
        return bit;
    }

    /// <summary>
    /// Peek how many leading zero bits are present.
    /// </summary>
    /// <returns>
    /// The number of leading zero bits or -1 when there are more than 16 leading zero bits.
    /// </returns>
    FORCE_INLINE int32_t peek_0_bits()
    {
        if (valid_bits_ < 16)
        {
            fill_read_cache();
        }

#if defined(_MSC_VER) || defined(__GNUC__)
        const auto count{countl_zero(read_cache_)};
        return count < 16 ? count : -1;
#else
        cache_t val_test = read_cache_;

        for (int32_t count{}; count < 16; ++count)
        {
            if ((val_test & (static_cast<cache_t>(1) << (cache_t_bit_count - 1))) != 0)
                return count;

            val_test <<= 1;
        }
        return -1;
#endif
    }

    /// <summary>
    /// Read zero bits until the first high bit.
    /// </summary>
    FORCE_INLINE int32_t read_unary_code()
    {
        if (const int32_t count{peek_0_bits()}; count >= 0)
        {
            skip_bits(count + 1);
            return count;
        }

        skip_bits(15);
        for (int32_t zero_bit_count{15};; ++zero_bit_count)
        {
            if (read_bit())
                return zero_bit_count;
        }
    }

    int32_t read_long_value(const int32_t length)
    {
        if (length <= 24)
            return read_value(length);

        return (read_value(length - 24) << 24) + read_value(24);
    }

    std::byte read_byte()
    {
        if (UNLIKELY(position_ == end_position_))
            impl::throw_jpegls_error(jpegls_errc::need_more_data);

        const std::byte value = *position_;
        ++position_;
        return value;
    }

    void process_restart_marker()
    {
        read_restart_marker(jpeg_restart_marker_base + restart_interval_counter_);
        restart_interval_counter_ = (restart_interval_counter_ + 1) % jpeg_restart_marker_range;

        re_initialize_read_cache();
    }

    copy_from_line_buffer_fn copy_from_line_buffer_{};

private:
    using cache_t = size_t;

    void fill_read_cache()
    {
        ASSERT(valid_bits_ <= max_readable_cache_bits);

        if (fill_read_cache_optimistic())
            return;

        do
        {
            if (position_ >= end_position_)
            {
                if (UNLIKELY(valid_bits_ == 0))
                {
                    // Decoding process expects at least some bits to be added to the cache.
                    impl::throw_jpegls_error(jpegls_errc::invalid_data);
                }

                return;
            }

            const cache_t new_byte_value{std::to_integer<cache_t>(*position_)};

            // JPEG-LS bit stream rule: if FF is followed by a 1 bit then it is a marker
            if (new_byte_value == std::to_integer<cache_t>(jpeg_marker_start_byte) &&
                (position_ == end_position_ - 1 || (position_[1] & std::byte{0x80}) != std::byte{}))
            {
                if (UNLIKELY(valid_bits_ <= 0))
                {
                    // Decoding process expects at least some bits to be added to the cache.
                    impl::throw_jpegls_error(jpegls_errc::invalid_data);
                }

                // End of buffer or marker detected. Typical found markers are EOI, SOS (next scan) or RSTm.
                return;
            }

            read_cache_ |= new_byte_value << (max_readable_cache_bits - valid_bits_);
            valid_bits_ += 8;
            ++position_;

            if (new_byte_value == std::to_integer<cache_t>(jpeg_marker_start_byte))
            {
                // The next bit after an 0xFF needs to be ignored, compensate for the next read (see ISO/IEC 14495-1,A.1)
                --valid_bits_;
            }

        } while (valid_bits_ < max_readable_cache_bits);

        find_jpeg_marker_start_byte();
    }

    FORCE_INLINE bool fill_read_cache_optimistic() noexcept
    {
        if (position_ >= position_ff_ - (sizeof(cache_t) - 1))
            return false;

        // Easy & fast: there is no 0xFF byte in sight, read without bit stuffing
        read_cache_ |= read_big_endian_unaligned<cache_t>(position_) >> valid_bits_;
        const int bytes_consumed{(cache_t_bit_count - valid_bits_) / 8};
        position_ += bytes_consumed;
        valid_bits_ += bytes_consumed * 8;
        ASSERT(valid_bits_ >= max_readable_cache_bits);
        return true;
    }

    void find_jpeg_marker_start_byte() noexcept
    {
        // Use memchr to find next start byte (0xFF). memchr is optimized on some platforms to search faster.
        position_ff_ = static_cast<const std::byte*>(
            memchr(position_, std::to_integer<int>(jpeg_marker_start_byte), static_cast<size_t>(end_position_ - position_)));
        if (!position_ff_)
        {
            position_ff_ = end_position_;
        }
    }

    void read_restart_marker(const uint32_t expected_restart_marker_id)
    {
        auto value{read_byte()};
        if (UNLIKELY(value != jpeg_marker_start_byte))
            impl::throw_jpegls_error(jpegls_errc::restart_marker_not_found);

        // Read all preceding 0xFF fill bytes until a non 0xFF byte has been found. (see T.81, B.1.1.2)
        do
        {
            value = read_byte();
        } while (value == jpeg_marker_start_byte);

        if (UNLIKELY(std::to_integer<uint32_t>(value) != expected_restart_marker_id))
            impl::throw_jpegls_error(jpegls_errc::restart_marker_not_found);
    }

    static constexpr auto cache_t_bit_count{static_cast<int32_t>(sizeof(cache_t) * 8)};
    static constexpr int32_t max_readable_cache_bits{cache_t_bit_count - 8};

    // decoding
    cache_t read_cache_{};
    int32_t valid_bits_{};
    uint32_t restart_interval_counter_{};
    const std::byte* position_{};
    const std::byte* end_position_{};
    const std::byte* position_ff_{};
};

} // namespace charls
