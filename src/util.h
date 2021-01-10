// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/annotations.h>
#include <charls/jpegls_error.h>

#include "byte_span.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <type_traits>
#include <vector>


// Use an uppercase alias for assert to make it clear that ASSERT is a pre-processor macro.
#ifdef _MSC_VER
#define ASSERT(expression) \
    __pragma(warning(push)) __pragma(warning(disable : 26493)) assert(expression) __pragma(warning(pop))
#else
#define ASSERT(expression) assert(expression)
#endif

// Only use __forceinline for the Microsoft C++ compiler in release mode (verified scenario)
// Use the build-in optimizer for all other C++ compilers.
// Note: usage of FORCE_INLINE may be reduced in the future as the latest generation of C++ compilers
// can handle optimization by themselves.
#ifndef FORCE_INLINE
#ifdef _MSC_VER
#ifdef NDEBUG
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE
#endif
#else
#define FORCE_INLINE
#endif
#endif

#ifdef _MSC_VER
#define MSVC_WARNING_SUPPRESS(x) \
    __pragma(warning(push)) __pragma(warning(disable : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))

#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x) \
    __pragma(warning(suppress \
                     : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses, cppcoreguidelines-macro-usage)

// Visual Studio 2015 supports C++14, but not all constexpr scenarios. VS 2017 has full C++14 support.
#if _MSC_VER >= 1910
#define CONSTEXPR constexpr
#else
#define CONSTEXPR inline
#endif

#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x)
#define CONSTEXPR constexpr
#endif

namespace charls {

inline jpegls_errc to_jpegls_errc() noexcept
{
    try
    {
        // re-trow the exception.
        throw;
    }
    catch (const jpegls_error& error)
    {
        return static_cast<jpegls_errc>(error.code().value());
    }
    catch (const std::bad_alloc&)
    {
        return jpegls_errc::not_enough_memory;
    }
    catch (...)
    {
        return jpegls_errc::unexpected_failure;
    }
}

inline void clear_error_message(OUT_OPT_ char* error_message) noexcept
{
    if (error_message)
    {
        error_message[0] = 0;
    }
}


/// <summary>
/// Cross platform safe version of strcpy.
/// </summary>
inline void string_copy(IN_Z_ const char* source, OUT_WRITES_Z_(size_in_bytes) char* destination,
                        const size_t size_in_bytes) noexcept
{
    ASSERT(strlen(source) < size_in_bytes && "String will be truncated");

#if defined(__STDC_SECURE_LIB__) && defined(__STDC_WANT_SECURE_LIB__) && __STDC_WANT_SECURE_LIB__ == 1
    constexpr size_t truncate{static_cast<size_t>(-1)};
    strncpy_s(destination, size_in_bytes, source, truncate);
#else
    strncpy(destination, source, size_in_bytes);
    destination[size_in_bytes - 1] = 0;
#endif
}

inline jpegls_errc set_error_message(const jpegls_errc error, OUT_WRITES_Z_(ErrorMessageSize) char* error_message) noexcept
{
    if (error_message)
    {
        string_copy(charls_get_error_message(error), error_message, ErrorMessageSize);
    }

    return error;
}


constexpr size_t int32_t_bit_count = sizeof(int32_t) * 8;


inline void push_back(std::vector<uint8_t>& values, const uint16_t value)
{
    values.push_back(static_cast<uint8_t>(value >> 8));
    values.push_back(static_cast<uint8_t>(value));
}


inline void push_back(std::vector<uint8_t>& values, const uint32_t value)
{
    values.push_back(static_cast<uint8_t>(value >> 24));
    values.push_back(static_cast<uint8_t>(value >> 16));
    values.push_back(static_cast<uint8_t>(value >> 8));
    values.push_back(static_cast<uint8_t>(value));
}


CONSTEXPR int32_t log_2(const int32_t n) noexcept
{
    int32_t x{};
    while (n > (1 << x))
    {
        ++x;
    }
    return x;
}


constexpr int32_t sign(const int32_t n) noexcept
{
    return (n >> (int32_t_bit_count - 1)) | 1;
}


constexpr int32_t bit_wise_sign(const int32_t i) noexcept
{
    return i >> (int32_t_bit_count - 1);
}


/// <summary>
/// Computes the parameter RANGE. When NEAR = 0, RANGE = MAXVAL + 1. (see ISO/IEC 14495-1, A.2.1)
/// </summary>
constexpr int32_t compute_range_parameter(const int32_t maximum_sample_value, const int32_t near_lossless) noexcept
{
    return (maximum_sample_value + 2 * near_lossless) / (2 * near_lossless + 1) + 1;
}


/// <summary>
/// Computes the parameter LIMIT. (see ISO/IEC 14495-1, A.2.1)
/// </summary>
constexpr int32_t compute_limit_parameter(const int32_t bits_per_pixel)
{
    return 2 * (bits_per_pixel + std::max(8, bits_per_pixel));
}


template<typename SampleType>
struct triplet
{
    triplet() noexcept : v1{0}, v2{0}, v3{0}
    {
    }

    triplet(int32_t x1, int32_t x2, int32_t x3) noexcept :
        v1(static_cast<SampleType>(x1)), v2(static_cast<SampleType>(x2)), v3(static_cast<SampleType>(x3))
    {
    }

    union
    {
        SampleType v1;
        SampleType R;
    };
    union
    {
        SampleType v2;
        SampleType G;
    };
    union
    {
        SampleType v3;
        SampleType B;
    };
};


inline bool operator==(const triplet<uint8_t>& lhs, const triplet<uint8_t>& rhs) noexcept
{
    return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3;
}


inline bool operator!=(const triplet<uint8_t>& lhs, const triplet<uint8_t>& rhs) noexcept
{
    return !(lhs == rhs);
}


template<typename SampleType>
struct quad final : triplet<SampleType>
{
    MSVC_WARNING_SUPPRESS(26495) // false warning that v4 is uninitialized [VS 2017 15.9.4]
    quad() noexcept : triplet<SampleType>(), v4{0}
    {
    }
    MSVC_WARNING_UNSUPPRESS()

    MSVC_WARNING_SUPPRESS(26495) // false warning that v4 is uninitialized [VS 2017 15.9.4]
    quad(triplet<SampleType> triplet_value, int32_t alpha) noexcept :
        triplet<SampleType>(triplet_value), A(static_cast<SampleType>(alpha))
    {
    }
    MSVC_WARNING_UNSUPPRESS()

    union
    {
        SampleType v4;
        SampleType A;
    };
};


template<int Size>
struct from_big_endian final
{
};


template<>
struct from_big_endian<4> final
{
    FORCE_INLINE static unsigned int read(const uint8_t* buffer) noexcept
    {
        return (static_cast<uint32_t>(buffer[0]) << 24U) + (static_cast<uint32_t>(buffer[1]) << 16U) +
               (static_cast<uint32_t>(buffer[2]) << 8U) + (static_cast<uint32_t>(buffer[3]) << 0U);
    }
};


template<>
struct from_big_endian<8> final
{
    FORCE_INLINE static uint64_t read(const uint8_t* buffer) noexcept
    {
        return (static_cast<uint64_t>(buffer[0]) << 56U) + (static_cast<uint64_t>(buffer[1]) << 48U) +
               (static_cast<uint64_t>(buffer[2]) << 40U) + (static_cast<uint64_t>(buffer[3]) << 32U) +
               (static_cast<uint64_t>(buffer[4]) << 24U) + (static_cast<uint64_t>(buffer[5]) << 16U) +
               (static_cast<uint64_t>(buffer[6]) << 8U) + (static_cast<uint64_t>(buffer[7]) << 0U);
    }
};


inline void skip_bytes(byte_span& stream_info, const size_t count) noexcept
{
    stream_info.data += count;
    stream_info.size -= count;
}


template<typename T>
std::ostream& operator<<(typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream, const T& e)
{
    return stream << static_cast<typename std::underlying_type<T>::type>(e);
}


template<typename T>
T* check_pointer(T* pointer)
{
    if (!pointer)
    {
        impl::throw_jpegls_error(jpegls_errc::invalid_argument);
    }

    return pointer;
}


CONSTEXPR uint32_t calculate_maximum_sample_value(const int32_t bits_per_sample)
{
    ASSERT(bits_per_sample > 0 && bits_per_sample <= 16);
    return (1U << bits_per_sample) - 1;
}


/// <summary>
/// Computes how many bytes are needed to hold the number of bits.
/// </summary>
constexpr uint32_t bit_to_byte_count(const int32_t bit_count) noexcept
{
    return static_cast<uint32_t>((bit_count + 7) / 8);
}


template<typename Enum>
constexpr auto to_underlying_type(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

} // namespace charls
