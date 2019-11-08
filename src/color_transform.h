// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.h"

namespace charls {

// This file defines simple classes that define (lossless) color transforms.
// They are invoked in process_line.h to convert between decoded values and the internal line buffers.
// Color transforms work best for computer generated images, but are outside the official JPEG-LS specifications.

template<typename T>
struct TransformNoneImpl
{
    static_assert(std::is_integral<T>::value, "Integral required.");

    using size_type = T;

    FORCE_INLINE Triplet<T> operator()(int v1, int v2, int v3) const noexcept
    {
        return Triplet<T>(v1, v2, v3);
    }
};


template<typename T>
struct TransformNone final : TransformNoneImpl<T>
{
    static_assert(std::is_integral<T>::value, "Integral required.");

    using Inverse = TransformNoneImpl<T>;
};


template<typename T>
struct TransformHp1 final
{
    static_assert(std::is_integral<T>::value, "Integral required.");

    using size_type = T;

    struct Inverse final
    {
        explicit Inverse(const TransformHp1&) noexcept
        {
        }

        FORCE_INLINE Triplet<T> operator()(int v1, int v2, int v3) const noexcept
        {
            return Triplet<T>(v1 + v2 - Range / 2, v2, v3 + v2 - Range / 2);
        }
    };

    FORCE_INLINE Triplet<T> operator()(int red, int green, int blue) const noexcept
    {
        Triplet<T> hp1;
        hp1.v2 = static_cast<T>(green);
        hp1.v1 = static_cast<T>(red - green + Range / 2);
        hp1.v3 = static_cast<T>(blue - green + Range / 2);
        return hp1;
    }

private:
    static constexpr size_t Range = 1 << (sizeof(T) * 8);
};


template<typename T>
struct TransformHp2 final
{
    static_assert(std::is_integral<T>::value, "Integral required.");

    using size_type = T;

    struct Inverse final
    {
        explicit Inverse(const TransformHp2&) noexcept
        {
        }

        FORCE_INLINE Triplet<T> operator()(int v1, int v2, int v3) const noexcept
        {
            Triplet<T> rgb;
            rgb.R = static_cast<T>(v1 + v2 - Range / 2);                     // new R
            rgb.G = static_cast<T>(v2);                                      // new G
            rgb.B = static_cast<T>(v3 + ((rgb.R + rgb.G) >> 1) - Range / 2); // new B
            return rgb;
        }
    };

    FORCE_INLINE Triplet<T> operator()(int red, int green, int blue) const noexcept
    {
        return Triplet<T>(red - green + Range / 2, green, blue - ((red + green) >> 1) - Range / 2);
    }

private:
    static constexpr size_t Range = 1 << (sizeof(T) * 8);
};


template<typename T>
struct TransformHp3 final
{
    static_assert(std::is_integral<T>::value, "Integral required.");

    using size_type = T;

    struct Inverse final
    {
        explicit Inverse(const TransformHp3&) noexcept
        {
        }

        FORCE_INLINE Triplet<T> operator()(int v1, int v2, int v3) const noexcept
        {
            const int G = v1 - ((v3 + v2) >> 2) + Range / 4;
            Triplet<T> rgb;
            rgb.R = static_cast<T>(v3 + G - Range / 2); // new R
            rgb.G = static_cast<T>(G);                  // new G
            rgb.B = static_cast<T>(v2 + G - Range / 2); // new B
            return rgb;
        }
    };

    FORCE_INLINE Triplet<T> operator()(int red, int green, int blue) const noexcept
    {
        Triplet<T> hp3;
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
struct TransformShifted final
{
    using size_type = typename Transform::size_type;

    struct Inverse final
    {
        explicit Inverse(const TransformShifted& transform) noexcept :
            shift_{transform.shift_},
            inverseTransform_{transform.colorTransform_}
        {
        }

        FORCE_INLINE Triplet<size_type> operator()(int v1, int v2, int v3) noexcept
        {
            const Triplet<size_type> result = inverseTransform_(v1 << shift_, v2 << shift_, v3 << shift_);
            return Triplet<size_type>(result.R >> shift_, result.G >> shift_, result.B >> shift_);
        }

        FORCE_INLINE Quad<size_type> operator()(int v1, int v2, int v3, int v4)
        {
            Triplet<size_type> result = inverseTransform_(v1 << shift_, v2 << shift_, v3 << shift_);
            return Quad<size_type>(result.R >> shift_, result.G >> shift_, result.B >> shift_, v4);
        }

    private:
        int shift_;
        typename Transform::Inverse inverseTransform_;
    };

    explicit TransformShifted(int shift) noexcept :
        shift_{shift}
    {
    }

    FORCE_INLINE Triplet<size_type> operator()(int red, int green, int blue) noexcept
    {
        const Triplet<size_type> result = colorTransform_(red << shift_, green << shift_, blue << shift_);
        return Triplet<size_type>(result.R >> shift_, result.G >> shift_, result.B >> shift_);
    }

    FORCE_INLINE Quad<size_type> operator()(int red, int green, int blue, int alpha)
    {
        Triplet<size_type> result = colorTransform_(red << shift_, green << shift_, blue << shift_);
        return Quad<size_type>(result.R >> shift_, result.G >> shift_, result.B >> shift_, alpha);
    }

private:
    int shift_;
    Transform colorTransform_;
};

} // namespace charls
