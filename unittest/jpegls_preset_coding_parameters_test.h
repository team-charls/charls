// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

namespace charls::test {

struct thresholds final
{
    int32_t max_value;
    int32_t t1;
    int32_t t2;
    int32_t t3;
    int32_t reset;
};

// Threshold function of JPEG-LS reference implementation.
constexpr thresholds compute_defaults_using_reference_implementation(const int32_t max_value, const uint16_t near) noexcept
{
    thresholds result{max_value, 0, 0, 0, 64};

    if (result.max_value >= 128)
    {
        int32_t factor{result.max_value};
        if (factor > 4095)
            factor = 4095;
        factor = (factor + 128) >> 8;
        result.t1 = factor * (3 - 2) + 2 + 3 * near;
        if (result.t1 > result.max_value || result.t1 < near + 1)
            result.t1 = near + 1;
        result.t2 = factor * (7 - 3) + 3 + 5 * near;
        if (result.t2 > result.max_value || result.t2 < result.t1)
            result.t2 = result.t1;
        result.t3 = factor * (21 - 4) + 4 + 7 * near;
        if (result.t3 > result.max_value || result.t3 < result.t2)
            result.t3 = result.t2;
    }
    else
    {
        const int32_t factor{256 / (result.max_value + 1)};
        result.t1 = 3 / factor + 3 * near;
        if (result.t1 < 2)
            result.t1 = 2;
        if (result.t1 > result.max_value || result.t1 < near + 1)
            result.t1 = near + 1;
        result.t2 = 7 / factor + 5 * near;
        if (result.t2 < 3)
            result.t2 = 3;
        if (result.t2 > result.max_value || result.t2 < result.t1)
            result.t2 = result.t1;
        result.t3 = 21 / factor + 7 * near;
        if (result.t3 < 4)
            result.t3 = 4;
        if (result.t3 > result.max_value || result.t3 < result.t2)
            result.t3 = result.t2;
    }

    return result;
}

} // namespace charls::test
