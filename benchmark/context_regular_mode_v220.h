// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "../src/regular_mode_context.hpp"

#include <cassert>
#include <cstdint>

namespace charls {

// Purpose: a JPEG-LS context with its current statistics.
struct jls_context_v220 final
{
    int32_t A{};
    int32_t B{};
    int16_t C{};
    int16_t N{1};

    jls_context_v220() = default;

    explicit jls_context_v220(const int32_t a) noexcept : A{a}
    {
    }

    [[nodiscard]]
    FORCE_INLINE int32_t get_error_correction(const int32_t k) const noexcept
    {
        if (k != 0)
            return 0;

        return bit_wise_sign(2 * B + N - 1);
    }

    /// <summary>Code segment A.12 – Variables update.</summary>
    FORCE_INLINE void update_variables(const int32_t error_value, const int32_t near_lossless, const int32_t reset_threshold)
    {
        ASSERT(N != 0);

        // For performance work on copies of A,B,N (compiler will use registers).
        int a{A + std::abs(error_value)};
        int b{B + error_value * (2 * near_lossless + 1)};
        int n{N};

        if (constexpr int limit{65536 * 256}; UNLIKELY(a >= limit || std::abs(b) >= limit))
            impl::throw_jpegls_error(jpegls_errc::invalid_data);

        if (n == reset_threshold)
        {
            a >>= 1;
            b >>= 1;
            n >>= 1;
        }

        A = a;
        ++n;
        N = static_cast<int16_t>(n);

        if (b + n <= 0)
        {
            b = b + n;
            if (b <= -n)
            {
                b = -n + 1;
            }
            C = static_cast<int16_t>(C - static_cast<int16_t>(C > -128));
        }
        else if (b > 0)
        {
            b = b - n;
            if (b > 0)
            {
                b = 0;
            }
            C = static_cast<int16_t>(C + static_cast<int16_t>(C < 127));
        }
        B = b;

        ASSERT(N != 0);
    }

    /// <summary>
    /// <para>Computes the Golomb coding parameter using the algorithm as defined in ISO/IEC 14495-1, code segment A.10
    /// </para> <para>Original algorithm is: for (k = 0; (N[Q] << k) < A[Q]; k++) </para>
    /// </summary>
    [[nodiscard]]
    FORCE_INLINE int32_t get_golomb_coding_parameter() const
    {
        int32_t k{};
        for (; N << k < A && k < max_k_value; ++k)
        {
        }

        if (UNLIKELY(k == max_k_value))
            impl::throw_jpegls_error(jpegls_errc::invalid_data);

        return k;
    }
};

} // namespace charls
