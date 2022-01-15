// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <benchmark/benchmark.h>

#include "context_regular_mode_v220.h"

using namespace charls;

context_regular_mode g_context;
jls_context_v220 g_context_v220;

volatile int32_t error_value;
volatile int32_t near_lossless;
volatile int32_t reset_threshold = 64;

static void bm_regular_mode_update_variables_220(benchmark::State& state)
{
    g_context_v220 = jls_context_v220();

    for (const auto _ : state)
    {
        g_context_v220.update_variables(error_value, near_lossless, reset_threshold);
    }
}
BENCHMARK(bm_regular_mode_update_variables_220);

static void bm_regular_mode_update_variables(benchmark::State& state)
{
    g_context = context_regular_mode();

    for (const auto _ : state)
    {
        g_context.update_variables_and_bias(error_value, near_lossless, reset_threshold);
    }
}
BENCHMARK(bm_regular_mode_update_variables);

static void bm_regular_mode_get_golomb_coding_parameter_v220(benchmark::State& state)
{
    g_context_v220 = jls_context_v220();
    g_context_v220.update_variables(error_value, near_lossless, reset_threshold);

    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(g_context_v220.get_golomb_coding_parameter());
    }
}
BENCHMARK(bm_regular_mode_get_golomb_coding_parameter_v220);

static void bm_regular_mode_get_golomb_coding_parameter(benchmark::State& state)
{
    g_context = context_regular_mode();
    g_context.update_variables_and_bias(error_value, near_lossless, reset_threshold);

    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(g_context.get_golomb_coding_parameter());
    }
}
BENCHMARK(bm_regular_mode_get_golomb_coding_parameter);
