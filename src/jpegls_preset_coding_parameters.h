// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/public_types.h>

#include "constants.h"
#include "util.h"

#include <algorithm>
#include <cassert>

namespace charls {

/// <summary>Clamping function as defined by ISO/IEC 14495-1, Figure C.3</summary>
inline int32_t clamp(const int32_t i, const int32_t j, const int32_t maximum_sample_value) noexcept
{
    if (i > maximum_sample_value || i < j)
        return j;

    return i;
}

/// <summary>Default coding threshold values as defined by ISO/IEC 14495-1, C.2.4.1.1.1</summary>
inline jpegls_pc_parameters compute_default(const int32_t maximum_sample_value, const int32_t near_lossless) noexcept
{
    ASSERT(maximum_sample_value <= UINT16_MAX);
    ASSERT(near_lossless >= 0 && near_lossless <= compute_maximum_near_lossless(maximum_sample_value));

    if (maximum_sample_value >= 128)
    {
        const int32_t factor{(std::min(maximum_sample_value, 4095) + 128) / 256};
        const int threshold1{
            clamp(factor * (default_threshold1 - 2) + 2 + 3 * near_lossless, near_lossless + 1, maximum_sample_value)};
        const int threshold2{
            clamp(factor * (default_threshold2 - 3) + 3 + 5 * near_lossless, threshold1, maximum_sample_value)};

        return {maximum_sample_value, threshold1, threshold2,
                clamp(factor * (default_threshold3 - 4) + 4 + 7 * near_lossless, threshold2, maximum_sample_value),
                default_reset_value};
    }

    const int32_t factor{256 / (maximum_sample_value + 1)};
    const int threshold1{
        clamp(std::max(2, default_threshold1 / factor + 3 * near_lossless), near_lossless + 1, maximum_sample_value)};
    const int threshold2{
        clamp(std::max(3, default_threshold2 / factor + 5 * near_lossless), threshold1, maximum_sample_value)};

    return {maximum_sample_value, threshold1, threshold2,
            clamp(std::max(4, default_threshold3 / factor + 7 * near_lossless), threshold2, maximum_sample_value),
            default_reset_value};
}


inline bool is_default(const jpegls_pc_parameters& preset_coding_parameters) noexcept
{
    if (preset_coding_parameters.maximum_sample_value != 0)
        return false;

    if (preset_coding_parameters.threshold1 != 0)
        return false;

    if (preset_coding_parameters.threshold2 != 0)
        return false;

    if (preset_coding_parameters.threshold3 != 0)
        return false;

    if (preset_coding_parameters.reset_value != 0)
        return false;

    return true;
}


inline bool is_valid(const jpegls_pc_parameters& pc_parameters, const int32_t maximum_component_value,
                     const int32_t near_lossless) noexcept
{
    ASSERT(maximum_component_value <= UINT16_MAX);

    // ISO/IEC 14495-1, C.2.4.1.1, Table C.1 defines the valid JPEG-LS preset coding parameters values.
    if (pc_parameters.maximum_sample_value != 0 &&
        (pc_parameters.maximum_sample_value < 1 || pc_parameters.maximum_sample_value > maximum_component_value))
        return false;

    const int32_t maximum_sample_value{pc_parameters.maximum_sample_value != 0 ? pc_parameters.maximum_sample_value
                                                                               : maximum_component_value};
    if (pc_parameters.threshold1 != 0 &&
        (pc_parameters.threshold1 < near_lossless + 1 || pc_parameters.threshold1 > maximum_sample_value))
        return false;

    const jpegls_pc_parameters default_parameters{compute_default(maximum_sample_value, near_lossless)};
    const int32_t threshold1{pc_parameters.threshold1 != 0 ? pc_parameters.threshold1 : default_parameters.threshold1};
    if (pc_parameters.threshold2 != 0 &&
        (pc_parameters.threshold2 < threshold1 || pc_parameters.threshold2 > maximum_sample_value))
        return false;

    const int32_t threshold2{pc_parameters.threshold2 != 0 ? pc_parameters.threshold2 : default_parameters.threshold2};
    if (pc_parameters.threshold3 != 0 &&
        (pc_parameters.threshold3 < threshold2 || pc_parameters.threshold3 > maximum_sample_value))
        return false;

    if (pc_parameters.reset_value != 0 &&
        (pc_parameters.reset_value < 3 || pc_parameters.reset_value > std::max(255, maximum_sample_value)))
        return false;

    return true;
}

} // namespace charls
