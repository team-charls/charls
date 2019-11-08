// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.h"

#include <cassert>
#include <cstdint>

namespace charls {

// Implements statistical modeling for the run mode context.
// Computes model dependent parameters like the Golomb code lengths
struct CContextRunMode final
{
    // Note: members are sorted based on their size.
    int32_t A{};
    int32_t nRItype_{};
    uint8_t nReset_{};
    uint8_t N{};
    uint8_t Nn{};

    CContextRunMode() = default;

    CContextRunMode(int32_t a, int32_t nRItype, int32_t nReset) noexcept :
        A{a},
        nRItype_{nRItype},
        nReset_{static_cast<uint8_t>(nReset)},
        N{1}
    {
    }

    FORCE_INLINE int32_t GetGolomb() const noexcept
    {
        const int32_t TEMP = A + (N >> 1) * nRItype_;
        int32_t nTest = N;
        int32_t k = 0;
        for (; nTest < TEMP; ++k)
        {
            nTest <<= 1;
            ASSERT(k <= 32);
        }
        return k;
    }

    void UpdateVariables(int32_t errorValue, int32_t EMErrval) noexcept
    {
        if (errorValue < 0)
        {
            Nn = Nn + 1;
        }
        A = A + ((EMErrval + 1 - nRItype_) >> 1);
        if (N == nReset_)
        {
            A = A >> 1;
            N = N >> 1;
            Nn = Nn >> 1;
        }
        N = N + 1;
    }

    FORCE_INLINE int32_t ComputeErrVal(int32_t temp, int32_t k) const noexcept
    {
        const bool map = temp & 1;
        const int32_t errorValueAbs = (temp + static_cast<int32_t>(map)) / 2;

        if ((k != 0 || (2 * Nn >= N)) == map)
        {
            ASSERT(map == ComputeMap(-errorValueAbs, k));
            return -errorValueAbs;
        }

        ASSERT(map == ComputeMap(errorValueAbs, k));
        return errorValueAbs;
    }

    bool ComputeMap(int32_t errorValue, int32_t k) const noexcept
    {
        if (k == 0 && errorValue > 0 && 2 * Nn < N)
            return true;

        if (errorValue < 0 && 2 * Nn >= N)
            return true;

        if (errorValue < 0 && k != 0)
            return true;

        return false;
    }

    FORCE_INLINE bool ComputeMapNegativeE(int32_t k) const noexcept
    {
        return k != 0 || 2 * Nn >= N;
    }
};

} // namespace charls
