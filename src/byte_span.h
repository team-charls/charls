// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstdint>

/// <summary>Simplified span class for C++14.</summary>
struct byte_span final
{
    byte_span() = default;

    byte_span(void* data, const size_t size) noexcept :
        rawData{static_cast<uint8_t*>(data)},
        count{size}
    {
    }

    byte_span(const void* data, const size_t size) noexcept :
        rawData{static_cast<uint8_t*>(const_cast<void*>(data))},
        count{size}
    {
    }

    uint8_t* rawData{};
    size_t count{};
};
