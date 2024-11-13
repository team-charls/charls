// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <benchmark/benchmark.h>

#include "../src/golomb_lut.hpp"
#include "../src/jpegls_algorithm.hpp"
#include "../src/conditional_static_cast.hpp"

#pragma warning(disable : 26409) // Avoid calling new explicitly (triggered by BENCHMARK macro)

using namespace charls;

static std::pair<int32_t, int32_t> create_encoded_value(const int32_t k, const int32_t mapped_error) noexcept
{
    const int32_t high_bits{mapped_error >> k};
    return std::make_pair(high_bits + k + 1, (1 << k) | (mapped_error & ((1 << k) - 1)));
}

golomb_code_match_table::golomb_code_match_table(const int32_t k)
{
    for (int16_t error_value{};; ++error_value)
    {
        // Q is not used when k != 0
        const int32_t mapped_error_value{map_error_value(error_value)};
        const auto [code_length, table_value]{create_encoded_value(k, mapped_error_value)};
        if (static_cast<size_t>(code_length) > byte_bit_count)
            break;

        const golomb_code_match code{error_value, conditional_static_cast<uint32_t>(code_length)};
        add_entry(static_cast<uint8_t>(table_value), code);
    }

    for (int16_t error_value{-1};; --error_value)
    {
        // Q is not used when k != 0
        const int32_t mapped_error_value{map_error_value(error_value)};
        const auto [code_length, table_value]{create_encoded_value(k, mapped_error_value)};
        if (static_cast<size_t>(code_length) > byte_bit_count)
            break;

        const golomb_code_match code{error_value, static_cast<uint32_t>(code_length)};
        add_entry(static_cast<uint8_t>(table_value), code);
    }
}

constexpr void golomb_code_match_table::add_entry(const uint8_t value, const golomb_code_match code) noexcept
{
    ASSERT(static_cast<size_t>(code.bit_count) <= byte_bit_count);

    for (size_t i{}; i < conditional_static_cast<size_t>(1U) << (byte_bit_count - code.bit_count); ++i)
    {
        const size_t index{(static_cast<size_t>(value) << (byte_bit_count - code.bit_count)) + i};
        ASSERT(matches_[index].bit_count == 0);
        matches_[index] = code;
    }
}

std::array<golomb_code_match_table, max_k_value> golomb_lut2{
    golomb_code_match_table(0),  golomb_code_match_table(1),  golomb_code_match_table(2),  golomb_code_match_table(3),
    golomb_code_match_table(4),  golomb_code_match_table(5),  golomb_code_match_table(6),  golomb_code_match_table(7),
    golomb_code_match_table(8),  golomb_code_match_table(9),  golomb_code_match_table(10), golomb_code_match_table(11),
    golomb_code_match_table(12), golomb_code_match_table(13), golomb_code_match_table(14), golomb_code_match_table(15)};


/// <summary>
/// Benchmark to measure how long it takes to initialize the golomb_code_match table at startup.
/// Information is useful to decide if initialization should be done at startup or at compile time (constexpr)
/// </summary>
static void bm_initialize_golomb_lut(benchmark::State& state)
{
    for (const auto _ : state)
    {
        for (int i = 0; i < max_k_value; ++i)
        {
            golomb_lut2[i] = golomb_code_match_table(i);
        }
    }
}
BENCHMARK(bm_initialize_golomb_lut);
