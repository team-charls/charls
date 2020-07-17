// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.h"
#include "util.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>

// Default traits that support all JPEG LS parameters: custom limit, near, maxval (not power of 2)

// This traits class is used to initialize a coder/decoder.
// The coder/decoder also delegates some functions to the traits class.
// This is to allow the traits class to replace the default implementation here with optimized specific implementations.
// This is done for lossless coding/decoding: see losslesstraits.h

namespace charls {

template<typename Sample, typename Pixel>
struct default_traits final
{
    using SAMPLE = Sample;
    using PIXEL = Pixel;

    int32_t MAXVAL;
    const int32_t RANGE;
    const int32_t NEAR;
    const int32_t qbpp;
    const int32_t bpp;
    const int32_t LIMIT;
    const int32_t RESET;

    default_traits(const int32_t max, const int32_t near, const int32_t reset = default_reset_value) noexcept :
        MAXVAL{max},
        RANGE{(max + 2 * near) / (2 * near + 1) + 1},
        NEAR{near},
        qbpp{log_2(RANGE)},
        bpp{log_2(max)},
        LIMIT{2 * (bpp + std::max(8, bpp))},
        RESET{reset}
    {
    }

    default_traits(const default_traits& other) noexcept :
        MAXVAL{other.MAXVAL},
        RANGE{other.RANGE},
        NEAR{other.NEAR},
        qbpp{other.qbpp},
        bpp{other.bpp},
        LIMIT{other.LIMIT},
        RESET{other.RESET}
    {
    }

    default_traits() = delete;
    default_traits(default_traits&&) noexcept = default;
    ~default_traits() = default;
    default_traits& operator=(const default_traits&) = delete;
    default_traits& operator=(default_traits&&) = delete;

    FORCE_INLINE int32_t compute_error_value(const int32_t e) const noexcept
    {
        return modulo_range(quantize(e));
    }

    FORCE_INLINE SAMPLE compute_reconstructed_sample(const int32_t predicted_value, const int32_t error_value) const noexcept
    {
        return fix_reconstructed_value(predicted_value + dequantize(error_value));
    }

    FORCE_INLINE bool is_near(const int32_t lhs, const int32_t rhs) const noexcept
    {
        return std::abs(lhs - rhs) <= NEAR;
    }

    bool is_near(const triplet<SAMPLE> lhs, const triplet<SAMPLE> rhs) const noexcept
    {
        return std::abs(lhs.v1 - rhs.v1) <= NEAR &&
               std::abs(lhs.v2 - rhs.v2) <= NEAR &&
               std::abs(lhs.v3 - rhs.v3) <= NEAR;
    }

    bool is_near(const quad<SAMPLE> lhs, const quad<SAMPLE> rhs) const noexcept
    {
        return std::abs(lhs.v1 - rhs.v1) <= NEAR &&
               std::abs(lhs.v2 - rhs.v2) <= NEAR &&
               std::abs(lhs.v3 - rhs.v3) <= NEAR &&
               std::abs(lhs.v4 - rhs.v4) <= NEAR;
    }

    FORCE_INLINE int32_t correct_prediction(const int32_t predicted) const noexcept
    {
        if ((predicted & MAXVAL) == predicted)
            return predicted;

        return (~(predicted >> (int32_t_bit_count - 1))) & MAXVAL;
    }

    /// <summary>
    /// Returns the value of errorValue modulo RANGE. ITU.T.87, A.4.5 (code segment A.9)
    /// This ensures the error is reduced to the range (-⌊RANGE/2⌋ .. ⌈RANGE/2⌉-1)
    /// </summary>
    FORCE_INLINE int32_t modulo_range(int32_t error_value) const noexcept
    {
        ASSERT(std::abs(error_value) <= RANGE);

        if (error_value < 0)
        {
            error_value += RANGE;
        }

        if (error_value >= (RANGE + 1) / 2)
        {
            error_value -= RANGE;
        }

        ASSERT(-RANGE / 2 <= error_value && error_value <= ((RANGE + 1) / 2) - 1);
        return error_value;
    }

private:
    int32_t quantize(const int32_t error_value) const noexcept
    {
        if (error_value > 0)
            return (error_value + NEAR) / (2 * NEAR + 1);

        return -(NEAR - error_value) / (2 * NEAR + 1);
    }

    FORCE_INLINE int32_t dequantize(const int32_t error_value) const noexcept
    {
        return error_value * (2 * NEAR + 1);
    }

    FORCE_INLINE SAMPLE fix_reconstructed_value(int32_t value) const noexcept
    {
        if (value < -NEAR)
        {
            value = value + RANGE * (2 * NEAR + 1);
        }
        else if (value > MAXVAL + NEAR)
        {
            value = value - RANGE * (2 * NEAR + 1);
        }

        return static_cast<SAMPLE>(correct_prediction(value));
    }
};

} // namespace charls
