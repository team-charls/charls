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
    int32_t run_interruption_type{};

    context_run_mode() = default;

    context_run_mode(const int32_t arg_run_interruption_type, const int32_t a, const int32_t reset_threshold) noexcept :
        run_interruption_type{arg_run_interruption_type},
        a_{a},
        reset_threshold_{static_cast<uint8_t>(reset_threshold)},
        n_{1}
    {
    }

    FORCE_INLINE int32_t get_golomb_code() const noexcept
    {
        const int32_t temp{a_ + (n_ >> 1) * run_interruption_type};
        int32_t n_test{n_};
        int32_t k{};
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
            nn_ = nn_ + 1U;
        }
        a_ = a_ + ((e_mapped_error_value + 1 - run_interruption_type) >> 1);
        if (n_ == reset_threshold_)
        {
            a_ = a_ >> 1;
            n_ = n_ >> 1;
            nn_ = nn_ >> 1;
        }
        n_ = n_ + 1;
    }

    FORCE_INLINE int32_t compute_error_value(const int32_t temp, const int32_t k) const noexcept
    {
        const bool map = temp & 1;
        const int32_t error_value_abs{(temp + static_cast<int32_t>(map)) / 2};

        if ((k != 0 || (2 * nn_ >= n_)) == map)
        {
            ASSERT(map == compute_map(-error_value_abs, k));
            return -error_value_abs;
        }

        ASSERT(map == compute_map(error_value_abs, k));
        return error_value_abs;
    }

    bool compute_map(const int32_t error_value, const int32_t k) const noexcept
    {
        if (k == 0 && error_value > 0 && 2 * nn_ < n_)
            return true;

        if (error_value < 0 && 2 * nn_ >= n_)
            return true;

        if (error_value < 0 && k != 0)
            return true;

        return false;
    }

    FORCE_INLINE bool compute_map_negative_e(const int32_t k) const noexcept
    {
        return k != 0 || 2 * nn_ >= n_;
    }

private:
    int32_t a_{};
    uint8_t reset_threshold_{};
    uint8_t n_{};
    uint8_t nn_{};
};

} // namespace charls
