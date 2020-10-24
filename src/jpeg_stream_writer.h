// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/jpegls_error.h>

#include "jpeg_marker_code.h"
#include "byte_span.h"

#include <vector>

namespace charls {

// Purpose: 'Writer' class that can generate JPEG-LS file streams.
class jpeg_stream_writer final
{
public:
    jpeg_stream_writer() = default;
    explicit jpeg_stream_writer(const byte_span& destination) noexcept;
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

    void write_spiff_directory_entry(uint32_t entry_tag,
                                     IN_READS_BYTES_(entry_data_size_bytes) const void* entry_data,
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
    void write_start_of_frame_segment(uint32_t width, uint32_t height, int bits_per_sample, int component_count);

    /// <summary>
    /// Writes a JPEG-LS Start Of Scan (SOS) segment.
    /// </summary>
    /// <param name="component_count">The number of components in the scan segment. Can only be > 1 when the components are interleaved.</param>
    /// <param name="near_lossless">The allowed lossy error. 0 means lossless.</param>
    /// <param name="interleave_mode">The interleave mode of the components.</param>
    void write_start_of_scan_segment(int component_count, int near_lossless, interleave_mode interleave_mode);

    void write_end_of_image();

    std::size_t bytes_written() const noexcept
    {
        return byte_offset_;
    }

    std::size_t get_length() const noexcept
    {
        return destination_.count - byte_offset_;
    }

    byte_span output_stream() const noexcept
    {
        byte_span data = destination_;
        data.count -= byte_offset_;
        data.rawData += byte_offset_;
        return data;
    }

    void seek(const std::size_t byte_count) noexcept
    {
        byte_offset_ += byte_count;
    }

    void update_destination(OUT_WRITES_BYTES_(destination_size) void* destination_buffer,
                            const size_t destination_size) noexcept
    {
        destination_.rawData = static_cast<uint8_t*>(destination_buffer);
        destination_.count = destination_size;
    }

private:
    uint8_t* get_pos() const noexcept
    {
        return destination_.rawData + byte_offset_;
    }

    void write_segment(jpeg_marker_code marker_code, IN_READS_BYTES_(size) const void* data, size_t size);

    void write_byte(const uint8_t value)
    {
        if (byte_offset_ >= destination_.count)
            impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

        destination_.rawData[byte_offset_++] = value;
    }

    void write_bytes(const std::vector<uint8_t>& bytes)
    {
        for (const auto value : bytes)
        {
            write_byte(value);
        }
    }

    void write_bytes(IN_READS_BYTES_(size) const void* data, const size_t size)
    {
        const auto* bytes = static_cast<const uint8_t*>(data);

        for (std::size_t i = 0; i < size; ++i)
        {
            write_byte(bytes[i]);
        }
    }

    void write_uint16(const uint16_t value)
    {
        write_byte(static_cast<uint8_t>(value / 0x100));
        write_byte(static_cast<uint8_t>(value % 0x100));
    }

    void write_uint32(const uint32_t value)
    {
        write_byte(static_cast<uint8_t>(value >> 24));
        write_byte(static_cast<uint8_t>(value >> 16));
        write_byte(static_cast<uint8_t>(value >> 8));
        write_byte(static_cast<uint8_t>(value));
    }

    void write_marker(const jpeg_marker_code marker_code)
    {
        write_byte(jpeg_marker_start_byte);
        write_byte(static_cast<uint8_t>(marker_code));
    }

    byte_span destination_{};
    std::size_t byte_offset_{};
    int8_t component_id_{1};
};

} // namespace charls
