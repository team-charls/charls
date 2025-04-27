// SPDX-FileCopyrightText: © 2009 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/jpegls_error.hpp>

#include "span.hpp"

#include <cstdlib>
#include <cstring>
#include <type_traits>

// Use forced inline for supported C++ compilers in release builds.
// Note: usage of FORCE_INLINE may be reduced in the future as the latest generation of C++ compilers
// can handle optimization by themselves.
#ifndef FORCE_INLINE
#ifdef NDEBUG
#ifdef _MSC_VER
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__)
// C++ Compilers that support the GCC extensions (GCC, clang, Intel, etc.)
#define FORCE_INLINE __attribute__((always_inline))
#else
// Unknown C++ compiler, fallback to auto inline.
#define FORCE_INLINE
#endif
#else
// Do not force inline in debug builds.
#define FORCE_INLINE
#endif
#endif


#ifdef _MSC_VER

#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x) \
    __pragma(warning(suppress \
                     : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses, cppcoreguidelines-macro-usage)

// Helper macro for SAL annotations.
#define USE_DECL_ANNOTATIONS _Use_decl_annotations_

#else

#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x)
#define USE_DECL_ANNOTATIONS

#endif

// C++20 has support for [[likely]] and [[unlikely]]. Use for now the GCC\Clang extension.
// MSVC has in C++17 mode no alternative for it.
#ifdef __GNUC__
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define UNLIKELY(x) (x)
#endif

// C++20 provides std::endian, use for now compiler macros.
#ifdef _MSC_VER
#define LITTLE_ENDIAN_ARCHITECTURE // MSVC++ compiler support only little endian platforms.
#elif __GNUC__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LITTLE_ENDIAN_ARCHITECTURE
#endif
#else
#error "Unknown compiler"
#endif

// Turn A into a string literal without expanding macro definitions
// (however, if invoked from a macro, macro arguments are expanded).
#define TO_STRING_NX(A) #A // NOLINT(cppcoreguidelines-macro-usage)

// Turn A into a string literal after macro-expanding it.
#define TO_STRING(A) TO_STRING_NX(A) // NOLINT(cppcoreguidelines-macro-usage)


namespace charls {

inline CHARLS_NO_INLINE jpegls_errc to_jpegls_errc() noexcept
{
    try
    {
        // re-throw the exception.
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

    // Don't catch exceptions that are not expected: it is safer to terminate then continue in an unknown state.
}


template<typename T>
struct pair final
{
    T v1{};
    T v2{};

    [[nodiscard]]
    friend constexpr bool
    operator==(const pair& lhs, const pair& rhs) noexcept
    {
        return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2;
    }
};


template<typename T>
struct triplet final
{
    T v1{};
    T v2{};
    T v3{};

    [[nodiscard]]
    friend constexpr bool
    operator==(const triplet& lhs, const triplet& rhs) noexcept
    {
        return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3;
    }
};


template<typename T>
struct quad final
{
    T v1{};
    T v2{};
    T v3{};
    T v4{};

    [[nodiscard]]
    friend constexpr bool
    operator==(const quad& lhs, const quad& rhs) noexcept
    {
        return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3 && lhs.v4 == rhs.v4;
    }
};


template<typename Callback>
struct callback_function final
{
    Callback handler;
    void* user_context;
};


// C++23 comes with std::byteswap. Use our own byte_swap implementation for now.

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

#ifdef __EMSCRIPTEN__

// Note: WebAssembly (emcc 3.1.1) will fail with the default read_unaligned.

template<typename T>
T read_big_endian_unaligned(const void* /*buffer*/) noexcept;

template<>
inline uint16_t read_big_endian_unaligned<uint16_t>(const void* buffer) noexcept
{
    const uint8_t* p{static_cast<const uint8_t*>(buffer)};
    return (static_cast<uint32_t>(p[0]) << 8U) + static_cast<uint32_t>(p[1]);
}

template<>
inline uint32_t read_big_endian_unaligned<uint32_t>(const void* buffer) noexcept
{
    const uint8_t* p{static_cast<const uint8_t*>(buffer)};
    return (static_cast<uint32_t>(p[0]) << 24U) + (static_cast<uint32_t>(p[1]) << 16U) +
           (static_cast<uint32_t>(p[2]) << 8U) + static_cast<uint32_t>(p[3]);
}

template<>
inline size_t read_big_endian_unaligned<size_t>(const void* buffer) noexcept
{
    static_assert(sizeof(size_t) == sizeof(uint32_t), "wasm32 only");
    return read_big_endian_unaligned<uint32_t>(buffer);
}

#else

template<typename T>
T read_big_endian_unaligned(const void* buffer) noexcept
{
#ifdef LITTLE_ENDIAN_ARCHITECTURE
    return byte_swap(read_unaligned<T>(buffer));
#else
    return read_unaligned<T>(buffer);
#endif
}

#endif


template<typename T>
T* check_pointer(T* pointer)
{
    if (UNLIKELY(!pointer))
        impl::throw_jpegls_error(jpegls_errc::invalid_argument);

    return pointer;
}


/// <summary>
/// Validates the boolean 'expression'.
/// </summary>
/// <exception cref="charls::jpegls_error">Throws jpegls_errc::invalid_operation if 'expression' is false.</exception>
inline void check_operation(const bool expression)
{
    if (UNLIKELY(!expression))
        impl::throw_jpegls_error(jpegls_errc::invalid_operation);
}


/// <summary>
/// Validates the boolean 'expression'.
/// </summary>
/// <exception cref="charls::jpegls_error">Throws jpegls_errc if 'expression' is false.</exception>
inline void check_argument(const bool expression, const jpegls_errc error_value = jpegls_errc::invalid_argument)
{
    if (UNLIKELY(!expression))
        impl::throw_jpegls_error(error_value);
}


template<typename T>
void check_argument(span<T> argument, const jpegls_errc error_value = jpegls_errc::invalid_argument)
{
    check_argument(argument.data() != nullptr || argument.empty(), error_value);
}


template<typename T>
void check_argument_range(const T minimum, const T maximum, const T value,
                          const jpegls_errc error_value = jpegls_errc::invalid_argument)
{
    if (UNLIKELY(!(minimum <= value && value <= maximum)))
        impl::throw_jpegls_error(error_value);
}


inline void check_interleave_mode(const interleave_mode mode, const jpegls_errc error_value)
{
    if (UNLIKELY(!(mode == interleave_mode::none || mode == interleave_mode::line || mode == interleave_mode::sample)))
        impl::throw_jpegls_error(error_value);
}


/// <summary>
/// Converts an enumeration to its underlying type. Equivalent to C++23 std::to_underlying
/// </summary>
template<typename Enum>
constexpr auto to_underlying_type(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

#ifdef _MSC_VER
#if defined(_M_X64) || defined(_M_ARM64)
/// <summary>
/// Custom implementation of C++20 std::countl_zero (for uint64_t)
/// </summary>
inline int countl_zero(const uint64_t value) noexcept
{
    if (value == 0)
        return 64;

    unsigned long index;
    _BitScanReverse64(&index, value);
    return static_cast<int>(63U - index);
}
#endif

/// <summary>
/// Custom implementation of C++20 std::countl_zero (for uint32_t)
/// </summary>
inline int countl_zero(const uint32_t value) noexcept
{
    if (value == 0)
        return 32;

    unsigned long index;
    _BitScanReverse(&index, value);

    return static_cast<int>(31U - index);
}
#endif

#ifdef __GNUC__

// A simple overload with uint64_t\uint32_t doesn't work for macOS. size_t is not the same type as uint64_t.

/// <summary>
/// Custom implementation of C++20 std::countl_zero (for uint64_t)
/// </summary>
template<typename T>
auto countl_zero(const T value) noexcept -> std::enable_if_t<is_uint_v<64, T>, int>
{
    if (value == 0)
        return 64;

    return __builtin_clzll(value);
}

/// <summary>
/// Custom implementation of C++20 std::countl_zero (for uint32_t)
/// </summary>
template<typename T>
auto countl_zero(const T value) noexcept -> std::enable_if_t<is_uint_v<32, T>, int>
{
    if (value == 0)
        return 32;

    return __builtin_clz(value);
}

#endif


#if INTPTR_MAX == INT64_MAX
constexpr size_t checked_mul(const size_t a, const size_t b) noexcept
{
    return a * b;
}
#elif INTPTR_MAX == INT32_MAX
inline size_t checked_mul(const size_t a, const size_t b)
{
    const size_t result{a * b};
    if (UNLIKELY(result < a || result < b)) // check for unsigned integer overflow.
        impl::throw_jpegls_error(jpegls_errc::parameter_value_not_supported);
    return result;
}
#endif

// Replacement for std::unreachable (introduced in C++23).
MSVC_WARNING_SUPPRESS_NEXT_LINE(26497) // method cannot be constexpr
[[noreturn]]
inline void unreachable() noexcept
{
#ifdef __GNUC__ // GCC, Clang, ICC
    __builtin_unreachable();
#endif

#ifdef _MSC_VER
    __assume(false);
#endif
}

} // namespace charls
