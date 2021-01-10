// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/jpegls_error.h>

#include "byte_span.h"
#include "jpeg_marker_code.h"
#include "util.h"

namespace charls {

// Purpose: 'Writer' class that can generate JPEG-LS file streams.
class jpeg_stream_writer final
{
public:
    jpeg_stream_writer() = default;
    explicit jpeg_stream_writer(byte_span destination) noexcept;
    ~jpeg_stream_writer() = default;

    jpeg_stream_writer(const jpeg_stream_writer&) = delete;
    jpeg_stream_writer& operator=(const jpeg_stream_writer&) = delete;
    jpeg_stream_writer(jpeg_stream_writer&&) = default;
    jpeg_stream_writer& operator=(jpeg_stream_writer&&) = default;

    void write_start_of_image();

    /// <summary>
    /// Write a JPEG SPIFF (APP8 + spiff) segment.
    /// This segment is documented in ISO/IEC 10918-3, Annex F.
    /// </summary>
    /// <param name="header">Header info to write into the SPIFF segment.</param>
    void write_spiff_header_segment(const spiff_header& header);

    void write_spiff_directory_entry(uint32_t entry_tag, IN_READS_BYTES_(entry_data_size_bytes) const void* entry_data,
                                     size_t entry_data_size_bytes);

    /// <summary>
    /// Write a JPEG SPIFF end of directory (APP8) segment.
    /// This segment is documented in ISO/IEC 10918-3, Annex F.
    /// </summary>
    void write_spiff_end_of_directory_entry();

    /// <summary>
    /// Writes a HP color transformation (APP8) segment.
    /// </summary>
    /// <param name="transformation">Color transformation to put into the segment.</param>
    void write_color_transform_segment(color_transformation transformation);

    /// <summary>
    /// Writes a JPEG-LS preset parameters (LSE) segment.
    /// </summary>
    /// <param name="preset_coding_parameters">Parameters to write into the JPEG-LS preset segment.</param>
    void write_jpegls_preset_parameters_segment(const jpegls_pc_parameters& preset_coding_parameters);

    /// <summary>
    /// Writes a JPEG-LS Start Of Frame (SOF-55) segment.
    /// </summary>
    /// <param name="width">The width of the frame.</param>
    /// <param name="height">The height of the frame.</param>
    /// <param name="bits_per_sample">The bits per sample.</param>
    /// <param name="component_count">The component count.</param>
    void write_start_of_frame_segment(uint32_t width, uint32_t height, int32_t bits_per_sample, int32_t component_count);

    /// <summary>
    /// Writes a JPEG-LS Start Of Scan (SOS) segment.
    /// </summary>
    /// <param name="component_count">
    /// The number of components in the scan segment. Can only be > 1 when the components are interleaved.
    /// </param>
    /// <param name="near_lossless">The allowed lossy error. 0 means lossless.</param>
    /// <param name="interleave_mode">The interleave mode of the components.</param>
    void write_start_of_scan_segment(int32_t component_count, int32_t near_lossless, interleave_mode interleave_mode);

    void write_end_of_image();

    size_t bytes_written() const noexcept
    {
        return byte_offset_;
    }

    byte_span remaining_destination() const noexcept
    {
        return {destination_.data + byte_offset_, destination_.size - byte_offset_};
    }

    void seek(const size_t byte_count) noexcept
    {
        ASSERT(byte_offset_ + byte_count <= destination_.size);
        byte_offset_ += byte_count;
    }

    void destination(const byte_span destination) noexcept
    {
        destination_ = destination;
    }

private:
    void write_segment_header(jpeg_marker_code marker_code, size_t data_size);

    void write_uint8(const uint8_t value) noexcept
    {
        ASSERT(byte_offset_ + sizeof(uint8_t) <= destination_.size);
        destination_.data[byte_offset_++] = value;
    }

    void write_uint16(const uint16_t value) noexcept
    {
        write_uint<uint16_t>(value);
    }

    void write_uint16(const int32_t value) noexcept
    {
        ASSERT(value >= 0 && value <= UINT16_MAX);
        write_uint16(static_cast<uint16_t>(value));
    }

    void write_uint32(const uint32_t value) noexcept
    {
        write_uint<uint32_t>(value);
    }

    template<typename UnsignedIntType>
    void write_uint(const UnsignedIntType value) noexcept
    {
        ASSERT(byte_offset_ + sizeof(UnsignedIntType) <= destination_.size);

        // Use write_bytes to write to the unaligned byte array.
        // The compiler will perform the correct optimization when the target platform support unaligned writes.
        const UnsignedIntType big_endian_value{endian_swap(value)};
        write_bytes(&big_endian_value, sizeof big_endian_value);
    }

    void write_bytes(IN_READS_BYTES_(size) const void* data, const size_t size) noexcept
    {
        ASSERT(byte_offset_ + size <= destination_.size);
        memcpy(destination_.data + byte_offset_, data, size);
        byte_offset_ += size;
    }

    void write_marker(const jpeg_marker_code marker_code) noexcept
    {
        write_uint8(jpeg_marker_start_byte);
        write_uint8(static_cast<uint8_t>(marker_code));
    }

    void write_segment_without_data(const jpeg_marker_code marker_code)
    {
        if (byte_offset_ + 2 > destination_.size)
            impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

        write_uint8(jpeg_marker_start_byte);
        write_uint8(static_cast<uint8_t>(marker_code));
    }

    static constexpr uint32_t endian_swap(const uint32_t value) noexcept
    {
        return value >> 24 | (value & 0x00FF0000) >> 8 | (value & 0x0000FF00) << 8 | value << 24;
    }

    static constexpr uint16_t endian_swap(const uint16_t value) noexcept
    {
        return static_cast<uint16_t>(value >> 8 | value << 8);
    }

    byte_span destination_{};
    size_t byte_offset_{};
    uint8_t component_id_{1};
};

} // namespace charls
