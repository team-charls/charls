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

template<typename sample, typename pixel>
struct DefaultTraits final
{
    using SAMPLE = sample;
    using PIXEL = pixel;

    int32_t MAXVAL;
    const int32_t RANGE;
    const int32_t NEAR;
    const int32_t qbpp;
    const int32_t bpp;
    const int32_t LIMIT;
    const int32_t RESET;

    DefaultTraits(const int32_t max, const int32_t near, const int32_t reset = DefaultResetValue) noexcept :
        MAXVAL{max},
        RANGE{(max + 2 * near) / (2 * near + 1) + 1},
        NEAR{near},
        qbpp{log_2(RANGE)},
        bpp{log_2(max)},
        LIMIT{2 * (bpp + std::max(8, bpp))},
        RESET{reset}
    {
    }

    DefaultTraits(const DefaultTraits& other) noexcept :
        MAXVAL{other.MAXVAL},
        RANGE{other.RANGE},
        NEAR{other.NEAR},
        qbpp{other.qbpp},
        bpp{other.bpp},
        LIMIT{other.LIMIT},
        RESET{other.RESET}
    {
    }

    DefaultTraits() = delete;
    DefaultTraits(DefaultTraits&&) noexcept = default;
    ~DefaultTraits() = default;
    DefaultTraits& operator=(const DefaultTraits&) = delete;
    DefaultTraits& operator=(DefaultTraits&&) = delete;

    FORCE_INLINE int32_t ComputeErrVal(const int32_t e) const noexcept
    {
        return ModuloRange(Quantize(e));
    }

    FORCE_INLINE SAMPLE ComputeReconstructedSample(const int32_t Px, const int32_t ErrVal) const noexcept
    {
        return FixReconstructedValue(Px + DeQuantize(ErrVal));
    }

    FORCE_INLINE bool IsNear(const int32_t lhs, const int32_t rhs) const noexcept
    {
        return std::abs(lhs - rhs) <= NEAR;
    }

    bool IsNear(const Triplet<SAMPLE> lhs, const Triplet<SAMPLE> rhs) const noexcept
    {
        return std::abs(lhs.v1 - rhs.v1) <= NEAR &&
               std::abs(lhs.v2 - rhs.v2) <= NEAR &&
               std::abs(lhs.v3 - rhs.v3) <= NEAR;
    }

    bool IsNear(const Quad<SAMPLE> lhs, const Quad<SAMPLE> rhs) const noexcept
    {
        return std::abs(lhs.v1 - rhs.v1) <= NEAR &&
               std::abs(lhs.v2 - rhs.v2) <= NEAR &&
               std::abs(lhs.v3 - rhs.v3) <= NEAR &&
               std::abs(lhs.v4 - rhs.v4) <= NEAR;
    }

    FORCE_INLINE int32_t CorrectPrediction(const int32_t Pxc) const noexcept
    {
        if ((Pxc & MAXVAL) == Pxc)
            return Pxc;

        return (~(Pxc >> (int32_t_bit_count - 1))) & MAXVAL;
    }

    /// <summary>
    /// Returns the value of errorValue modulo RANGE. ITU.T.87, A.4.5 (code segment A.9)
    /// This ensures the error is reduced to the range (-⌊RANGE/2⌋ .. ⌈RANGE/2⌉-1)
    /// </summary>
    FORCE_INLINE int32_t ModuloRange(int32_t errorValue) const noexcept
    {
        ASSERT(std::abs(errorValue) <= RANGE);

        if (errorValue < 0)
        {
            errorValue += RANGE;
        }

        if (errorValue >= (RANGE + 1) / 2)
        {
            errorValue -= RANGE;
        }

        ASSERT(-RANGE / 2 <= errorValue && errorValue <= ((RANGE + 1) / 2) - 1);
        return errorValue;
    }

private:
    int32_t Quantize(const int32_t errorValue) const noexcept
    {
        if (errorValue > 0)
            return (errorValue + NEAR) / (2 * NEAR + 1);

        return -(NEAR - errorValue) / (2 * NEAR + 1);
    }

    FORCE_INLINE int32_t DeQuantize(const int32_t ErrorValue) const noexcept
    {
        return ErrorValue * (2 * NEAR + 1);
    }

    FORCE_INLINE SAMPLE FixReconstructedValue(int32_t value) const noexcept
    {
        if (value < -NEAR)
        {
            value = value + RANGE * (2 * NEAR + 1);
        }
        else if (value > MAXVAL + NEAR)
        {
            value = value - RANGE * (2 * NEAR + 1);
        }

        return static_cast<SAMPLE>(CorrectPrediction(value));
    }
};

} // namespace charls
