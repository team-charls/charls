// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstddef>
#include <cstdint>

/// <summary>Simplified span class for C++14 (std::span<std::byte>).</summary>
struct byte_span final
{
    byte_span() = default;

    byte_span(void* data_arg, const size_t size_arg) noexcept : data{static_cast<uint8_t*>(data_arg)}, size{size_arg}
    {
    }

    byte_span(const void* data_arg, const size_t size_arg) noexcept :
        data{static_cast<uint8_t*>(const_cast<void*>(data_arg))}, size{size_arg}
    {
    }

    uint8_t* data{};
    size_t size{};
};


/// <summary>Simplified span class for C++14 (std::span<const std::byte>).</summary>
class const_byte_span final
{
public:
    const_byte_span(const void* data, const size_t size) noexcept :
        data_{static_cast<const uint8_t*>(data)}, size_{size}
    {
    }

    constexpr size_t size() const noexcept
    {
        return size_;
    }

    constexpr const uint8_t* data() const noexcept
    {
        return data_;
    }

private:
    const uint8_t* data_{};
    size_t size_{};
};
