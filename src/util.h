// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/charls_legacy.h>
#include <charls/jpegls_error.h>

#include <cassert>
#include <cstring>
#include <vector>

// Use an uppercase alias for assert to make it clear that it is a pre-processor macro.
#define ASSERT(t) assert(t)

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
#define MSVC_WARNING_SUPPRESS(x) __pragma(warning(push)) \
    __pragma(warning(disable                             \
                     : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))
#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
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

inline void clear_error_message(char* errorMessage) noexcept
{
    if (errorMessage)
    {
        errorMessage[0] = 0;
    }
}


/// <summary>
/// Cross platform safe version of strcpy.
/// </summary>
inline void string_copy(const char* source, char* destination, const size_t size_in_bytes) noexcept
{
    ASSERT(strlen(source) < size_in_bytes && "String will be truncated");

#if defined(__STDC_SECURE_LIB__) && defined(__STDC_WANT_SECURE_LIB__) && __STDC_WANT_SECURE_LIB__ == 1
    strncpy_s(destination, size_in_bytes, source, _TRUNCATE);
#else
    strncpy(destination, source, size_in_bytes);
    destination[size_in_bytes - 1] = 0;
#endif
}

inline jpegls_errc set_error_message(const jpegls_errc error, char* error_message) noexcept
{
    if (error_message)
    {
        string_copy(charls_get_error_message(error), error_message, ErrorMessageSize);
    }

    return error;
}


constexpr size_t int32_t_bit_count = sizeof(int32_t) * 8;


inline void push_back(std::vector<uint8_t>& values, uint16_t value)
{
    values.push_back(static_cast<uint8_t>(value >> 8));
    values.push_back(static_cast<uint8_t>(value));
}


inline void push_back(std::vector<uint8_t>& values, uint32_t value)
{
    values.push_back(static_cast<uint8_t>(value >> 24));
    values.push_back(static_cast<uint8_t>(value >> 16));
    values.push_back(static_cast<uint8_t>(value >> 8));
    values.push_back(static_cast<uint8_t>(value));
}


constexpr int32_t log_2(int32_t n) noexcept
{
    int32_t x = 0;
    while (n > (1 << x))
    {
        ++x;
    }
    return x;
}


constexpr int32_t Sign(int32_t n) noexcept
{
    return (n >> (int32_t_bit_count - 1)) | 1;
}


constexpr int32_t BitWiseSign(int32_t i) noexcept
{
    return i >> (int32_t_bit_count - 1);
}


template<typename T>
struct Triplet
{
    Triplet() noexcept :
        v1(0),
        v2(0),
        v3(0)
    {
    }

    Triplet(int32_t x1, int32_t x2, int32_t x3) noexcept :
        v1(static_cast<T>(x1)),
        v2(static_cast<T>(x2)),
        v3(static_cast<T>(x3))
    {
    }

    union
    {
        T v1;
        T R;
    };
    union
    {
        T v2;
        T G;
    };
    union
    {
        T v3;
        T B;
    };
};


inline bool operator==(const Triplet<uint8_t>& lhs, const Triplet<uint8_t>& rhs) noexcept
{
    return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3;
}


inline bool operator!=(const Triplet<uint8_t>& lhs, const Triplet<uint8_t>& rhs) noexcept
{
    return !(lhs == rhs);
}


template<typename sample>
struct Quad final : Triplet<sample>
{
    MSVC_WARNING_SUPPRESS(26495) // false warning that v4 is uninitialized [VS 2017 15.9.4]
    Quad() noexcept :
        Triplet<sample>(),
        v4(0)
    {
    }
    MSVC_WARNING_UNSUPPRESS()

    MSVC_WARNING_SUPPRESS(26495) // false warning that v4 is uninitialized [VS 2017 15.9.4]
    Quad(Triplet<sample> triplet, int32_t alpha) noexcept :
        Triplet<sample>(triplet),
        A(static_cast<sample>(alpha))
    {
    }
    MSVC_WARNING_UNSUPPRESS()

    union
    {
        sample v4;
        sample A;
    };
};


template<int size>
struct FromBigEndian final
{
};


template<>
struct FromBigEndian<4> final
{
    FORCE_INLINE static unsigned int Read(const uint8_t* buffer) noexcept
    {
        return (static_cast<uint32_t>(buffer[0]) << 24U) + (static_cast<uint32_t>(buffer[1]) << 16U) +
               (static_cast<uint32_t>(buffer[2]) << 8U) + (static_cast<uint32_t>(buffer[3]) << 0U);
    }
};


template<>
struct FromBigEndian<8> final
{
    FORCE_INLINE static uint64_t Read(const uint8_t* buffer) noexcept
    {
        return (static_cast<uint64_t>(buffer[0]) << 56U) + (static_cast<uint64_t>(buffer[1]) << 48U) +
               (static_cast<uint64_t>(buffer[2]) << 40U) + (static_cast<uint64_t>(buffer[3]) << 32U) +
               (static_cast<uint64_t>(buffer[4]) << 24U) + (static_cast<uint64_t>(buffer[5]) << 16U) +
               (static_cast<uint64_t>(buffer[6]) << 8U) + (static_cast<uint64_t>(buffer[7]) << 0U);
    }
};


inline void SkipBytes(ByteStreamInfo& streamInfo, std::size_t count) noexcept
{
    if (!streamInfo.rawData)
        return;

    streamInfo.rawData += count;
    streamInfo.count -= count;
}


template<typename T>
std::ostream& operator<<(typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream, const T& e)
{
    return stream << static_cast<typename std::underlying_type<T>::type>(e);
}

} // namespace charls
