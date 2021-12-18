// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "byte_span.h"
#include "charls/public_types.h"
#include "coding_parameters.h"

#include <cstdint>
#include <vector>

namespace charls {

enum class jpeg_marker_code : uint8_t;

// Purpose: minimal implementation to read a JPEG byte stream.
class jpeg_stream_reader final
{
public:
    jpeg_stream_reader() = default;
    ~jpeg_stream_reader() = default;

    jpeg_stream_reader(const jpeg_stream_reader&) = delete;
    jpeg_stream_reader& operator=(const jpeg_stream_reader&) = delete;
    jpeg_stream_reader(jpeg_stream_reader&&) = default;
    jpeg_stream_reader& operator=(jpeg_stream_reader&&) = default;

    void source(byte_span source) noexcept;

    const charls::frame_info& frame_info() const noexcept
    {
        return frame_info_;
    }

    const coding_parameters& parameters() const noexcept
    {
        return parameters_;
    }

    const jpegls_pc_parameters& preset_coding_parameters() const noexcept
    {
        return preset_coding_parameters_;
    }

    void read(byte_span destination, size_t stride);
    void read_header(spiff_header* header = nullptr, bool* spiff_header_found = nullptr);

    void output_bgr(const bool value) noexcept
    {
        parameters_.output_bgr = value;
    }

    void rect(const JlsRect& rect) noexcept
    {
        rect_ = rect;
    }

    void at_comment(const at_comment_handler handler, void* user_context) noexcept
    {
        comment_handler_ = handler;
        comment_handler_user_context_ = user_context;
    }

    void read_start_of_scan();
    uint8_t read_byte();

private:
    void skip_byte();
    uint16_t read_uint16();
    uint32_t read_uint24();
    uint32_t read_uint32();
    int32_t read_segment_size();
    std::vector<uint8_t> read_bytes(size_t byte_count);
    void read_next_start_of_scan();
    jpeg_marker_code read_next_marker_code();
    void validate_marker_code(jpeg_marker_code marker_code) const;
    jpegls_pc_parameters get_validated_preset_coding_parameters() const;

    int read_marker_segment(jpeg_marker_code marker_code, int32_t segment_size, spiff_header* header = nullptr,
                            bool* spiff_header_found = nullptr);
    int read_spiff_directory_entry(jpeg_marker_code marker_code, int32_t segment_size);
    int read_start_of_frame_segment(int32_t segment_size);
    int read_comment(int32_t segment_size) const;
    int read_preset_parameters_segment(int32_t segment_size);
    int read_define_restart_interval(int32_t segment_size);
    int try_read_application_data8_segment(int32_t segment_size, spiff_header* header, bool* spiff_header_found);
    int try_read_spiff_header_segment(OUT_ spiff_header& header, OUT_ bool& spiff_header_found);

    int try_read_hp_color_transform_segment();
    void add_component(uint8_t component_id);
    void check_parameter_coherent() const;
    uint32_t maximum_sample_value() const noexcept;

    enum class state
    {
        before_start_of_image,
        header_section,
        spiff_header_section,
        image_section,
        frame_section,
        scan_section,
        bit_stream_section
    };

    byte_span source_;
    charls::frame_info frame_info_{};
    coding_parameters parameters_{};
    jpegls_pc_parameters preset_coding_parameters_{};
    JlsRect rect_{};
    std::vector<uint8_t> component_ids_;
    state state_{};

    at_comment_handler comment_handler_{};
    void* comment_handler_user_context_{};
};

} // namespace charls
