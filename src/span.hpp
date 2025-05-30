// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "assert.hpp"

#include <array>
#include <cstddef>

namespace charls {

// Replacement for C++20 std::to_address, which is not available in C++17.
template<typename Ptr>
constexpr auto to_address(const Ptr& it)
{
    return &*it;
}


// Replacement for C++20 type std::span, which is not available in C++17.
template<typename T>
class span final
{
public:
    using iterator = T*;
    using pointer = T*;
    using size_type = size_t;

    span() = default;

    constexpr span(T* data, const size_t size) noexcept : data_{data}, size_{size}
    {
    }

    template<typename It>
    constexpr span(It first, It last) noexcept : span(first, static_cast<size_type>(last - first))
    {
    }

    // ReSharper disable once CppNonExplicitConvertingConstructor
    template<typename OtherType, size_t Size>
    constexpr span(const std::array<OtherType, Size>& data) noexcept : data_{data.data()}, size_{Size}
    {
    }

    [[nodiscard]]
    constexpr size_t size() const noexcept
    {
        return size_;
    }

    [[nodiscard]]
    constexpr size_t size_bytes() const noexcept
    {
        return sizeof(T) * size_;
    }

    [[nodiscard]]
    constexpr pointer data() const noexcept
    {
        return data_;
    }

    [[nodiscard]]
    constexpr iterator begin() const noexcept
    {
        return data_;
    }

    [[nodiscard]]
    constexpr iterator end() const noexcept
    {
        return data_ + size_;
    }

    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
        return size_ == 0;
    }

    [[nodiscard]]
    constexpr span subspan(const size_t offset) const noexcept
    {
        ASSERT(offset <= size_);
        return {data_ + offset, size_ - offset};
    }

private:
    pointer data_{};
    size_t size_{};
};

template<typename T>
span(T) -> span<T>;


template<typename T>
[[nodiscard]]
span<const std::byte> as_bytes(span<T> source) noexcept
{
    return {reinterpret_cast<const std::byte*>(source.data()), source.size_bytes()};
}

} // namespace charls
