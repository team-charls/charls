// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <benchmark/benchmark.h>

#include "../src/jpegls_preset_coding_parameters.hpp"

#include <cstdint>
#include <memory>
#include <vector>

#pragma warning(disable : 26409) // Avoid calling new explicitly (triggered by BENCHMARK macro)


static int8_t quantize_gradient_org(const charls::jpegls_pc_parameters& preset, const int32_t di) noexcept
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

static std::vector<int8_t> create_quantize_lut_lossless(const int32_t bit_count)
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


const std::vector quantization_lut_lossless_8{create_quantize_lut_lossless(8)};

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

    [[nodiscard]]
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


static __declspec(noinline) int32_t
    get_predicted_value_default(const int32_t ra, const int32_t rb, const int32_t rc) noexcept
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


static constexpr int32_t bit_wise_sign(const int32_t i) noexcept
{
    return i >> (int32_t_bit_count - 1);
}


static __declspec(noinline) int32_t
    get_predicted_value_optimized(const int32_t ra, const int32_t rb, const int32_t rc) noexcept
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
inline static int countl_zero(const uint64_t value) noexcept
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


static int peek_zero_bits(uint64_t val_test) noexcept
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


static std::vector<uint8_t> allocate_buffer(const size_t size)
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

    [[nodiscard]]
    uint8_t* data() const noexcept
    {
        return data_.get();
    }

    [[nodiscard]]
    size_t size() const noexcept
    {
        return size_;
    }

private:
    std::unique_ptr<uint8_t[]> data_{};
    size_t size_{};
};


static overwrite_buffer allocate_overwrite_buffer(const size_t size)
{
    overwrite_buffer buffer;
    buffer.reset(size);
    return buffer;
}

static void bm_resize_overwrite_buffer(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(allocate_overwrite_buffer(size_t{512} * 512 * 16));
        benchmark::DoNotOptimize(allocate_overwrite_buffer(size_t{1024} * 1024 * 8 * 3));
    }
}
BENCHMARK(bm_resize_overwrite_buffer);


static int memset_buffer(uint8_t* data, const size_t size) noexcept
{
    memset(data, 0, size);
    return 0;
}
static void bm_memset_buffer(benchmark::State& state)
{
    std::vector<uint8_t> buffer(size_t{1024} * 1024 * 8 * 3);

    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(memset_buffer(buffer.data(), size_t{512} * 512 * 16));
        benchmark::DoNotOptimize(memset_buffer(buffer.data(), size_t{1024} * 1024 * 8 * 3));
    }
}
BENCHMARK(bm_memset_buffer);


constexpr static bool has_ff_byte_classic(const unsigned int value) noexcept
{
    // Check if any byte is equal to 0xFF
    return ((value & 0xFF) == 0xFF) || (((value >> 8) & 0xFF) == 0xFF) || (((value >> 16) & 0xFF) == 0xFF) ||
           (((value >> 24) & 0xFF) == 0xFF);
}
static void bm_has_ff_byte_classic(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(has_ff_byte_classic(0));
        benchmark::DoNotOptimize(has_ff_byte_classic(0xFF));
    }
}
BENCHMARK(bm_has_ff_byte_classic);

static bool has_ff_byte_loop(const unsigned int value) noexcept
{
    // Iterate over each byte and check if it is equal to 0xFF
    for (size_t i = 0; i < sizeof(unsigned int); ++i)
    {
        if ((value & (0xFF << (8 * i))) == (0xFFU << (8 * i)))
        {
            return true;
        }
    }
    return false;
}
static void bm_has_ff_byte_loop(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(has_ff_byte_loop(0));
        benchmark::DoNotOptimize(has_ff_byte_loop(0xFF));
    }
}
BENCHMARK(bm_has_ff_byte_loop);

#if !defined(_M_ARM64)
static bool has_ff_byte_simd(const unsigned int value) noexcept {
     // Use SSE instructions for parallel comparison
     const __m128i xmm_value = _mm_set1_epi32(value);
     const __m128i xmm_ff = _mm_set1_epi32(0xFF);

     // Compare each byte for equality with 0xFF
     const __m128i comparison = _mm_cmpeq_epi8(xmm_value, xmm_ff);

     // Check if any comparison result is true
     return _mm_testz_si128(comparison, comparison) == 0;
 }
static void bm_has_ff_byte_simd(benchmark::State& state)
{
    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(has_ff_byte_simd(0));
        benchmark::DoNotOptimize(has_ff_byte_simd(0xFF));
    }
}
BENCHMARK(bm_has_ff_byte_simd);
#endif

static const std::byte* find_jpeg_marker_start_byte(const std::byte* position, const std::byte* end_position) noexcept
{
    constexpr std::byte jpeg_marker_start_byte{0xFF};

    // Use memchr to find next start byte (0xFF). memchr is optimized on some platforms to search faster.
    return static_cast<const std::byte*>(
        memchr(position, std::to_integer<int>(jpeg_marker_start_byte), end_position - position));
}
static void bm_find_jpeg_marker_start_byte(benchmark::State& state)
{
    const std::vector<std::byte> buffer(size_t{1024} * 1024 * 8 * 3);

    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(find_jpeg_marker_start_byte(buffer.data(), buffer.data() + buffer.size()));
    }
}
BENCHMARK(bm_find_jpeg_marker_start_byte);

// A simple overload with uint64_t\uint32_t doesn't work for macOS. size_t is not the same type as uint64_t.
template<int BitCount, typename T>
constexpr bool is_uint_v = sizeof(T) == BitCount / 8 && std::is_integral_v<T> && !std::is_signed_v<T>;

