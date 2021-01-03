// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.h"

#include <cassert>
#include <cstdint>

namespace charls {

// Purpose: a JPEG-LS context with it's current statistics.
struct jls_context final
{
    int32_t A{};
    int32_t B{};
    int16_t C{};
    int16_t N{1};

    jls_context() = default;

    explicit jls_context(const int32_t a) noexcept : A{a}
    {
    }

    FORCE_INLINE int32_t get_error_correction(const int32_t k) const noexcept
    {
        if (k != 0)
            return 0;

        return bit_wise_sign(2 * B + N - 1);
    }

    FORCE_INLINE void update_variables(const int32_t error_value, const int32_t near_lossless,
                                       const int32_t reset_threshold)
    {
        ASSERT(N != 0);

        // For performance work on copies of A,B,N (compiler will use registers).
        int a{A + std::abs(error_value)};
        int b{B + error_value * (2 * near_lossless + 1)};
        int n{N};

        constexpr int limit{65536 * 256};
        if (a >= limit || std::abs(b) >= limit)
            impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

        if (n == reset_threshold)
        {
            a = a >> 1;
            b = b >> 1;
            n = n >> 1;
        }

        A = a;
        n = n + 1;
        N = static_cast<int16_t>(n);

        if (b + n <= 0)
        {
            b = b + n;
            if (b <= -n)
            {
                b = -n + 1;
            }
            C = C - (C > -128);
        }
        else if (b > 0)
        {
            b = b - n;
            if (b > 0)
            {
                b = 0;
            }
            C = C + (C < 127);
        }
        B = b;

        ASSERT(N != 0);
    }

    /// <summary>
    /// <para>Computes the Golomb coding parameter using the algorithm as defined in ISO/IEC 14495-1, code segment A.10 </para>
    /// <para>Original algorithm is: for (k = 0; (N[Q] << k) < A[Q]; k++) </para>
    /// </summary>
    FORCE_INLINE int32_t get_golomb_coding_parameter() const
    {
        int32_t k{0};
        for (; N << k < A && k < max_k_value; ++k)
        {
        }

        if (k == max_k_value)
            impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

        return k;
    }
};

} // namespace charls
