//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//


#ifndef CHARLS_LOOKUP_TABLE
#define CHARLS_LOOKUP_TABLE


#include <cstring>


// Tables for fast decoding of short Golomb Codes.
struct Code
{
    Code() noexcept :
        _value(),
        _length()
    {
    }

    Code(int32_t value, int32_t length) noexcept :
        _value(value),
        _length(length)
    {
    }

    int32_t GetValue() const noexcept
    {
        return _value;
    }

    int32_t GetLength() const noexcept
    {
        return _length;
    }

    int32_t _value;
    int32_t _length;
};


class CTable
{
public:
    static constexpr size_t byte_bit_count = 8;

    CTable() noexcept
    {
        std::memset(_types, 0, sizeof(_types));
    }

    void AddEntry(uint8_t value, Code c) noexcept
    {
        const int32_t length = c.GetLength();
        ASSERT(static_cast<size_t>(length) <= byte_bit_count);

        for (int32_t i = 0; i < static_cast<int32_t>(1) << (byte_bit_count - length); ++i)
        {
            ASSERT(_types[(value << (byte_bit_count - length)) + i].GetLength() == 0);
            _types[(value << (byte_bit_count - length)) + i] = c;
        }
    }

    FORCE_INLINE const Code& Get(int32_t value) const noexcept
    {
        return _types[value];
    }

private:
    Code _types[1 << byte_bit_count];
};


#endif
