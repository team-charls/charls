// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <array>

namespace charls {

// Used to determine how large runs should be encoded at a time. Defined by the JPEG-LS standard, A.2.1., Initialization
// step 3.
constexpr std::array<int, 32> J{
    {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15}};

/// <summary>
/// Base class for scan_encoder and scan_decoder
/// Contains the variables and methods that are identical for the encoding/decoding process and can be shared.
/// </summary>
class scan_codec
{
public:
    scan_codec(const scan_codec&) = delete;
    scan_codec(scan_codec&&) = delete;
    scan_codec& operator=(const scan_codec&) = delete;
    scan_codec& operator=(scan_codec&&) = delete;

protected:
    /// <remarks>
    /// Copy frame_info and parameters to prevent 1 indirection during encoding/decoding.
    /// </remarks>
    scan_codec(const frame_info& frame_info, const coding_parameters& parameters) noexcept :
        frame_info_{frame_info}, parameters_{parameters}
    {
    }

    ~scan_codec() = default;

    [[nodiscard]] int8_t quantize_gradient_org(const int32_t di, const int32_t near_lossless) const noexcept
    {
        if (di <= -t3_)
            return -4;
        if (di <= -t2_)
            return -3;
        if (di <= -t1_)
            return -2;
        if (di < -near_lossless)
            return -1;
        if (di <= near_lossless)
            return 0;
        if (di < t1_)
            return 1;
        if (di < t2_)
            return 2;
        if (di < t3_)
            return 3;

        return 4;
    }

    [[nodiscard]] const coding_parameters& parameters() const noexcept
    {
        return parameters_;
    }

    [[nodiscard]] const charls::frame_info& frame_info() const noexcept
    {
        return frame_info_;
    }

    [[nodiscard]] bool is_interleaved() const noexcept
    {
        ASSERT((parameters().interleave_mode == interleave_mode::none && frame_info().component_count == 1) ||
               parameters().interleave_mode != interleave_mode::none);

        return parameters().interleave_mode != interleave_mode::none;
    }

    charls::frame_info frame_info_;
    coding_parameters parameters_;
    int32_t t1_{};
    int32_t t2_{};
    int32_t t3_{};
};

} // namespace charls
