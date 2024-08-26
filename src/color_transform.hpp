// Copyright (c) Team CharLS.
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


template<typename SampleType>
struct transform_hp1 final
{
    static_assert(std::is_integral_v<SampleType>, "Integral required.");

    using sample_type = SampleType;

    FORCE_INLINE triplet<SampleType> operator()(const int red, const int green, const int blue) const noexcept
    {
        return {static_cast<SampleType>(red - green + range_ / 2), static_cast<SampleType>(green), static_cast<SampleType>(blue - green + range_ / 2)};
    }

    struct inverse final
    {
        FORCE_INLINE triplet<SampleType> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            return {static_cast<SampleType>(v1 + v2 - range_ / 2), static_cast<SampleType>(v2), static_cast<SampleType>(v3 + v2 - range_ / 2)};
        }
    };

private:
    static constexpr size_t range_{1 << (sizeof(SampleType) * 8)};
};


template<typename SampleType>
struct transform_hp2 final
{
    static_assert(std::is_integral_v<SampleType>, "Integral required.");

    using sample_type = SampleType;

    FORCE_INLINE triplet<SampleType> operator()(const int red, const int green, const int blue) const noexcept
    {
        return {static_cast<SampleType>(red - green + range_ / 2), static_cast<SampleType>(green),
                static_cast<SampleType>(blue - ((red + green) >> 1) - range_ / 2)};
    }

    struct inverse final
    {
        FORCE_INLINE triplet<SampleType> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            const auto r{static_cast<SampleType>(v1 + v2 - range_ / 2)};
            return {r, static_cast<SampleType>(v2), static_cast<SampleType>(v3 + ((r + static_cast<SampleType>(v2)) >> 1) - range_ / 2)};
        }
    };

private:
    static constexpr size_t range_{1 << (sizeof(SampleType) * 8)};
};


template<typename SampleType>
struct transform_hp3 final
{
    static_assert(std::is_integral_v<SampleType>, "Integral required.");

    using sample_type = SampleType;

    FORCE_INLINE triplet<SampleType> operator()(const int red, const int green, const int blue) const noexcept
    {
        const auto v2{static_cast<SampleType>(blue - green + range_ / 2)};
        const auto v3{static_cast<SampleType>(red - green + range_ / 2)};

        return {static_cast<SampleType>(green + ((v2 + v3) >> 2) - range_ / 4), static_cast<SampleType>(blue - green + range_ / 2),
                static_cast<SampleType>(red - green + range_ / 2)};
    }

    struct inverse final
    {
        FORCE_INLINE triplet<SampleType> operator()(const int v1, const int v2, const int v3) const noexcept
        {
            const auto g{static_cast<int>(v1 - ((v3 + v2) >> 2) + range_ / 4)};
            return {static_cast<SampleType>(v3 + g - range_ / 2), static_cast<SampleType>(g), static_cast<SampleType>(v2 + g - range_ / 2)};
        }
    };

private:
    static constexpr size_t range_{1 << (sizeof(SampleType) * 8)};
};

} // namespace charls
