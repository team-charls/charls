// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "golomb_lut.hpp"

#include "jpegls_algorithm.hpp"

namespace charls {

namespace {

// Functions to build tables used to decode short Golomb codes.
constexpr std::pair<int32_t, int32_t> create_encoded_value(const int32_t k, const int32_t mapped_error) noexcept
{
    const int32_t high_bits{mapped_error >> k};
    return std::make_pair(high_bits + k + 1, (1 << k) | (mapped_error & ((1 << k) - 1)));
}

constexpr golomb_code_table make_code_table(const int32_t k) noexcept
{
    golomb_code_table table;
    for (int16_t error_value{};; ++error_value)
    {
        // Q is not used when k != 0
        const int32_t mapped_error_value{map_error_value(error_value)};
        const auto [code_length, table_value]{create_encoded_value(k, mapped_error_value)};
        if (static_cast<size_t>(code_length) > golomb_code_table::byte_bit_count)
            break;

        const golomb_code code(error_value, conditional_static_cast<int16_t>(code_length));
        table.add_entry(static_cast<uint8_t>(table_value), code);
    }

    for (int16_t error_value{-1};; --error_value)
    {
        // Q is not used when k != 0
        const int32_t mapped_error_value{map_error_value(error_value)};
        const auto [code_length, table_value]{create_encoded_value(k, mapped_error_value)};
        if (static_cast<size_t>(code_length) > golomb_code_table::byte_bit_count)
            break;

        const auto code{golomb_code(error_value, static_cast<int16_t>(code_length))};
        table.add_entry(static_cast<uint8_t>(table_value), code);
    }

    return table;
}

} // namespace


// Lookup table: decode symbols that are smaller or equal to 8 bit (16 tables for each value of k)
const std::array<golomb_code_table, max_k_value> golomb_lut{
    make_code_table(0),  make_code_table(1),  make_code_table(2),  make_code_table(3),
    make_code_table(4),  make_code_table(5),  make_code_table(6),  make_code_table(7),
    make_code_table(8),  make_code_table(9),  make_code_table(10), make_code_table(11),
    make_code_table(12), make_code_table(13), make_code_table(14), make_code_table(15)};

} // namespace charls
