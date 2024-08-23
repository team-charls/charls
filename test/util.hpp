// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/charls.hpp>

#include <exception>
#include <istream>
#include <ostream>
#include <vector>

struct rect_size final
{
    rect_size(const size_t width, const size_t height) noexcept : cx(width), cy(height)
    {
    }
    size_t cx;
    size_t cy;
};


std::ofstream open_output_stream(const char* filename);
void fix_endian(std::vector<std::byte>* buffer, bool little_endian_data) noexcept;
std::vector<std::byte> read_file(const char* filename, long offset = 0, size_t bytes = 0);
void write_file(const char* filename, const void* data, size_t size);
void test_file(const char* filename, int offset, rect_size size2, int bits_per_sample, int component_count,
               bool little_endian_file = false, int loop_count = 1);
void test_round_trip(const char* name, const std::vector<std::byte>& original_buffer, rect_size size, int bits_per_sample,
                     int component_count, int loop_count = 1);
void test_portable_anymap_file(const char* filename, int loop_count = 1);

/// <summary>
/// Computes how many bytes are needed to hold the number of bits.
/// </summary>
constexpr uint32_t bit_to_byte_count(const int32_t bit_count) noexcept
{
    return static_cast<uint32_t>((bit_count + 7) / 8);
}

template<typename Container>
void read(std::istream& input, Container& destination)
{
    input.read(reinterpret_cast<char*>(destination.data()), static_cast<std::streamsize>(destination.size()));
}

template<typename Container>
void write(std::ostream& output, const Container& source, const size_t size)
{
    output.write(reinterpret_cast<const char*>(source.data()), static_cast<std::streamsize>(size));
}


class unit_test_exception final : public std::exception
{
};


namespace assert {

inline void is_true(const bool condition)
{
    if (!condition)
        throw unit_test_exception();
}

}

#ifdef _MSC_VER
#define MSVC_WARNING_SUPPRESS(x) \
    __pragma(warning(push)) __pragma(warning(disable : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))
#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x) \
    __pragma(warning(suppress \
                     : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses, cppcoreguidelines-macro-usage)
#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x)
#endif
