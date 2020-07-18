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

    explicit jls_context(const int32_t a) noexcept :
        A{a}
    {
    }

    FORCE_INLINE int32_t get_error_correction(const int32_t k) const noexcept
    {
        if (k != 0)
            return 0;

        return bit_wise_sign(2 * B + N - 1);
    }

    FORCE_INLINE void update_variables(const int32_t error_value, const int32_t near_lossless, const int32_t reset_threshold) noexcept
    {
        ASSERT(N != 0);

        // For performance work on copies of A,B,N (compiler will use registers).
        int a = A + std::abs(error_value);
        int b = B + error_value * (2 * near_lossless + 1);
        int n = N;

        ASSERT(a < 65536 * 256);
        ASSERT(std::abs(b) < 65536 * 256);

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

    FORCE_INLINE int32_t get_golomb_code() const noexcept
    {
        const int32_t n_test = N;
        const int32_t a_test = A;

        if (n_test >= a_test) return 0;
        if (n_test << 1 >= a_test) return 1;
        if (n_test << 2 >= a_test) return 2;
        if (n_test << 3 >= a_test) return 3;
        if (n_test << 4 >= a_test) return 4;

        int32_t k = 5;
        for (; n_test << k < a_test; ++k)
        {
            ASSERT(k <= 32);
        }
        return k;
    }
};

} // namespace charls
