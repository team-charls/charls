// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.h"

namespace charls {

// This file defines simple classes that define (lossless) color transforms.
// They are invoked in process_line.h to convert between decoded values and the internal line buffers.
// Color transforms work best for computer generated images, but are outside the official JPEG-LS specifications.

template<typename T>
struct transform_none_impl
{
    static_assert(std::is_integral_v<T>, "Integral required.");

    using size_type = T;

    FORCE_INLINE triplet<T> operator()(const int v1, const int v2, const int v3) const noexcept
    {
        return {v1, v2, v3};
    }

    FORCE_INLINE quad<T> operator()(const int v1, const int v2, const int v3, const int v4) const noexcept
    {
        return {v1, v2, v3, v4};
    }
};


template<typename T>
struct transform_none final : transform_none_impl<T>
{
    static_assert(std::is_integral_v<T>, "Integral required.");

    using inverse = transform_none_impl<T>;
};


template<typename T>
struct transform_hp1 final
{
    static_assert(std::is_integral_v<T>, "Integral required.");

    using size_type = T;

    struct inverse final
    {
        explicit inverse(const transform_hp1& /*template_selector*/) noexcept
        {
        }

        FORCE_INLINE triplet<T> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            return {static_cast<T>(v1 + v2 - range_ / 2), v2, static_cast<T>(v3 + v2 - range_ / 2)};
        }

        quad<T> operator()(int, int, int, int) const noexcept
        {
            ASSERT(false);
            return {};
        }
    };

    FORCE_INLINE triplet<T> operator()(const int red, const int green, const int blue) const noexcept
    {
        return {static_cast<T>(red - green + range_ / 2), static_cast<T>(green), static_cast<T>(blue - green + range_ / 2)};
    }

    quad<T> operator()(int, int, int, int) const noexcept
    {
        ASSERT(false);
        return {};
    }

private:
    static constexpr size_t range_{1 << (sizeof(T) * 8)};
};


template<typename T>
struct transform_hp2 final
{
    static_assert(std::is_integral_v<T>, "Integral required.");

    using size_type = T;

    struct inverse final
    {
        explicit inverse(const transform_hp2& /*template_selector*/) noexcept
        {
        }

        FORCE_INLINE triplet<T> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            const auto r{static_cast<T>(v1 + v2 - range_ / 2)};
            return {r, static_cast<T>(v2), static_cast<T>(v3 + ((r + static_cast<T>(v2)) >> 1) - range_ / 2)};
        }

        quad<T> operator()(int, int, int, int) const noexcept
        {
            ASSERT(false);
            return {};
        }
    };

    FORCE_INLINE triplet<T> operator()(const int red, const int green, const int blue) const noexcept
    {
        return {static_cast<T>(red - green + range_ / 2), green, static_cast<T>(blue - ((red + green) >> 1) - range_ / 2)};
    }

    quad<T> operator()(int, int, int, int) const noexcept
    {
        ASSERT(false);
        return {};
    }

private:
    static constexpr size_t range_{1 << (sizeof(T) * 8)};
};


template<typename T>
struct transform_hp3 final
{
    static_assert(std::is_integral_v<T>, "Integral required.");

    using size_type = T;

    struct inverse final
    {
        explicit inverse(const transform_hp3& /*template_selector*/) noexcept
        {
        }

        FORCE_INLINE triplet<T> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            const auto g{static_cast<int>(v1 - ((v3 + v2) >> 2) + range_ / 4)};
            return {static_cast<T>(v3 + g - range_ / 2), static_cast<T>(g), static_cast<T>(v2 + g - range_ / 2)};
        }

        quad<T> operator()(int, int, int, int) const noexcept
        {
            ASSERT(false);
            return {};
        }
    };

    FORCE_INLINE triplet<T> operator()(const int red, const int green, const int blue) const noexcept
    {
        const auto v2{static_cast<T>(blue - green + range_ / 2)};
        const auto v3{static_cast<T>(red - green + range_ / 2)};

        return {static_cast<T>(green + ((v2 + v3) >> 2) - range_ / 4), static_cast<T>(blue - green + range_ / 2),
                static_cast<T>(red - green + range_ / 2)};
    }

    quad<T> operator()(int, int, int, int) const noexcept
    {
        ASSERT(false);
        return {};
    }

private:
    static constexpr size_t range_{1 << (sizeof(T) * 8)};
};

} // namespace charls
