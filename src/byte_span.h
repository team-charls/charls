// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstddef>

namespace charls {

/// <summary>Simplified span class as replacement for C++20 std::span<std::byte>.</summary>
struct byte_span final
{
    using iterator = std::byte*;
    using pointer = std::byte*;

    byte_span() = default;

    constexpr byte_span(void* data, const size_t size) noexcept :
        data_{static_cast<std::byte*>(data)}, size_{size}
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

private:
    std::byte* data_{};
    size_t size_{};
};


/// <summary>Simplified span class as replacement for C++20 std::span<const std::byte>.</summary>
class const_byte_span final
{
public:
    using iterator = const std::byte*;
    using pointer = const std::byte*;

    const_byte_span() = default;

    constexpr const_byte_span(const void* data, const size_t size) noexcept :
        data_{static_cast<const std::byte*>(data)}, size_{size}
    {
    }

    template<typename It>
    constexpr const_byte_span(It first, It last) noexcept : const_byte_span(first, last - first)
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

private:
    const std::byte* data_{};
    size_t size_{};
};

} // namespace charls
