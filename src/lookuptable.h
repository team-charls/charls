// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include <cstring>
#include <array>


// Tables for fast decoding of short Golomb Codes.
struct Code final
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


class CTable final
{
public:
    static constexpr size_t byte_bit_count = 8;

    CTable() noexcept
    {
        std::memset(_types.data(), 0, sizeof(_types)); // TODO: analyze if needed
    }

    void AddEntry(uint8_t value, Code c) noexcept
    {
        const int32_t length = c.GetLength();
        ASSERT(static_cast<size_t>(length) <= byte_bit_count);

        for (int32_t i = 0; i < static_cast<int32_t>(1) << (byte_bit_count - length); ++i)
        {
            ASSERT(_types[(static_cast<size_t>(value) << (byte_bit_count - length)) + i].GetLength() == 0);
            _types[(static_cast<size_t>(value) << (byte_bit_count - length)) + i] = c;
        }
    }

    FORCE_INLINE const Code& Get(int32_t value) const noexcept
    {
        return _types[value];
    }

private:
    std::array<Code, 1 << byte_bit_count> _types;
};
