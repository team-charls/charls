// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <benchmark/benchmark.h>

#include "../src/jpegls_preset_coding_parameters.h"

#include <cstdint>
#include <memory>
#include <vector>

#pragma warning(disable : 26409) // Avoid calling new explicitly (triggered by BENCHMARK macro)


int8_t quantize_gradient_org(const charls::jpegls_pc_parameters& preset, const int32_t di) noexcept
{
    constexpr int32_t near_lossless{};

    if (di <= -preset.threshold3)
        return -4;
    if (di <= -preset.threshold2)
        return -3;
    if (di <= -preset.threshold1)
        return -2;
    if (di < -near_lossless)
        return -1;
    if (di <= near_lossless)
        return 0;
    if (di < preset.threshold1)
        return 1;
    if (di < preset.threshold2)
        return 2;
    if (di < preset.threshold3)
        return 3;

    return 4;
}

std::vector<int8_t> create_quantize_lut_lossless(const int32_t bit_count)
{
    const charls::jpegls_pc_parameters preset{charls::compute_default((1 << static_cast<uint32_t>(bit_count)) - 1, 0)};
    const int32_t range{preset.maximum_sample_value + 1};

    std::vector<int8_t> lut(static_cast<size_t>(range) * 2);
    for (size_t i{}; i != lut.size(); ++i)
    {
        lut[i] = quantize_gradient_org(preset, static_cast<int32_t>(i) - range);
    }

    return lut;
}


const std::vector<int8_t> quantization_lut_lossless_8{create_quantize_lut_lossless(8)};

template<typename Traits>
struct scan_decoder
{
    int32_t t1_{};
    int32_t t2_{};
    int32_t t3_{};
    Traits traits_;

    explicit scan_decoder(Traits traits, const int32_t bit_count) noexcept : traits_{std::move(traits)}
    {
        const charls::jpegls_pc_parameters preset{charls::compute_default((1 << static_cast<uint32_t>(bit_count)) - 1, 0)};

        t1_ = preset.threshold1;
        t2_ = preset.threshold2;
        t3_ = preset.threshold3;
    }

    int8_t quantize_gradient_org(const int32_t di) const noexcept
    {
        if (di <= -t3_)
            return -4;
        if (di <= -t2_)
            return -3;
        if (di <= -t1_)
            return -2;
        if (di < -traits_.near_lossless)
            return -1;
        if (di <= traits_.near_lossless)
            return 0;
        if (di < t1_)
            return 1;
        if (di < t2_)
            return 2;
        if (di < t3_)
            return 3;

        return 4;
    }
};

struct lossless_traits final
{
    static constexpr int32_t near_lossless{};
};



__declspec(noinline) int32_t get_predicted_value_default(const int32_t ra, const int32_t rb, const int32_t rc) noexcept
{
    if (ra < rb)
    {
        if (rc < ra)
            return rb;

        if (rc > rb)
            return ra;
    }
    else
    {
        if (rc < rb)
            return ra;

        if (rc > ra)
            return rb;
    }

    return ra + rb - rc;
}


constexpr size_t int32_t_bit_count = sizeof(int32_t) * 8;


constexpr int32_t bit_wise_sign(const int32_t i) noexcept
{
    return i >> (int32_t_bit_count - 1);
}


__declspec(noinline) int32_t get_predicted_value_optimized(const int32_t ra, const int32_t rb, const int32_t rc) noexcept
{
    // sign trick reduces the number of if statements (branches)
    const int32_t sign{bit_wise_sign(rb - ra)};

    // is Ra between Rc and Rb?
    if ((sign ^ (rc - ra)) < 0)
    {
        return rb;
    }
    if ((sign ^ (rb - rc)) < 0)
    {
        return ra;
    }

    // default case, valid if Rc element of [Ra,Rb]
    return ra + rb - rc;
}


