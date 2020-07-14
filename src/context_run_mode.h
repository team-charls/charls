// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.h"

#include <cassert>
#include <cstdint>

namespace charls {

// Implements statistical modeling for the run mode context.
// Computes model dependent parameters like the Golomb code lengths
struct context_run_mode final
{
    // Note: members are sorted based on their size.
    int32_t A{};
    int32_t nRItype_{};
    uint8_t n_reset_threshold_{};
    uint8_t N{};
    uint8_t Nn{};

    context_run_mode() = default;

    context_run_mode(const int32_t a, const int32_t nRItype, const int32_t n_reset_threshold) noexcept :
        A{a},
        nRItype_{nRItype},
        n_reset_threshold_{static_cast<uint8_t>(n_reset_threshold)},
        N{1}
    {
    }

    FORCE_INLINE int32_t get_golomb_code() const noexcept
    {
        const int32_t temp = A + (N >> 1) * nRItype_;
        int32_t n_test = N;
        int32_t k = 0;
        for (; n_test < temp; ++k)
        {
            n_test <<= 1;
            ASSERT(k <= 32);
        }
        return k;
    }

    void update_variables(const int32_t error_value, const int32_t e_mapped_error_value) noexcept
    {
        if (error_value < 0)
        {
            Nn = Nn + 1U;
        }
        A = A + ((e_mapped_error_value + 1 - nRItype_) >> 1);
        if (N == n_reset_threshold_)
        {
            A = A >> 1;
            N = N >> 1;
            Nn = Nn >> 1;
        }
        N = N + 1;
    }

    FORCE_INLINE int32_t compute_error_value(const int32_t temp, const int32_t k) const noexcept
    {
        const bool map = temp & 1;
        const int32_t error_value_abs = (temp + static_cast<int32_t>(map)) / 2;

        if ((k != 0 || (2 * Nn >= N)) == map)
        {
            ASSERT(map == compute_map(-error_value_abs, k));
            return -error_value_abs;
        }

        ASSERT(map == compute_map(error_value_abs, k));
        return error_value_abs;
    }

    bool compute_map(const int32_t error_value, const int32_t k) const noexcept
    {
        if (k == 0 && error_value > 0 && 2 * Nn < N)
            return true;

        if (error_value < 0 && 2 * Nn >= N)
            return true;

        if (error_value < 0 && k != 0)
            return true;

        return false;
    }

    FORCE_INLINE bool compute_map_negative_e(const int32_t k) const noexcept
    {
        return k != 0 || 2 * Nn >= N;
    }
};

} // namespace charls