template<typename T>
[[nodiscard]]
auto byte_swap(const T value) noexcept
{
    if constexpr (is_uint_v<16, T>)
    {
#ifdef _MSC_VER
        return _byteswap_ushort(value);
#else
        // Note: GCC and Clang will optimize this pattern to a built-in intrinsic.
        return static_cast<uint16_t>(value << 8 | value >> 8);
#endif
    }
    else if constexpr (is_uint_v<32, T>)
    {
#ifdef _MSC_VER
        return _byteswap_ulong(value);
#else
        // Note: GCC and Clang will optimize this pattern to a built-in intrinsic.
        return value >> 24 | (value & 0x00FF0000) >> 8 | (value & 0x0000FF00) << 8 | value << 24;
#endif
    }
    else
    {
        static_assert(is_uint_v<64, T>);
#ifdef _MSC_VER
        return _byteswap_uint64(value);
#else
        // Note: GCC and Clang will optimize this pattern to a built-in intrinsic.
        return (value << 56) | ((value << 40) & 0x00FF'0000'0000'0000) | ((value << 24) & 0x0000'FF00'0000'0000) |
               ((value << 8) & 0x0000'00FF'0000'0000) | ((value >> 8) & 0x0000'0000'FF00'0000) |
               ((value >> 24) & 0x0000'0000'00FF'0000) | ((value >> 40) & 0x0000'0000'0000'FF00) | (value >> 56);
#endif
    }
}



template<typename T>
[[nodiscard]]
T read_unaligned(const void* buffer) noexcept
{
    // Note: MSVC, GCC and clang will replace this with a direct register read if the CPU architecture allows it
    // On x86, x64 and ARM64 this will just be 1 register load.
    T value;
    memcpy(&value, buffer, sizeof(T));
    return value;
}

template<typename T>
T read_big_endian_unaligned(const void* buffer) noexcept
{
#ifdef LITTLE_ENDIAN_ARCHITECTURE
    return byte_swap(read_unaligned<T>(buffer));
#else
    return read_unaligned<T>(buffer);
#endif
}

#if !defined(_M_ARM64)
static uint32_t read_all_bytes_with_ff_check(const std::byte* position, const std::byte* end_position) noexcept
{
    uint32_t result{};

    for (; position < end_position; position += sizeof(uint32_t))
    {
        if (const uint32_t value{read_big_endian_unaligned<uint32_t>(position)};
            has_ff_byte_simd(value))
        {
            result++;
        }
        else
        {
            result |= value;
        }

    }

    return result;
}
static void bm_read_all_bytes_with_ff_check(benchmark::State& state)
{
    const std::vector<std::byte> buffer(size_t{1024} * 1024 * 8 * 3);

    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(read_all_bytes_with_ff_check(buffer.data(), buffer.data() + buffer.size()));
    }
}
BENCHMARK(bm_read_all_bytes_with_ff_check);
#endif

#if !defined(_M_ARM64)
static bool has_ff_byte_simd64(const uint64_t value) noexcept
{
    // Use SSE instructions for parallel comparison
    const __m128i xmm_value = _mm_set1_epi64x(value);
    const __m128i xmm_ff = _mm_set1_epi32(0xFF);

    // Compare each byte for equality with 0xFF
    const __m128i comparison = _mm_cmpeq_epi8(xmm_value, xmm_ff);

    // Check if any comparison result is true
    return _mm_testz_si128(comparison, comparison) == 0;
}

static uint64_t read_all_bytes_with_ff_check64(const std::byte* position, const std::byte* end_position) noexcept
{
    uint64_t result{};

    for (; position < end_position; position += sizeof(uint64_t))
    {
        if (const uint64_t value{read_big_endian_unaligned<uint64_t>(position)}; has_ff_byte_simd64(value))
        {
            result++;
        }
        else
        {
            result |= value;
        }
    }

    return result;
}
static void bm_read_all_bytes_with_ff_check64(benchmark::State& state)
{
    const std::vector<std::byte> buffer(size_t{1024} * 1024 * 8 * 3);

    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(read_all_bytes_with_ff_check64(buffer.data(), buffer.data() + buffer.size()));
    }
}
BENCHMARK(bm_read_all_bytes_with_ff_check64);
#endif


static uint32_t read_all_bytes_no_check(const std::byte* position, const std::byte* end_position) noexcept
{
    uint32_t result{};

    for (; position < end_position; position += sizeof(uint32_t))
    {
        const uint32_t value{read_big_endian_unaligned<uint32_t>(position)};
        result |= value;
    }

    return result;
}
static void bm_read_all_bytes_no_check(benchmark::State& state)
{
    const std::vector<std::byte> buffer(size_t{1024} * 1024 * 8 * 3);

    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(read_all_bytes_no_check(buffer.data(), buffer.data() + buffer.size()));
    }
}
BENCHMARK(bm_read_all_bytes_no_check);

static uint64_t read_all_bytes_no_check64(const std::byte* position, const std::byte* end_position) noexcept
{
    uint64_t result{};

    for (; position < end_position; position += sizeof(uint64_t))
    {
        const uint64_t value{read_big_endian_unaligned<uint64_t>(position)};
        result |= value;
    }

    return result;
}
static void bm_read_all_bytes_no_check64(benchmark::State& state)
{
    const std::vector<std::byte> buffer(size_t{1024} * 1024 * 8 * 3);

    for (const auto _ : state)
    {
        benchmark::DoNotOptimize(read_all_bytes_no_check64(buffer.data(), buffer.data() + buffer.size()));
    }
}
BENCHMARK(bm_read_all_bytes_no_check64);

// Tips to run the benchmark tests:

// To run a single benchmark:
// benchmark --benchmark_filter=bm_decode   

BENCHMARK_MAIN();
