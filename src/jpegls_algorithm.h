// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.h"
#include "util.h"

namespace charls {

/// <summary>
/// This is the optimized inverse algorithm of ISO/IEC 14495-1, A.5.2, Code Segment A.11 (second else branch)
/// It will map unsigned values back to signed values.
/// </summary>
[[nodiscard]] constexpr int32_t unmap_error_value(const int32_t mapped_error) noexcept
{
    const int32_t sign{static_cast<int32_t>(static_cast<uint32_t>(mapped_error) << (int32_t_bit_count - 1)) >>
                       (int32_t_bit_count - 1)};
    return sign ^ (mapped_error >> 1);
}


/// <summary>
/// This is the algorithm of ISO/IEC 14495-1, A.5.2, Code Segment A.11 (second else branch)
/// It will map signed values to unsigned values. It has been optimized to prevent branching.
/// </summary>
[[nodiscard]] constexpr int32_t map_error_value(const int32_t error_value) noexcept
{
    ASSERT(error_value <= std::numeric_limits<int32_t>::max() / 2);

    const int32_t mapped_error{(error_value >> (int32_t_bit_count - 2)) ^ (2 * error_value)};
    return mapped_error;
}


[[nodiscard]] constexpr int32_t sign(const int32_t n) noexcept
{
    return (n >> (int32_t_bit_count - 1)) | 1;
}


[[nodiscard]] constexpr int32_t bit_wise_sign(const int32_t i) noexcept
{
    return i >> (int32_t_bit_count - 1);
}


[[nodiscard]] constexpr int32_t apply_sign(const int32_t i, const int32_t sign) noexcept
{
    return (sign ^ i) - sign;
}


/// <summary>
/// Computes the parameter RANGE. When NEAR = 0, RANGE = MAXVAL + 1. (see ISO/IEC 14495-1, A.2.1)
/// </summary>
[[nodiscard]] constexpr int32_t compute_range_parameter(const int32_t maximum_sample_value,
                                                        const int32_t near_lossless) noexcept
{
    return (maximum_sample_value + 2 * near_lossless) / (2 * near_lossless + 1) + 1;
}


/// <summary>
/// Computes the parameter LIMIT. (see ISO/IEC 14495-1, A.2.1)
/// </summary>
[[nodiscard]] constexpr int32_t compute_limit_parameter(const int32_t bits_per_pixel)
{
    return 2 * (bits_per_pixel + std::max(8, bits_per_pixel));
}


[[nodiscard]] inline int32_t get_predicted_value(const int32_t ra, const int32_t rb, const int32_t rc) noexcept
{
    // sign trick reduces the number of if statements (branches)
    const int32_t sign{bit_wise_sign(rb - ra)};

    // is Ra between Rc and Rb?
    if ((sign ^ (rc - ra)) < 0)
    {
        return rb;
    }
    if ((sign ^ (rb - rc)) < 0)
    {
        return ra;
    }

    // default case, valid if Rc element of [Ra,Rb]
    return ra + rb - rc;
}


[[nodiscard]] constexpr int32_t compute_context_id(const int32_t q1, const int32_t q2, const int32_t q3) noexcept
{
    return (q1 * 9 + q2) * 9 + q3;
}


// See JPEG-LS standard ISO/IEC 14495-1, A.3.3, golomb_code Segment A.4
[[nodiscard]] constexpr int8_t quantize_gradient_org(const int32_t di, const int32_t threshold1, const int32_t threshold2,
                                                  const int32_t threshold3, const int32_t near_lossless = 0) noexcept
{
    if (di <= -threshold3)
        return -4;
    if (di <= -threshold2)
        return -3;
    if (di <= -threshold1)
        return -2;
    if (di < -near_lossless)
        return -1;
    if (di <= near_lossless)
        return 0;
    if (di < threshold1)
        return 1;
    if (di < threshold2)
        return 2;
    if (di < threshold3)
        return 3;

    return 4;
}

} // namespace charls
