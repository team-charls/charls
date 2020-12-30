// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/charls.h>

#include <exception>
#include <vector>

struct rect_size final
{
    rect_size(const size_t width, const size_t height) noexcept : cx(width), cy(height)
    {
    }
    size_t cx;
    size_t cy;
};


void fix_endian(std::vector<uint8_t>* buffer, bool little_endian_data) noexcept;
std::vector<uint8_t> read_file(const char* filename, long offset = 0, size_t bytes = 0);
void write_file(const char* filename, const void* data, size_t size);
void test_file(const char* filename, int offset, rect_size size2, int bits_per_sample, int component_count,
               bool little_endian_file = false, int loop_count = 1);
void test_round_trip(const char* name, const std::vector<uint8_t>& decoded_buffer, rect_size size, int bits_per_sample,
                     int component_count, int loop_count = 1);
void test_round_trip(const char* name, const std::vector<uint8_t>& original_buffer, const JlsParameters& params,
                     int loop_count = 1);
void test_portable_anymap_file(const char* filename, int loop_count = 1);

/// <summary>
/// Computes how many bytes are needed to hold the number of bits.
/// </summary>
constexpr uint32_t bit_to_byte_count(const int32_t bit_count) noexcept
{
    return static_cast<uint32_t>((bit_count + 7) / 8);
}

class unit_test_exception final : public std::exception
{
public:
    explicit unit_test_exception() = default;
};

class assert final
{
public:
    static void is_true(const bool condition)
    {
        if (!condition)
            throw unit_test_exception();
    }
};

#ifdef _MSC_VER
#define MSVC_WARNING_SUPPRESS(x) \
    __pragma(warning(push)) __pragma(warning(disable : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))
#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x) \
    __pragma(warning(suppress \
                     : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses, cppcoreguidelines-macro-usage)
#define MSVC_CONST const
#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x)
#define MSVC_CONST
#endif

// clang-format off
#ifdef __clang__
#define DISABLE_DEPRECATED_WARNING \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#elif defined(__GNUC__) || defined(__GNUG__)
#define DISABLE_DEPRECATED_WARNING \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#elif defined(_MSC_VER)
#define DISABLE_DEPRECATED_WARNING \
    __pragma(warning(push)) \
    __pragma(warning(disable : 4996)) // was declared deprecated
#else
#define DISABLE_DEPRECATED_WARNING
#endif
// clang-format on

#ifdef __clang__
#define RESTORE_DEPRECATED_WARNING _Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
#define RESTORE_DEPRECATED_WARNING _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#define RESTORE_DEPRECATED_WARNING __pragma(warning(pop))
#else
#define RESTORE_DEPRECATED_WARNING
#endif
