//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//


#ifndef CHARLS_UTIL
#define CHARLS_UTIL

#include "publictypes.h"
#include <vector>
#include <system_error>
#include <memory>
#include <cassert>

// Use an uppercase alias for assert to make it clear that it is a pre-processor macro.
#define ASSERT(t) assert(t)

// Only use __forceinline for the Microsoft C++ compiler in release mode (verified scenario)
// Use the build-in optimizer for all other C++ compilers.
// Note: usage of force_inline may be reduced in the future as the latest generation of C++ compilers
// can handle optimization by themselves.
#ifndef force_inline
#  ifdef _MSC_VER
#    ifdef NDEBUG
#      define force_inline __forceinline
#    else
#      define force_inline
#    endif
#  else
#    define force_inline
#  endif
#endif


enum constants
{
    INT32_BITCOUNT = sizeof(int32_t) * 8
};


inline void push_back(std::vector<uint8_t>& values, uint16_t value)
{
    values.push_back(uint8_t(value / 0x100));
    values.push_back(uint8_t(value % 0x100));
}


inline int32_t log_2(int32_t n)
{
    int32_t x = 0;
    while (n > (int32_t(1) << x))
    {
        ++x;
    }
    return x;
}


inline int32_t Sign(int32_t n)
{
    return (n >> (INT32_BITCOUNT - 1)) | 1;
}


inline int32_t BitWiseSign(int32_t i)
{
    return i >> (INT32_BITCOUNT - 1);
}


template<typename SAMPLE>
struct Triplet
{
    Triplet() :
        v1(0),
        v2(0),
        v3(0)
    {}

    Triplet(int32_t x1, int32_t x2, int32_t x3) :
        v1(static_cast<SAMPLE>(x1)),
        v2(static_cast<SAMPLE>(x2)),
        v3(static_cast<SAMPLE>(x3))
    {}

    union
    {
        SAMPLE v1;
        SAMPLE R;
    };
    union
    {
        SAMPLE v2;
        SAMPLE G;
    };
    union
    {
        SAMPLE v3;
        SAMPLE B;
    };
};


inline bool operator==(const Triplet<uint8_t>& lhs, const Triplet<uint8_t>& rhs)
{
    return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3;
}


inline bool operator!=(const Triplet<uint8_t>& lhs, const Triplet<uint8_t>& rhs)
{
    return !(lhs == rhs);
}


template<typename sample>
struct Quad : Triplet<sample>
{
    Quad() :
        v4(0)
        {}

    Quad(Triplet<sample> triplet, int32_t alpha) : Triplet<sample>(triplet), A(static_cast<sample>(alpha))
        {}

    union
    {
        sample v4;
        sample A;
    };
};


template<int size>
struct FromBigEndian
{
};


template<>
struct FromBigEndian<4>
{
    force_inline static unsigned int Read(const uint8_t* pbyte)
    {
        return (pbyte[0] << 24) + (pbyte[1] << 16) + (pbyte[2] << 8) + (pbyte[3] << 0);
    }
};


template<>
struct FromBigEndian<8>
{
    force_inline static uint64_t Read(const uint8_t* pbyte)
    {
        return (static_cast<uint64_t>(pbyte[0]) << 56) + (static_cast<uint64_t>(pbyte[1]) << 48) +
               (static_cast<uint64_t>(pbyte[2]) << 40) + (static_cast<uint64_t>(pbyte[3]) << 32) +
               (static_cast<uint64_t>(pbyte[4]) << 24) + (static_cast<uint64_t>(pbyte[5]) << 16) +
               (static_cast<uint64_t>(pbyte[6]) <<  8) + (static_cast<uint64_t>(pbyte[7]) << 0);
    }
};


class charls_error : public std::system_error
{
public:
    explicit charls_error(charls::ApiResult errorCode)
        : system_error(static_cast<int>(errorCode), CharLSCategoryInstance())
    {
    }


    charls_error(charls::ApiResult errorCode, const std::string& message)
        : system_error(static_cast<int>(errorCode), CharLSCategoryInstance(), message)
    {
    }

private:
    static const std::error_category& CharLSCategoryInstance();
};


inline void SkipBytes(ByteStreamInfo& streamInfo, std::size_t count)
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

#endif
