// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_algorithm.h"
#include "quantization_lut.h"

#include <array>

namespace charls {

// Used to determine how large runs should be encoded at a time. Defined by the JPEG-LS standard, A.2.1., Initialization
// step 3.
constexpr std::array<int, 32> J{
    {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15}};


// C4127 = conditional expression is constant (caused by some template methods that are not fully specialized) [VS2017]
// C6326 = Potential comparison of a constant with another constant. (false warning, triggered by template construction
// in Checked build)
// C26814 = The const variable 'RANGE' can be computed at compile-time. [incorrect warning, VS 16.3.0 P3]
MSVC_WARNING_SUPPRESS(4127 6326 26814)

template<typename Traits>
const int8_t* initialize_quantization_lut(Traits traits, const int32_t threshold1, const int32_t threshold2,
                                          const int32_t threshold3, std::vector<int8_t>& quantization_lut)
{
    // for lossless mode with default parameters, we have precomputed the look up table for bit counts 8, 10, 12 and 16.
    if (traits.near_lossless == 0 && traits.maximum_sample_value == (1 << traits.bits_per_pixel) - 1)
    {
        if (const jpegls_pc_parameters presets{compute_default(traits.maximum_sample_value, traits.near_lossless)};
            presets.threshold1 == threshold1 && presets.threshold2 == threshold2 && presets.threshold3 == threshold3)
        {
            if (traits.bits_per_pixel == 8)
            {
                return &quantization_lut_lossless_8[quantization_lut_lossless_8.size() / 2];
            }
            if (traits.bits_per_pixel == 10)
            {
                return &quantization_lut_lossless_10[quantization_lut_lossless_10.size() / 2];
            }
            if (traits.bits_per_pixel == 12)
            {
                return &quantization_lut_lossless_12[quantization_lut_lossless_12.size() / 2];
            }
            if (traits.bits_per_pixel == 16)
            {
                return &quantization_lut_lossless_16[quantization_lut_lossless_16.size() / 2];
            }
        }
    }

    // Initialize the quantization lookup table dynamic.
    const int32_t range{1 << traits.bits_per_pixel};
    quantization_lut.resize(static_cast<size_t>(range) * 2);
    for (size_t i{}; i < quantization_lut.size(); ++i)
    {
        quantization_lut[i] =
            quantize_gradient_org(-range + static_cast<int32_t>(i), threshold1, threshold2, threshold3, traits.near_lossless);
    }

    return &quantization_lut[range];
}
MSVC_WARNING_UNSUPPRESS()


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
        return charls::quantize_gradient_org(di, t1_, t2_, t3_, near_lossless);
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

    // Quantization lookup table
    const int8_t* quantization_{};
    std::vector<int8_t> quantization_lut_;
};

} // namespace charls
