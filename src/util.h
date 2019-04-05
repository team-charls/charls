// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include <charls/public_types.h>
#include <vector>

// Use an uppercase alias for assert to make it clear that it is a pre-processor macro.
#define ASSERT(t) assert(t)

// Only use __forceinline for the Microsoft C++ compiler in release mode (verified scenario)
// Use the build-in optimizer for all other C++ compilers.
// Note: usage of FORCE_INLINE may be reduced in the future as the latest generation of C++ compilers
// can handle optimization by themselves.
#ifndef FORCE_INLINE
#  ifdef _MSC_VER
#    ifdef NDEBUG
#      define FORCE_INLINE __forceinline
#    else
#      define FORCE_INLINE
#    endif
#  else
#    define FORCE_INLINE
#  endif
#endif

#ifdef _MSC_VER
#define MSVC_WARNING_SUPPRESS(x) __pragma(warning(push)) __pragma(warning(disable : x))  // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))
#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
#endif

namespace charls
{

constexpr size_t int32_t_bit_count = sizeof(int32_t) * 8;


inline void push_back(std::vector<uint8_t>& values, uint16_t value)
{
    values.push_back(uint8_t(value / 0x100));
    values.push_back(uint8_t(value % 0x100));
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
    {}

    Triplet(int32_t x1, int32_t x2, int32_t x3) noexcept :
        v1(static_cast<T>(x1)),
        v2(static_cast<T>(x2)),
        v3(static_cast<T>(x3))
    {}

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
        return (buffer[0] << 24u) + (buffer[1] << 16u) + (buffer[2] << 8u) + (buffer[3] << 0u);
    }
};


template<>
struct FromBigEndian<8> final
{
    FORCE_INLINE static uint64_t Read(const uint8_t* buffer) noexcept
    {
        return (static_cast<uint64_t>(buffer[0]) << 56u) + (static_cast<uint64_t>(buffer[1]) << 48u) +
               (static_cast<uint64_t>(buffer[2]) << 40u) + (static_cast<uint64_t>(buffer[3]) << 32u) +
               (static_cast<uint64_t>(buffer[4]) << 24u) + (static_cast<uint64_t>(buffer[5]) << 16u) +
               (static_cast<uint64_t>(buffer[6]) <<  8u) + (static_cast<uint64_t>(buffer[7]) << 0u);
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
