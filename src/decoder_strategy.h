// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/jpegls_error.h>

#include "jpeg_marker_code.h"
#include "process_line.h"
#include "util.h"

#include <cassert>
#include <memory>

namespace charls {

// Purpose: Implements encoding to stream of bits. In encoding mode JpegLsCodec inherits from EncoderStrategy
class decoder_strategy
{
public:
    explicit decoder_strategy(const frame_info& frame, const coding_parameters& parameters) noexcept :
        frame_info_{frame},
        parameters_{parameters}
    {
    }

    virtual ~decoder_strategy() = default;

    decoder_strategy(const decoder_strategy&) = delete;
    decoder_strategy(decoder_strategy&&) = delete;
    decoder_strategy& operator=(const decoder_strategy&) = delete;
    decoder_strategy& operator=(decoder_strategy&&) = delete;

    virtual std::unique_ptr<process_line> create_process(byte_stream_info raw_stream_info, uint32_t stride) = 0;
    virtual void set_presets(const jpegls_pc_parameters& preset_coding_parameters) = 0;
    virtual void decode_scan(std::unique_ptr<process_line> output_data, const JlsRect& size, byte_stream_info& compressed_data) = 0;

    void initialize(byte_stream_info& compressed_stream)
    {
        validBits_ = 0;
        readCache_ = 0;

        if (compressed_stream.rawStream)
        {
            buffer_.resize(40000);
            position_ = buffer_.data();
            endPosition_ = position_;
            byteStream_ = compressed_stream.rawStream;
            add_bytes_from_stream();
        }
        else
        {
            byteStream_ = nullptr;
            position_ = compressed_stream.rawData;
            endPosition_ = position_ + compressed_stream.count;
        }

        nextFFPosition_ = find_next_ff();
        make_valid();
    }

    void add_bytes_from_stream()
    {
        if (!byteStream_ || byteStream_->sgetc() == std::char_traits<char>::eof())
            return;

        const auto count = endPosition_ - position_;

        if (count > 64)
            return;

        for (std::size_t i = 0; i < static_cast<std::size_t>(count); ++i)
        {
            buffer_[i] = position_[i];
        }
        const auto offset = buffer_.data() - position_;

        position_ += offset;
        endPosition_ += offset;
        nextFFPosition_ += offset;

        const std::streamsize read_bytes = byteStream_->sgetn(reinterpret_cast<char*>(endPosition_),
                                                              static_cast<std::streamsize>(buffer_.size()) - count);
        endPosition_ += read_bytes;
    }

    FORCE_INLINE void skip(const int32_t length) noexcept
    {
        validBits_ -= length;
        readCache_ = readCache_ << length;
    }

    static void on_line_begin(const size_t /*pixel_count*/, void* /*ptypeBuffer*/, int32_t /*pixelStride*/) noexcept
    {
    }

    void on_line_end(const size_t pixel_count, const void* source, const int32_t pixel_stride) const
    {
        processLine_->new_line_decoded(source, pixel_count, pixel_stride);
    }

    void end_scan()
    {
        if (*position_ != JpegMarkerStartByte)
        {
            read_bit();

            if (*position_ != JpegMarkerStartByte)
                impl::throw_jpegls_error(jpegls_errc::too_much_encoded_data);
        }

        if (readCache_ != 0)
            impl::throw_jpegls_error(jpegls_errc::too_much_encoded_data);
    }

    FORCE_INLINE bool optimized_read() noexcept
    {
        // Easy & fast: if there is no 0xFF byte in sight, we can read without bit stuffing
        if (position_ < nextFFPosition_ - (sizeof(bufType) - 1))
        {
            readCache_ |= from_big_endian<sizeof(bufType)>::read(position_) >> validBits_;
            const int bytes_to_read = (bufType_bit_count - validBits_) >> 3;
            position_ += bytes_to_read;
            validBits_ += bytes_to_read * 8;
            ASSERT(validBits_ >= bufType_bit_count - 8);
            return true;
        }
        return false;
    }

    void make_valid()
    {
        ASSERT(validBits_ <= bufType_bit_count - 8);

        if (optimized_read())
            return;

        add_bytes_from_stream();

        do
        {
            if (position_ >= endPosition_)
            {
                if (validBits_ <= 0)
                    impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

                return;
            }

            const bufType value_new = position_[0];

            if (value_new == JpegMarkerStartByte)
            {
                // JPEG bit stream rule: no FF may be followed by 0x80 or higher
                if (position_ == endPosition_ - 1 || (position_[1] & 0x80) != 0)
                {
                    if (validBits_ <= 0)
                        impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

                    return;
                }
            }

            readCache_ |= value_new << (bufType_bit_count - 8 - validBits_);
            position_ += 1;
            validBits_ += 8;

            if (value_new == JpegMarkerStartByte)
            {
                --validBits_;
            }
        } while (validBits_ < bufType_bit_count - 8);

        nextFFPosition_ = find_next_ff();
    }

    uint8_t* find_next_ff() const noexcept
    {
        auto* position_next_ff = position_;

        while (position_next_ff < endPosition_)
        {
            if (*position_next_ff == JpegMarkerStartByte)
                break;

            ++position_next_ff;
        }

        return position_next_ff;
    }

    uint8_t* get_cur_byte_pos() const noexcept
    {
        int32_t valid_bits = validBits_;
        uint8_t* compressed_bytes = position_;

        for (;;)
        {
            const int32_t last_bits_count = compressed_bytes[-1] == JpegMarkerStartByte ? 7 : 8;

            if (valid_bits < last_bits_count)
                return compressed_bytes;

            valid_bits -= last_bits_count;
            --compressed_bytes;
        }
    }

    FORCE_INLINE int32_t read_value(const int32_t length)
    {
        if (validBits_ < length)
        {
            make_valid();
            if (validBits_ < length)
                impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);
        }

        ASSERT(length != 0 && length <= validBits_);
        ASSERT(length < 32);
        const auto result = static_cast<int32_t>(readCache_ >> (bufType_bit_count - length));
        skip(length);
        return result;
    }

    FORCE_INLINE int32_t peek_byte()
    {
        if (validBits_ < 8)
        {
            make_valid();
        }

        return static_cast<int32_t>(readCache_ >> (bufType_bit_count - 8));
    }

    FORCE_INLINE bool read_bit()
    {
        if (validBits_ <= 0)
        {
            make_valid();
        }

        const bool set = (readCache_ & (static_cast<bufType>(1) << (bufType_bit_count - 1))) != 0;
        skip(1);
        return set;
    }

    FORCE_INLINE int32_t peek_0_bits()
    {
        if (validBits_ < 16)
        {
            make_valid();
        }
        bufType val_test = readCache_;

        for (int32_t count = 0; count < 16; ++count)
        {
            if ((val_test & (static_cast<bufType>(1) << (bufType_bit_count - 1))) != 0)
                return count;

            val_test <<= 1;
        }
        return -1;
    }

    FORCE_INLINE int32_t read_high_bits()
    {
        const int32_t count = peek_0_bits();
        if (count >= 0)
        {
            skip(count + 1);
            return count;
        }
        skip(15);

        for (int32_t high_bits_count = 15;; ++high_bits_count)
        {
            if (read_bit())
                return high_bits_count;
        }
    }

    int32_t read_long_value(const int32_t length)
    {
        if (length <= 24)
            return read_value(length);

        return (read_value(length - 24) << 24) + read_value(24);
    }

protected:
    frame_info frame_info_;
    coding_parameters parameters_;
    std::unique_ptr<process_line> processLine_;

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
