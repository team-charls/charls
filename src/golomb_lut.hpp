// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.hpp"
#include "util.hpp"

#include <array>

namespace charls {

/// <summary>
/// Maps a possible golomb code to an error value and a bit-count.
/// If the bit-count is zero, there was no match and full decoding is required.
/// </summary>
struct golomb_code_match final
{
    int32_t error_value;
    int32_t bit_count;
};


/// <summary>
/// Lookup up table with possible golomb code matches.
/// </summary>
class golomb_code_match_table final
{
public:
    explicit golomb_code_match_table(int32_t k);

    constexpr void add_entry(uint8_t value, golomb_code_match code) noexcept;

    [[nodiscard]]
    FORCE_INLINE golomb_code_match get(const size_t value) const noexcept
    {
        return matches_[value];
    }

private:
    static constexpr size_t byte_bit_count{8};
    std::array<golomb_code_match, 1 << byte_bit_count> matches_{};
};


extern const std::array<golomb_code_match_table, max_k_value> golomb_lut;

} // namespace charls
