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
    static_assert(std::is_integral<T>::value, "Integral required.");

    using size_type = T;

    FORCE_INLINE triplet<T> operator()(const int v1, const int v2, const int v3) const noexcept
    {
        return triplet<T>(v1, v2, v3);
    }
};


template<typename T>
struct transform_none final : transform_none_impl<T>
{
    static_assert(std::is_integral<T>::value, "Integral required.");

    using inverse = transform_none_impl<T>;
};


template<typename T>
struct transform_hp1 final
{
    static_assert(std::is_integral<T>::value, "Integral required.");

    using size_type = T;

    struct inverse final
    {
        explicit inverse(const transform_hp1&) noexcept
        {
        }

        FORCE_INLINE triplet<T> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            return triplet<T>(v1 + v2 - Range / 2, v2, v3 + v2 - Range / 2);
        }
    };

    FORCE_INLINE triplet<T> operator()(const int red, const int green, const int blue) const noexcept
    {
        triplet<T> hp1;
        hp1.v2 = static_cast<T>(green);
        hp1.v1 = static_cast<T>(red - green + Range / 2);
        hp1.v3 = static_cast<T>(blue - green + Range / 2);
        return hp1;
    }

private:
    static constexpr size_t Range = 1 << (sizeof(T) * 8);
};


template<typename T>
struct transform_hp2 final
{
    static_assert(std::is_integral<T>::value, "Integral required.");

    using size_type = T;

    struct inverse final
    {
        explicit inverse(const transform_hp2&) noexcept
        {
        }

        FORCE_INLINE triplet<T> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            triplet<T> rgb;
            rgb.R = static_cast<T>(v1 + v2 - Range / 2);                     // new R
            rgb.G = static_cast<T>(v2);                                      // new G
            rgb.B = static_cast<T>(v3 + ((rgb.R + rgb.G) >> 1) - Range / 2); // new B
            return rgb;
        }
    };

    FORCE_INLINE triplet<T> operator()(const int red, const int green, const int blue) const noexcept
    {
        return triplet<T>(red - green + Range / 2, green, blue - ((red + green) >> 1) - Range / 2);
    }

private:
    static constexpr size_t Range = 1 << (sizeof(T) * 8);
};


template<typename T>
struct transform_hp3 final
{
    static_assert(std::is_integral<T>::value, "Integral required.");

    using size_type = T;

    struct inverse final
    {
        explicit inverse(const transform_hp3&) noexcept
        {
        }

        FORCE_INLINE triplet<T> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            const int g = v1 - ((v3 + v2) >> 2) + Range / 4;
            triplet<T> rgb;
            rgb.R = static_cast<T>(v3 + g - Range / 2); // new R
            rgb.G = static_cast<T>(g);                  // new G
            rgb.B = static_cast<T>(v2 + g - Range / 2); // new B
            return rgb;
        }
    };

    FORCE_INLINE triplet<T> operator()(const int red, const int green, const int blue) const noexcept
    {
        triplet<T> hp3;
        hp3.v2 = static_cast<T>(blue - green + Range / 2);
        hp3.v3 = static_cast<T>(red - green + Range / 2);
        hp3.v1 = static_cast<T>(green + ((hp3.v2 + hp3.v3) >> 2)) - Range / 4;
        return hp3;
    }

private:
    static constexpr size_t Range = 1 << (sizeof(T) * 8);
};


// Transform class that shifts bits towards the high bit when bit count is not 8 or 16
// needed to make the HP color transformations work correctly.
template<typename Transform>
struct transform_shifted final
{
    using size_type = typename Transform::size_type;

    struct inverse final
    {
        explicit inverse(const transform_shifted& transform) noexcept :
            shift_{transform.shift_},
            inverse_transform_{transform.color_transform_}
        {
        }

        FORCE_INLINE triplet<size_type> operator()(const int v1, const int v2, const int v3) noexcept
        {
            const triplet<size_type> result = inverse_transform_(v1 << shift_, v2 << shift_, v3 << shift_);
            return triplet<size_type>(result.R >> shift_, result.G >> shift_, result.B >> shift_);
        }

        FORCE_INLINE quad<size_type> operator()(const int v1, const int v2, const int v3, int v4)
        {
            triplet<size_type> result = inverse_transform_(v1 << shift_, v2 << shift_, v3 << shift_);
            return quad<size_type>(result.R >> shift_, result.G >> shift_, result.B >> shift_, v4);
        }

    private:
        int shift_;
        typename Transform::inverse inverse_transform_;
    };

    explicit transform_shifted(const int shift) noexcept :
        shift_{shift}
    {
    }

    FORCE_INLINE triplet<size_type> operator()(const int red, const int green, const int blue) noexcept
    {
        const triplet<size_type> result = color_transform_(red << shift_, green << shift_, blue << shift_);
        return triplet<size_type>(result.R >> shift_, result.G >> shift_, result.B >> shift_);
    }

    FORCE_INLINE quad<size_type> operator()(const int red, const int green, const int blue, int alpha)
    {
        triplet<size_type> result = color_transform_(red << shift_, green << shift_, blue << shift_);
        return quad<size_type>(result.R >> shift_, result.G >> shift_, result.B >> shift_, alpha);
    }

private:
    int shift_;
    Transform color_transform_;
};

} // namespace charls
