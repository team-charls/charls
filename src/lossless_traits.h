// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.h"
#include "util.h"

#include <cstdint>

namespace charls {

// Optimized trait classes for lossless compression of 8 bit color and 8/16 bit monochrome images.
// This class assumes MaximumSampleValue correspond to a whole number of bits, and no custom ResetValue is set when encoding.
// The point of this is to have the most optimized code for the most common and most demanding scenario.
template<typename Sample, int32_t bitsPerPixel>
struct lossless_traits_impl
{
    using SAMPLE = Sample;

    enum
    {
        NEAR = 0,
        bpp = bitsPerPixel,
        qbpp = bitsPerPixel,
        RANGE = (1U << bpp),
        MAXVAL = (1U << bpp) - 1,
        LIMIT = 2 * (bitsPerPixel + std::max(8, bitsPerPixel)),
        RESET = default_reset_value
    };

    FORCE_INLINE constexpr static int32_t compute_error_value(const int32_t d) noexcept
    {
        return modulo_range(d);
    }

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

// The following optimization is implementation-dependent (works on x86 and ARM, see charlstest).
#if defined(__clang__)
    __attribute__((no_sanitize("shift")))
#endif
    FORCE_INLINE constexpr static int32_t
    modulo_range(int32_t error_value) noexcept
    {
        return static_cast<int32_t>(error_value << (int32_t_bit_count - bpp)) >> (int32_t_bit_count - bpp); //NOLINT
    }

    FORCE_INLINE static SAMPLE compute_reconstructed_sample(const int32_t predicted_value, const int32_t error_value) noexcept
    {
        return static_cast<SAMPLE>(MAXVAL & (predicted_value + error_value));
    }

    FORCE_INLINE static int32_t correct_prediction(const int32_t predicted) noexcept
    {
        if ((predicted & MAXVAL) == predicted)
            return predicted;

        return (~(predicted >> (int32_t_bit_count - 1))) & MAXVAL;
    }
};


template<typename PixelType, int32_t bits_per_pixel>
struct lossless_traits final : lossless_traits_impl<PixelType, bits_per_pixel>
{
    using PIXEL = PixelType;
};


template<>
struct lossless_traits<uint8_t, 8> final : lossless_traits_impl<uint8_t, 8>
{
    using PIXEL = SAMPLE;

    FORCE_INLINE constexpr static signed char mod_range(const int32_t error_value) noexcept
    {
        return static_cast<signed char>(error_value);
    }

    FORCE_INLINE constexpr static int32_t compute_error_value(const int32_t d) noexcept
    {
        return static_cast<signed char>(d);
    }

    FORCE_INLINE constexpr static uint8_t compute_reconstructed_sample(const int32_t predicted_value, const int32_t error_value) noexcept
    {
        return static_cast<uint8_t>(predicted_value + error_value);
    }
};


template<>
struct lossless_traits<uint16_t, 16> final : lossless_traits_impl<uint16_t, 16>
{
    using PIXEL = SAMPLE;

    FORCE_INLINE constexpr static short mod_range(const int32_t error_value) noexcept
    {
        return static_cast<short>(error_value);
    }

    FORCE_INLINE constexpr static int32_t compute_error_value(const int32_t d) noexcept
    {
        return static_cast<short>(d);
    }

    FORCE_INLINE constexpr static SAMPLE compute_reconstructed_sample(const int32_t predicted_value, const int32_t error_value) noexcept
    {
        return static_cast<SAMPLE>(predicted_value + error_value);
    }
};


template<typename PixelType, int32_t bits_per_pixel>
struct lossless_traits<triplet<PixelType>, bits_per_pixel> final : lossless_traits_impl<PixelType, bits_per_pixel>
{
    using PIXEL = triplet<PixelType>;

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static bool is_near(PIXEL lhs, PIXEL rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static PixelType compute_reconstructed_sample(const int32_t predicted_value, const int32_t error_value) noexcept
    {
        return static_cast<PixelType>(predicted_value + error_value);
    }
};


template<typename T, int32_t bpp>
struct lossless_traits<quad<T>, bpp> final : lossless_traits_impl<T, bpp>
{
    using PIXEL = quad<T>;

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static bool is_near(PIXEL lhs, PIXEL rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static T compute_reconstructed_sample(const int32_t predicted_value, const int32_t error_value) noexcept
    {
        return static_cast<T>(predicted_value + error_value);
    }
};

} // namespace charls
