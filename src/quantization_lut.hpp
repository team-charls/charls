// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_algorithm.hpp"
#include "jpegls_preset_coding_parameters.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace charls {

template<int32_t BitsPerSample>
constexpr auto create_quantization_lut_lossless() noexcept
{
    constexpr int32_t max_val{calculate_maximum_sample_value(BitsPerSample)};
    constexpr auto preset{compute_default(max_val, 0)};
    constexpr int32_t range{preset.maximum_sample_value + 1};

    std::array<int8_t, static_cast<size_t>(range) * 2> lut{};
    for (size_t i{}; i != lut.size(); ++i)
    {
        lut[i] = quantize_gradient_org(static_cast<int32_t>(i) - range, preset.threshold1, preset.threshold2,
                                       preset.threshold3);
    }

    return lut;
}

// Compile-time computed lookup tables for 8, 10 and 12-bit (up to 8 KB each).
inline constexpr auto quantization_lut_lossless_8{create_quantization_lut_lossless<8>()};
inline constexpr auto quantization_lut_lossless_10{create_quantization_lut_lossless<10>()};
inline constexpr auto quantization_lut_lossless_12{create_quantization_lut_lossless<12>()};

// The 16-bit table (128 KB) exceeds typical constexpr evaluation limits.
// Use lazy initialization to defer allocation until first use.
const std::vector<int8_t>& quantization_lut_lossless_16();

} // namespace charls
