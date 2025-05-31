// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.hpp"
#include "jpegls_algorithm.hpp"
#include "util.hpp"

#include <cstdlib>

// Default traits that support all JPEG LS parameters: custom limit, near, maxval (not power of 2)

// This traits class is used to initialize a coder/decoder.
// The coder/decoder also delegates some functions to the traits class.
// This is to allow the traits class to replace the default implementation here with optimized specific implementations.
// This is done for lossless coding/decoding: see lossless_traits.h

namespace charls {

template<typename SampleType, typename PixelType>
struct default_traits final
{
    using sample_type = SampleType;
    using pixel_type = PixelType;

    static constexpr bool always_lossless_and_default_parameters{};
    static constexpr bool fixed_bits_per_pixel{};

    // ISO 14495-1 MAXVAL symbol: maximum possible image sample value over all components of a scan.
    int32_t maximum_sample_value;

    // ISO 14495-1 NEAR symbol: difference bound for near-lossless coding, 0 means lossless.
    int32_t near_lossless;

    // ISO 14495-1 RANGE symbol: range of prediction error representation.
    int32_t range;

    // ISO 14495-1 qbpp symbol: number of bits needed to represent a mapped error value.
    int32_t quantized_bits_per_sample;

    // ISO 14495-1 bpp symbol: number of bits needed to represent MAXVAL (not less than 2).
    int32_t bits_per_sample;

    // ISO 14495-1 LIMIT symbol: the value of glimit for a sample encoded in regular mode.
    int32_t limit;

    uint32_t quantization_range;

    default_traits(const int32_t arg_maximum_sample_value, const int32_t arg_near_lossless) noexcept :
        maximum_sample_value{arg_maximum_sample_value},
        near_lossless{arg_near_lossless},
        range{compute_range_parameter(maximum_sample_value, near_lossless)},
        quantized_bits_per_sample{log2_ceiling(range)},
        bits_per_sample{log2_ceiling(maximum_sample_value)},
        limit{compute_limit_parameter(bits_per_sample)},
        quantization_range{1U << bits_per_sample}
    {
        ASSERT(sizeof(SampleType) * 8 >= static_cast<size_t>(bits_per_sample));
    }

    default_traits() = delete;
    default_traits(const default_traits&) noexcept = default;
    default_traits(default_traits&&) noexcept = default;
    ~default_traits() = default;
    default_traits& operator=(const default_traits&) = delete;
    default_traits& operator=(default_traits&&) = delete;

    [[nodiscard]]
    FORCE_INLINE int32_t compute_error_value(const int32_t e) const noexcept
    {
        return modulo_range(quantize(e));
    }

    [[nodiscard]]
    FORCE_INLINE SampleType compute_reconstructed_sample(const int32_t predicted_value,
                                                         const int32_t error_value) const noexcept
    {
        return fix_reconstructed_value(predicted_value + dequantize(error_value));
    }

    [[nodiscard]]
    FORCE_INLINE bool is_near(const int32_t lhs, const int32_t rhs) const noexcept
    {
        return std::abs(lhs - rhs) <= near_lossless;
    }

    [[nodiscard]]
    bool is_near(const pair<SampleType> lhs, const pair<SampleType> rhs) const noexcept
    {
        return std::abs(lhs.v1 - rhs.v1) <= near_lossless && std::abs(lhs.v2 - rhs.v2) <= near_lossless;
    }

    [[nodiscard]]
    bool is_near(const triplet<SampleType> lhs, const triplet<SampleType> rhs) const noexcept
    {
        return std::abs(lhs.v1 - rhs.v1) <= near_lossless && std::abs(lhs.v2 - rhs.v2) <= near_lossless &&
               std::abs(lhs.v3 - rhs.v3) <= near_lossless;
    }

    [[nodiscard]]
    bool is_near(const quad<SampleType> lhs, const quad<SampleType> rhs) const noexcept
    {
        return std::abs(lhs.v1 - rhs.v1) <= near_lossless && std::abs(lhs.v2 - rhs.v2) <= near_lossless &&
               std::abs(lhs.v3 - rhs.v3) <= near_lossless && std::abs(lhs.v4 - rhs.v4) <= near_lossless;
    }

    [[nodiscard]]
    FORCE_INLINE int32_t correct_prediction(const int32_t predicted) const noexcept
    {
        if ((predicted & maximum_sample_value) == predicted)
            return predicted;

        return (~(predicted >> (int32_t_bit_count - 1))) & maximum_sample_value;
    }

    /// <summary>
    /// Returns the value of errorValue modulo RANGE. ITU.T.87, A.4.5 (code segment A.9)
    /// This ensures the error is reduced to the range (-⌊RANGE/2⌋ - ⌈RANGE/2⌉-1)
    /// </summary>
    [[nodiscard]]
    FORCE_INLINE int32_t modulo_range(int32_t error_value) const noexcept
    {
        ASSERT(std::abs(error_value) <= range);

        if (error_value < 0)
        {
            error_value += range;
        }

        if (error_value >= (range + 1) / 2)
        {
            error_value -= range;
        }

        ASSERT(-range / 2 <= error_value && error_value <= ((range + 1) / 2) - 1);
        return error_value;
    }

#ifndef NDEBUG
    [[nodiscard]]
    bool is_valid() const noexcept
    {
        if (maximum_sample_value < 1 || maximum_sample_value > std::numeric_limits<uint16_t>::max())
            return false;

        if (bits_per_sample < 1 || bits_per_sample > 16)
            return false;

        return true;
    }
#endif

private:
    [[nodiscard]]
    int32_t quantize(const int32_t error_value) const noexcept
    {
        if (error_value > 0)
            return (error_value + near_lossless) / (2 * near_lossless + 1);

        return -(near_lossless - error_value) / (2 * near_lossless + 1);
    }

    [[nodiscard]]
    FORCE_INLINE int32_t dequantize(const int32_t error_value) const noexcept
    {
        return error_value * (2 * near_lossless + 1);
    }

    [[nodiscard]]
    FORCE_INLINE SampleType fix_reconstructed_value(int32_t value) const noexcept
    {
        if (value < -near_lossless)
        {
            value = value + range * (2 * near_lossless + 1);
        }
        else if (value > maximum_sample_value + near_lossless)
        {
            value = value - range * (2 * near_lossless + 1);
        }

        return static_cast<SampleType>(correct_prediction(value));
    }
};

} // namespace charls
