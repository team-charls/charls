// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <benchmark/benchmark.h>

#include "../src/jpegls_algorithm.hpp"

#include <cmath>
#include <limits>

#pragma warning(disable : 26409) // Avoid calling new explicitly (triggered by BENCHMARK macro)

static uint32_t log2_floor(const uint32_t n) noexcept
{
    return 31 - charls::countl_zero(n);
}

static uint32_t max_value_to_bits_per_sample(const uint32_t max_value) noexcept
{
    ASSERT(max_value > 0);
    return log2_floor(max_value) + 1;
}


static void bm_log2_floor_floating_point(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(std::floor(std::log2(255)));
        benchmark::DoNotOptimize(std::floor(std::log2(1023)));
        benchmark::DoNotOptimize(std::floor(std::log2(std::numeric_limits<uint16_t>::max())));
    }
}
BENCHMARK(bm_log2_floor_floating_point);


static void bm_log2_floor_uint32(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(log2_floor(255));
        benchmark::DoNotOptimize(log2_floor(1023));
        benchmark::DoNotOptimize(log2_floor(std::numeric_limits<uint16_t>::max()));
    }
}
BENCHMARK(bm_log2_floor_uint32);

static void bm_log2_ceil_int32(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(charls::log2_ceiling(256));
        benchmark::DoNotOptimize(charls::log2_ceiling(1024));
        benchmark::DoNotOptimize(charls::log2_ceiling(std::numeric_limits<uint16_t>::max()));
    }
}
BENCHMARK(bm_log2_ceil_int32);

static void bm_max_value_to_bits_per_sample(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(max_value_to_bits_per_sample(255));
        benchmark::DoNotOptimize(max_value_to_bits_per_sample(1023));
        benchmark::DoNotOptimize(max_value_to_bits_per_sample(std::numeric_limits<uint16_t>::max()));
    }
}
BENCHMARK(bm_max_value_to_bits_per_sample);
