// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstddef>

namespace charls {

/// <summary>Simplified span class as replacement for C++20 std::span<std::byte>.</summary>
struct byte_span final
{
    using iterator = const std::byte*;

    byte_span() = default;

    constexpr byte_span(void* data_arg, const size_t size_arg) noexcept :
        data{static_cast<std::byte*>(data_arg)}, size{size_arg}
    {
    }

    constexpr byte_span(const void* data_arg, const size_t size_arg) noexcept :
        data{static_cast<std::byte*>(const_cast<void*>(data_arg))}, size{size_arg}
    {
    }

    [[nodiscard]] constexpr iterator begin() const noexcept
    {
        return data;
    }

    [[nodiscard]] constexpr iterator end() const noexcept
    {
        return data + size;
    }

    std::byte* data{};
    size_t size{};
};


/// <summary>Simplified span class as replacement for C++20 std::span<const std::byte>.</summary>
class const_byte_span final
{
public:
    using iterator = const std::byte*;

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

    [[nodiscard]] constexpr const std::byte* data() const noexcept
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
