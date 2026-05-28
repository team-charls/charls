// SPDX-FileCopyrightText: © 2009 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.hpp"

namespace charls {

inline bool color_transformation_possible(const frame_info& frame) noexcept
{
    return frame.component_count == 3 && (frame.bits_per_sample == 8 || frame.bits_per_sample == 16);
}

// This file defines simple classes that define (lossless) color transforms.
// They are used to convert between decoded values and the internal line buffers.
// Color transforms work best for computer generated images, but are outside the official JPEG-LS specifications.

// COLORXFORM_HP1 :
// G = G
// R = R - G
// B = B - G
template<typename SampleType>
struct transform_hp1 final
{
    static_assert(std::is_integral_v<SampleType>, "Integral required.");

    using sample_type = SampleType;

    FORCE_INLINE triplet<SampleType> operator()(const int red, const int green, const int blue) const noexcept
    {
        return {static_cast<SampleType>(red - green + bias), static_cast<SampleType>(green),
                static_cast<SampleType>(blue - green + bias)};
    }

    struct inverse final
    {
        FORCE_INLINE triplet<SampleType> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            return {static_cast<SampleType>(std::clamp(v1 + v2 - bias, 0, range - 1)), static_cast<SampleType>(v2),
                    static_cast<SampleType>(std::clamp(v3 + v2 - bias, 0, range - 1))};
        }
    };

private:
    static constexpr int range{1 << (sizeof(SampleType) * 8)};
    static constexpr int bias{range / 2};
};

// COLORXFORM_HP2 :
// G = G
// B = B - (R + G) / 2
// R = R - G
template<typename SampleType>
struct transform_hp2 final
{
    static_assert(std::is_integral_v<SampleType>, "Integral required.");

    using sample_type = SampleType;

    FORCE_INLINE triplet<SampleType> operator()(const int red, const int green, const int blue) const noexcept
    {
        return {static_cast<SampleType>(red - green + bias), static_cast<SampleType>(green),
                static_cast<SampleType>(blue - ((red + green) / 2) - bias)}; // TODO: fix -> - to +?
    }

    struct inverse final
    {
        FORCE_INLINE triplet<SampleType> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            const auto r{v1 + v2 - bias};
            return {static_cast<SampleType>(std::clamp(r, 0, range - 1)), static_cast<SampleType>(v2),
                    static_cast<SampleType>(std::clamp(v3 + ((r + v2) >> 1) - bias, 0, range - 1))};
        }
    };

private:
    static constexpr int range{1 << (sizeof(SampleType) * 8)};
    static constexpr int bias{range / 2};
};


template<typename SampleType>
struct transform_hp3 final
{
    static_assert(std::is_integral_v<SampleType>, "Integral required.");

    using sample_type = SampleType;

    FORCE_INLINE triplet<SampleType> operator()(const int red, const int green, const int blue) const noexcept
    {
        const auto v2{static_cast<SampleType>(blue - green + range / 2)};
        const auto v3{static_cast<SampleType>(red - green + range / 2)};

        return {static_cast<SampleType>(green + ((v2 + v3) >> 2) - range / 4),
                static_cast<SampleType>(blue - green + range / 2), static_cast<SampleType>(red - green + range / 2)};
    }

    struct inverse final
    {
        FORCE_INLINE triplet<SampleType> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            const auto g{static_cast<int>(v1 - ((v3 + v2) >> 2) + range / 4)};
            return {static_cast<SampleType>(v3 + g - range / 2), static_cast<SampleType>(g),
                    static_cast<SampleType>(v2 + g - range / 2)};
        }
    };

private:
    static constexpr size_t range{1 << (sizeof(SampleType) * 8)};
};

} // namespace charls
