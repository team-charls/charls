// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstddef>

#include "util.h"

namespace charls {

/// <summary>Simplified span class as temporarily replacement for C++20 std::span<std::byte>.</summary>
template<class T>
class span final
{
public:
    using iterator = T*;
    using pointer = T*;

    span() = default;

    constexpr span(T* data, const size_t size) noexcept : data_{data}, size_{size}
    {
    }

    template<typename It>
    constexpr span(It first, It last) noexcept : span(first, last - first)
    {
    }

    [[nodiscard]] constexpr size_t size() const noexcept
    {
        return size_;
    }

    [[nodiscard]] constexpr pointer data() const noexcept
    {
        return data_;
    }

    [[nodiscard]] constexpr iterator begin() const noexcept
    {
        return data_;
    }

    [[nodiscard]] constexpr iterator end() const noexcept
    {
        return data_ + size_;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size_ == 0;
    }

    [[nodiscard]] constexpr span subspan(const size_t offset) const noexcept
    {
        ASSERT(offset <= size_);
        return {data_ + offset, size_ - offset};
    }

private:
    pointer data_{};
    size_t size_{};
};

} // namespace charls
