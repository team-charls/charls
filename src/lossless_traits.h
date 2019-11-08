// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.h"
#include <cstdint>

namespace charls {

// Optimized trait classes for lossless compression of 8 bit color and 8/16 bit monochrome images.
// This class assumes MaximumSampleValue correspond to a whole number of bits, and no custom ResetValue is set when encoding.
// The point of this is to have the most optimized code for the most common and most demanding scenario.
template<typename sample, int32_t bitsPerPixel>
struct LosslessTraitsImpl
{
    using SAMPLE = sample;

    enum
    {
        NEAR = 0,
        bpp = bitsPerPixel,
        qbpp = bitsPerPixel,
        RANGE = (1 << bpp),
        MAXVAL = (1 << bpp) - 1,
        LIMIT = 2 * (bitsPerPixel + std::max(8, bitsPerPixel)),
        RESET = DefaultResetValue
    };

    FORCE_INLINE constexpr static int32_t ComputeErrVal(int32_t d) noexcept
    {
        return ModuloRange(d);
    }

    FORCE_INLINE constexpr static bool IsNear(int32_t lhs, int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

// The following optimization is implementation-dependent (works on x86 and ARM, see charlstest).
#if defined(__clang__)
    __attribute__((no_sanitize("shift")))
#endif
    FORCE_INLINE constexpr static int32_t
    ModuloRange(int32_t errorValue) noexcept
    {
        return static_cast<int32_t>(errorValue << (int32_t_bit_count - bpp)) >> (int32_t_bit_count - bpp);
    }

    FORCE_INLINE static SAMPLE ComputeReconstructedSample(int32_t Px, int32_t ErrVal) noexcept
    {
        return static_cast<SAMPLE>(MAXVAL & (Px + ErrVal));
    }

    FORCE_INLINE static int32_t CorrectPrediction(int32_t Pxc) noexcept
    {
        if ((Pxc & MAXVAL) == Pxc)
            return Pxc;

        return (~(Pxc >> (int32_t_bit_count - 1))) & MAXVAL;
    }
};


template<typename T, int32_t bpp>
struct LosslessTraits final : LosslessTraitsImpl<T, bpp>
{
    using PIXEL = T;
};


template<>
struct LosslessTraits<uint8_t, 8> final : LosslessTraitsImpl<uint8_t, 8>
{
    using PIXEL = SAMPLE;

    FORCE_INLINE constexpr static signed char ModRange(int32_t errorValue) noexcept
    {
        return static_cast<signed char>(errorValue);
    }

    FORCE_INLINE constexpr static int32_t ComputeErrVal(int32_t d) noexcept
    {
        return static_cast<signed char>(d);
    }

    FORCE_INLINE constexpr static uint8_t ComputeReconstructedSample(int32_t Px, int32_t ErrVal) noexcept
    {
        return static_cast<uint8_t>(Px + ErrVal);
    }
};


template<>
struct LosslessTraits<uint16_t, 16> final : LosslessTraitsImpl<uint16_t, 16>
{
    using PIXEL = SAMPLE;

    FORCE_INLINE constexpr static short ModRange(int32_t errorValue) noexcept
    {
        return static_cast<short>(errorValue);
    }

    FORCE_INLINE constexpr static int32_t ComputeErrVal(int32_t d) noexcept
    {
        return static_cast<short>(d);
    }

    FORCE_INLINE constexpr static SAMPLE ComputeReconstructedSample(int32_t Px, int32_t errorValue) noexcept
    {
        return static_cast<SAMPLE>(Px + errorValue);
    }
};


template<typename T, int32_t bpp>
struct LosslessTraits<Triplet<T>, bpp> final : LosslessTraitsImpl<T, bpp>
{
    using PIXEL = Triplet<T>;

    FORCE_INLINE constexpr static bool IsNear(int32_t lhs, int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static bool IsNear(PIXEL lhs, PIXEL rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static T ComputeReconstructedSample(int32_t Px, int32_t errorValue) noexcept
    {
        return static_cast<T>(Px + errorValue);
    }
};


template<typename T, int32_t bpp>
struct LosslessTraits<Quad<T>, bpp> final : LosslessTraitsImpl<T, bpp>
{
    using PIXEL = Quad<T>;

    FORCE_INLINE constexpr static bool IsNear(int32_t lhs, int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static bool IsNear(PIXEL lhs, PIXEL rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static T ComputeReconstructedSample(int32_t Px, int32_t errorValue) noexcept
    {
        return static_cast<T>(Px + errorValue);
    }
};

} // namespace charls
