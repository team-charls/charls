// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

/// <summary>Simplified span class for C++14.</summary>
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
