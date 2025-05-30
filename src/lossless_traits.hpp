// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.hpp"
#include "util.hpp"

#include <cstdint>

namespace charls {

// Optimized trait classes for lossless compression of 8-bit color and 8/16 bit monochrome images.
// This class assumes MaximumSampleValue correspond to a whole number of bits, and no custom ResetValue is set when encoding.
// The point of this is to have the most optimized code for the most common and most demanding scenario.
template<typename SampleType, int32_t BitsPerSample>
struct lossless_traits_impl
{
    using sample_type = SampleType;

    static constexpr bool always_lossless_and_default_parameters{true};
    static constexpr bool fixed_bits_per_pixel{true};

    // ISO 14495-1 MAXVAL symbol: maximum possible image sample value over all components of a scan.
    static constexpr int32_t maximum_sample_value{(1U << BitsPerSample) - 1};

    // ISO 14495-1 NEAR symbol: difference bound for near-lossless coding, 0 means lossless.
    static constexpr int32_t near_lossless{};

    // ISO 14495-1 qbpp symbol: number of bits needed to represent a mapped error value.
    static constexpr int32_t quantized_bits_per_sample{BitsPerSample};

    // ISO 14495-1 RANGE symbol: range of prediction error representation.
    static constexpr int32_t range{compute_range_parameter(maximum_sample_value, near_lossless)};

    // ISO 14495-1 bpp symbol: number of bits needed to represent MAXVAL (not less than 2).
    static constexpr int32_t bits_per_sample{BitsPerSample};

    // ISO 14495-1 LIMIT symbol: the value of glimit for a sample encoded in regular mode.
    static constexpr int32_t limit{compute_limit_parameter(BitsPerSample)};

    static constexpr uint32_t quantization_range{1U << BitsPerSample};

    static_assert(sizeof(SampleType) * 8 >= BitsPerSample);

    FORCE_INLINE constexpr static int32_t compute_error_value(const int32_t d) noexcept
    {
        return modulo_range(d);
    }

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE constexpr static int32_t modulo_range(const int32_t error_value) noexcept
    {
        return (static_cast<int32_t>(static_cast<uint32_t>(error_value) << (int32_t_bit_count - bits_per_sample))) >>
               (int32_t_bit_count - bits_per_sample);
    }

    FORCE_INLINE static SampleType compute_reconstructed_sample(const int32_t predicted_value,
                                                                const int32_t error_value) noexcept
    {
        return static_cast<SampleType>(maximum_sample_value & (predicted_value + error_value));
    }

    FORCE_INLINE static int32_t correct_prediction(const int32_t predicted) noexcept
    {
        if ((predicted & maximum_sample_value) == predicted)
            return predicted;

        return (~(predicted >> (int32_t_bit_count - 1))) & maximum_sample_value;
    }

#ifndef NDEBUG
    static bool is_valid() noexcept
    {
        return true;
    }
#endif
};


template<typename SampleType, int32_t BitsPerSample>
struct lossless_traits final : lossless_traits_impl<SampleType, BitsPerSample>
{
    using pixel_type = SampleType;
};


template<>
struct lossless_traits<uint8_t, 8> final : lossless_traits_impl<uint8_t, 8>
{
    using pixel_type = sample_type;

    FORCE_INLINE constexpr static int8_t modulo_range(const int32_t error_value) noexcept
    {
        return static_cast<int8_t>(error_value);
    }

    FORCE_INLINE constexpr static int32_t compute_error_value(const int32_t d) noexcept
    {
        return static_cast<int8_t>(d);
    }

    FORCE_INLINE constexpr static uint8_t compute_reconstructed_sample(const int32_t predicted_value,
                                                                       const int32_t error_value) noexcept
    {
        return static_cast<uint8_t>(predicted_value + error_value);
    }
};


template<>
struct lossless_traits<uint16_t, 16> final : lossless_traits_impl<uint16_t, 16>
{
    using pixel_type = sample_type;

    FORCE_INLINE constexpr static int16_t modulo_range(const int32_t error_value) noexcept
    {
        return static_cast<int16_t>(error_value);
    }

    FORCE_INLINE constexpr static int32_t compute_error_value(const int32_t d) noexcept
    {
        return static_cast<int16_t>(d);
    }

    FORCE_INLINE constexpr static sample_type compute_reconstructed_sample(const int32_t predicted_value,
                                                                           const int32_t error_value) noexcept
    {
        return static_cast<sample_type>(predicted_value + error_value);
    }
};


template<typename SampleType, int32_t BitsPerSample>
struct lossless_traits<pair<SampleType>, BitsPerSample> final : lossless_traits_impl<SampleType, BitsPerSample>
{
    using pixel_type = pair<SampleType>;

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE constexpr static bool is_near(const pixel_type lhs, const pixel_type rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static SampleType compute_reconstructed_sample(const int32_t predicted_value,
                                                                const int32_t error_value) noexcept
    {
        return static_cast<SampleType>(predicted_value + error_value);
    }
};


template<typename SampleType, int32_t BitsPerSample>
struct lossless_traits<triplet<SampleType>, BitsPerSample> final : lossless_traits_impl<SampleType, BitsPerSample>
{
    using pixel_type = triplet<SampleType>;

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE constexpr static bool is_near(const pixel_type lhs, const pixel_type rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static SampleType compute_reconstructed_sample(const int32_t predicted_value,
                                                                const int32_t error_value) noexcept
    {
        return static_cast<SampleType>(predicted_value + error_value);
    }
};


template<typename SampleType, int32_t BitsPerSample>
struct lossless_traits<quad<SampleType>, BitsPerSample> final : lossless_traits_impl<SampleType, BitsPerSample>
{
    using pixel_type = quad<SampleType>;

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE constexpr static bool is_near(const pixel_type lhs, const pixel_type rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static SampleType compute_reconstructed_sample(const int32_t predicted_value,
                                                                const int32_t error_value) noexcept
    {
        return static_cast<SampleType>(predicted_value + error_value);
    }
};

} // namespace charls
