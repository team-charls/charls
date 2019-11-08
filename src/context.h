// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.h"

#include <cassert>
#include <cstdint>

namespace charls {

// Purpose: a JPEG-LS context with it's current statistics.
struct JlsContext final
{
    int32_t A{};
    int32_t B{};
    int16_t C{};
    int16_t N{1};

    JlsContext() = default;

    explicit JlsContext(int32_t a) noexcept :
        A{a}
    {
    }

    FORCE_INLINE int32_t GetErrorCorrection(int32_t k) const noexcept
    {
        if (k != 0)
            return 0;

        return BitWiseSign(2 * B + N - 1);
    }

    FORCE_INLINE void UpdateVariables(int32_t errorValue, int32_t NEAR, int32_t NRESET) noexcept
    {
        ASSERT(N != 0);

        // For performance work on copies of A,B,N (compiler will use registers).
        int a = A + std::abs(errorValue);
        int b = B + errorValue * (2 * NEAR + 1);
        int n = N;

        ASSERT(a < 65536 * 256);
        ASSERT(std::abs(b) < 65536 * 256);

        if (n == NRESET)
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

    FORCE_INLINE int32_t GetGolomb() const noexcept
    {
        const int32_t nTest = N;
        const int32_t aTest = A;

        if (nTest >= aTest) return 0;
        if (nTest << 1 >= aTest) return 1;
        if (nTest << 2 >= aTest) return 2;
        if (nTest << 3 >= aTest) return 3;
        if (nTest << 4 >= aTest) return 4;

        int32_t k = 5;
        for (; nTest << k < aTest; ++k)
        {
            ASSERT(k <= 32);
        }
        return k;
    }
};

} // namespace charls
