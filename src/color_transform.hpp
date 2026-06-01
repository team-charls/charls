// SPDX-FileCopyrightText: © 2009 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.hpp"

namespace charls {

[[nodiscard]]
inline bool color_transformation_possible(const frame_info& frame, const int32_t near_lossless,
                                          const interleave_mode mode) noexcept
{
    return frame.component_count == 3 && (frame.bits_per_sample == 8 || frame.bits_per_sample == 16) && near_lossless == 0 &&
           mode != interleave_mode::none;
}

// This file defines simple classes that define (lossless) color transforms.
// They are used to convert between decoded values and the internal line buffers.
// Color transforms work best for computer generated images, but are outside the official JPEG-LS specifications.

// COLORXFORM_HP1:
// G = G
// R = R - G
// B = B - G
template<typename SampleType>
struct transform_hp1 final
{
    static_assert(std::is_integral_v<SampleType>, "Integral required.");

    using sample_type = SampleType;

    FORCE_INLINE triplet<SampleType> operator()(const int32_t red, const int32_t green, const int32_t blue) const noexcept
    {
        return {static_cast<SampleType>(red - green + bias), static_cast<SampleType>(green),
                static_cast<SampleType>(blue - green + bias)};
    }

    struct inverse final
    {
        FORCE_INLINE triplet<SampleType> operator()(const int32_t v1, const int32_t v2, const int32_t v3) const noexcept
        {
            return {static_cast<SampleType>(v1 + v2 - bias), static_cast<SampleType>(v2),
                    static_cast<SampleType>(v3 + v2 - bias)};
        }
    };

private:
    static constexpr int32_t range{1 << (sizeof(SampleType) * 8)};
    static constexpr int32_t bias{range / 2};
};

// COLORXFORM_HP2:
// G = G
// B = B - (R + G) / 2
// R = R - G
template<typename SampleType>
struct transform_hp2 final
{
    static_assert(std::is_integral_v<SampleType>, "Integral required.");

    using sample_type = SampleType;

    FORCE_INLINE triplet<SampleType> operator()(const int32_t red, const int32_t green, const int32_t blue) const noexcept
    {
        return {static_cast<SampleType>(red - green + bias), static_cast<SampleType>(green),
                static_cast<SampleType>(blue - ((red + green) / 2) + bias)};
    }

    struct inverse final
    {
        FORCE_INLINE triplet<SampleType> operator()(const int32_t v1, const int32_t v2, const int32_t v3) const noexcept
        {
            const auto r{static_cast<SampleType>(v1 + v2 - bias)};
            return {r, static_cast<SampleType>(v2),
                    static_cast<SampleType>(v3 + ((r + static_cast<SampleType>(v2)) >> 1) - bias)};
        }
    };

private:
    static constexpr int32_t range{1 << (sizeof(SampleType) * 8)};
    static constexpr int32_t bias{range / 2};
};


// COLORXFORM_HP3: (a lossless version of Y-Cb-Cr)
// R = R - G
// B = B - G
// G = G + (R + B) / 4
template<typename SampleType>
struct transform_hp3 final
{
    static_assert(std::is_integral_v<SampleType>, "Integral required.");

    using sample_type = SampleType;

    FORCE_INLINE triplet<SampleType> operator()(const int32_t red, const int32_t green, const int32_t blue) const noexcept
    {
        const auto v2{static_cast<SampleType>(blue - green + bias)};
        const auto v3{static_cast<SampleType>(red - green + bias)};
        return {static_cast<SampleType>(green + ((v2 + v3) >> 2) - range / 4), v2, v3};
    }

    struct inverse final
    {
        FORCE_INLINE triplet<SampleType> operator()(const int32_t v1, const int32_t v2, const int32_t v3) const noexcept
        {
            const auto g{v1 - ((v3 + v2) >> 2) + range / 4};
            return {static_cast<SampleType>(v3 + g - bias), static_cast<SampleType>(g),
                    static_cast<SampleType>(v2 + g - bias)};
        }
    };

private:
    static constexpr int32_t range{1 << (sizeof(SampleType) * 8)};
    static constexpr int32_t bias{range / 2};
};

} // namespace charls
