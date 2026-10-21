// SPDX-FileCopyrightText: © 2022 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <type_traits>

// Some cross-platform builds require an explicit static_cast, while others don't.
// These templates can be used to keep code compatible with the GCC useless-cast warning and ReSharper.

namespace charls {

template<typename T, typename U>
[[nodiscard]]
constexpr T conditional_static_cast(const U value) noexcept
{
    if constexpr (std::is_same_v<T, U>)
    {
        return value;
    }
    else
    {
        return static_cast<T>(value);
    }
}

} // namespace charls
