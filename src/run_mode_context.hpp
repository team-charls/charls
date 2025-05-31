// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_algorithm.hpp"
#include "util.hpp"
#include "assert.hpp"

#include <cstdint>

namespace charls {

/// <summary>
/// JPEG-LS uses arrays of variables: A[0..366], B[0..364], C[0..364] and N[0..366]
/// to maintain the statistic information for the context modeling.
/// Index 365 and 366 are used for run mode interruption contexts.
/// </summary>
class run_mode_context final
{
public:
    run_mode_context() = default;

    run_mode_context(const int32_t run_interruption_type, const int32_t range) noexcept :
        run_interruption_type_{run_interruption_type}, a_{initialization_value_for_a(range)}
    {
    }

    [[nodiscard]]
    int32_t run_interruption_type() const noexcept
    {
        return run_interruption_type_;
    }

    [[nodiscard]]
    FORCE_INLINE int32_t compute_golomb_coding_parameter_checked() const
    {
        const int32_t temp{a_ + (n_ >> 1) * run_interruption_type_};
        int32_t n_test{n_};
        int32_t k{};
        for (; n_test < temp; ++k)
        {
            n_test <<= 1;
            if (k > 32)
                impl::throw_jpegls_error(jpegls_errc::invalid_data);
        }
        return k;
    }

    [[nodiscard]]
    FORCE_INLINE int32_t compute_golomb_coding_parameter() const noexcept
    {
        const int32_t temp{a_ + (n_ >> 1) * run_interruption_type_};
        int32_t n_test{n_};
        int32_t k{};
        for (; n_test < temp; ++k)
        {
            n_test <<= 1;
            ASSERT(k <= 32);
        }
        return k;
    }

    /// <summary>Code segment A.23 – Update of variables for run interruption sample.</summary>
    void update_variables(const int32_t error_value, const int32_t e_mapped_error_value,
                          const int32_t reset_threshold) noexcept
    {
        if (error_value < 0)
        {
            ++nn_;
        }

        a_ += (e_mapped_error_value + 1 - run_interruption_type_) >> 1;

        if (n_ == reset_threshold)
        {
            a_ >>= 1;
            n_ >>= 1;
            nn_ >>= 1;
        }

        ++n_;
    }

    [[nodiscard]]
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

    /// <summary>Code segment A.21 – Computation of map for error value mapping.</summary>
    [[nodiscard]]
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

private:
    // Initialize with the default values as defined in ISO 14495-1, A.8, step 1.d and 1.f.
    int32_t run_interruption_type_{};
    int32_t a_{};
    int32_t n_{1};
    int32_t nn_{};
};

} // namespace charls
