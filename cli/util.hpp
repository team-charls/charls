// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/charls.hpp>

#include <fstream>
#include <istream>
#include <ostream>
#include <vector>
#include <cstddef>

[[nodiscard]]
std::ofstream open_output_stream(const char* filename);

void fix_endian(std::vector<std::byte>* buffer, bool little_endian_data) noexcept;

[[nodiscard]]
std::vector<std::byte> read_file(const char* filename);

void write_file(const char* filename, const void* data, size_t size);

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


#ifdef _MSC_VER
#define MSVC_WARNING_SUPPRESS(x) \
    __pragma(warning(push)) __pragma(warning(disable : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))
#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x) \
    __pragma( \
        warning(suppress : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses, cppcoreguidelines-macro-usage)

// C26493 = Don't use C-style casts
#define ASSERT(expression) \
    __pragma(warning(push)) __pragma(warning(disable : 26493)) assert(expression) __pragma(warning(pop))
#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x)
#define ASSERT(expression) assert(expression)
#endif