#if defined(_M_X64) || defined(_M_ARM64)
inline int countl_zero(const uint64_t value) noexcept
{
    if (value == 0)
        return 64;

    unsigned long index;
    _BitScanReverse64(&index, value);

    return 63 - static_cast<int>(index);
}
#endif


static void bm_get_predicted_value_default(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(get_predicted_value_default(100, 200, 300));
        benchmark::DoNotOptimize(get_predicted_value_default(200, 100, 300));
    }
}
BENCHMARK(bm_get_predicted_value_default);

static void bm_get_predicted_value_optimized(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(get_predicted_value_optimized(100, 200, 300));
        benchmark::DoNotOptimize(get_predicted_value_default(200, 100, 300));
    }
}
BENCHMARK(bm_get_predicted_value_optimized);

static void bm_quantize_gradient_calculated(benchmark::State& state)
{
    const scan_decoder<lossless_traits> sd({}, 8);

    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(sd.quantize_gradient_org(0));
        benchmark::DoNotOptimize(sd.quantize_gradient_org(127));
        benchmark::DoNotOptimize(sd.quantize_gradient_org(255));
    }
}
BENCHMARK(bm_quantize_gradient_calculated);

static void bm_quantize_gradient_lut(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(quantization_lut_lossless_8[0]);
        benchmark::DoNotOptimize(quantization_lut_lossless_8[127]);
        benchmark::DoNotOptimize(quantization_lut_lossless_8[255]);
    }
}
BENCHMARK(bm_quantize_gradient_lut);


int peek_zero_bits(uint64_t val_test) noexcept
{
    for (int32_t count{}; count < 16; ++count)
    {
        if ((val_test & (uint64_t{1} << (64 - 1))) != 0)
            return count;

        val_test <<= 1;
    }
    return -1;
}

static void bm_peek_zero_bits(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(peek_zero_bits(0));
        benchmark::DoNotOptimize(peek_zero_bits(UINT64_MAX));
    }
}
BENCHMARK(bm_peek_zero_bits);


#if defined(_M_X64) || defined(_M_ARM64)
int peek_zero_bits_intrinsic(const uint64_t value) noexcept
{
    const auto count = countl_zero(value);
    return count < 16 ? count : -1;
}


static void bm_peek_zero_bits_intrinsic(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(peek_zero_bits_intrinsic(0));
        benchmark::DoNotOptimize(peek_zero_bits_intrinsic(UINT64_MAX));
    }
}
BENCHMARK(bm_peek_zero_bits_intrinsic);
#endif


std::vector<uint8_t> allocate_buffer(const size_t size)
{
    std::vector<uint8_t> buffer;
    buffer.resize(size);
    return buffer;
}

static void bm_resize_vector(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(allocate_buffer(size_t{512} * 512 * 16));
        benchmark::DoNotOptimize(allocate_buffer(size_t{1024} * 1024 * 8 * 3));
    }
}
BENCHMARK(bm_resize_vector);


class overwrite_buffer
{
public:
    void reset(const size_t new_size)
    {
        if (new_size <= size_)
        {
            size_ = new_size;
            return;
        }

        data_.reset(); // First release, then re-alloc new memory.
        data_.reset(new uint8_t[new_size]);
        size_ = new_size;
    }

    uint8_t* data() const noexcept
    {
        return data_.get();
    }

    size_t size() const noexcept
    {
        return size_;
    }

private:
    std::unique_ptr<uint8_t[]> data_{};
    size_t size_{};
};



overwrite_buffer allocate_overwrite_buffer(const size_t size)
{
    overwrite_buffer buffer;
    buffer.reset(size);
    return buffer;
}



static void bm_resize_overwrite_buffer(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(allocate_buffer(size_t{512} * 512 * 16));
        benchmark::DoNotOptimize(allocate_buffer(size_t{1024} * 1024 * 8 * 3));
    }
}
BENCHMARK(bm_resize_overwrite_buffer);

// Tips to run the benchmark tests:

// To run a single benchmark:
// benchmark --benchmark_filter=bm_decode

BENCHMARK_MAIN();
