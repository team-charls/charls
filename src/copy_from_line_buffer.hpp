// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "color_transform.hpp"
#include "scan_codec.hpp"

#include <cstring>

// During decoding, CharLS process one line at a time.
// Conversions include color transforms, line interleaved vs sample interleaved, masking out unused bits,
// accounting for line padding etc.

namespace charls {

using copy_from_line_buffer_fn = void (*)(const void* source, void* destination, size_t pixel_count) noexcept;

template<typename SampleType>
class copy_from_line_buffer final
{
public:
    [[nodiscard]]
    static copy_from_line_buffer_fn get_copy_function(const interleave_mode interleave_mode, const int32_t component_count,
                                                      const color_transformation color_transformation) noexcept
    {
        switch (interleave_mode)
        {
        case interleave_mode::none:
            return &copy_samples;

        case interleave_mode::line:
            switch (component_count)
            {
            case 2:
                return &copy_line_2_components;

            case 3:
                switch (color_transformation)
                {
                case color_transformation::none:
                    return &copy_line_3_components;

                case color_transformation::hp1:
                    return &copy_line_3_components_transform<transform_hp1<sample_type>>;

                case color_transformation::hp2:
                    return &copy_line_3_components_transform<transform_hp2<sample_type>>;

                case color_transformation::hp3:
                    return &copy_line_3_components_transform<transform_hp3<sample_type>>;
                }
                break;

            default:
                ASSERT(component_count == 4);
                return &copy_line_4_components;
            }
            break;

        case interleave_mode::sample:
            switch (component_count)
            {
            case 2:
                return &copy_pixels_2_components;

            case 3:
                switch (color_transformation)
                {
                case color_transformation::none:
                    return &copy_pixels_3_components;

                case color_transformation::hp1:
                    return &copy_pixels_3_components_transform<transform_hp1<sample_type>>;

                case color_transformation::hp2:
                    return &copy_pixels_3_components_transform<transform_hp2<sample_type>>;

                case color_transformation::hp3:
                    return &copy_pixels_3_components_transform<transform_hp3<sample_type>>;
                }
                break;

            default:
                ASSERT(component_count == 4);
                return &copy_pixels_4_components;
            }
        }

        unreachable();
    }

private:
    using sample_type = SampleType;

    static void copy_samples(const void* source, void* destination, const size_t pixel_count) noexcept
    {
        memcpy(destination, source, pixel_count * sizeof(sample_type));
    }

    static void copy_line_2_components(const void* source, void* destination, const size_t pixel_count) noexcept
    {
        auto* s{static_cast<const sample_type*>(source)};
        auto* d{static_cast<pair<sample_type>*>(destination)};
        const size_t pixel_stride{pixel_count_to_pixel_stride(pixel_count)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            d[i] = {s[i], s[i + pixel_stride]};
        }
    }

    static void copy_line_3_components(const void* source, void* destination, const size_t pixel_count) noexcept
    {
        auto* s{static_cast<const sample_type*>(source)};
        auto* d{static_cast<triplet<sample_type>*>(destination)};
        const size_t pixel_stride{pixel_count_to_pixel_stride(pixel_count)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            d[i] = {s[i], s[i + pixel_stride], s[i + 2 * pixel_stride]};
        }
    }

    template<typename Transform>
    static void copy_line_3_components_transform(const void* source, void* destination, const size_t pixel_count) noexcept
    {
        copy_line_3_components_transform_impl(source, destination, pixel_count, typename Transform::inverse{});
    }

    template<typename Transform>
    static void copy_line_3_components_transform_impl(const void* source, void* destination, const size_t pixel_count,
                                                      Transform transform) noexcept
    {
        auto* s{static_cast<const sample_type*>(source)};
        auto* d{static_cast<triplet<sample_type>*>(destination)};
        const size_t pixel_stride{pixel_count_to_pixel_stride(pixel_count)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            d[i] = transform(s[i], s[i + pixel_stride], s[i + 2 * pixel_stride]);
        }
    }

    static void copy_line_4_components(const void* source, void* destination, const size_t pixel_count) noexcept
    {
        auto* s{static_cast<const sample_type*>(source)};
        auto* d{static_cast<quad<sample_type>*>(destination)};
        const size_t pixel_stride{pixel_count_to_pixel_stride(pixel_count)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            d[i] = {s[i], s[i + pixel_stride], s[i + 2 * pixel_stride], s[i + 3 * pixel_stride]};
        }
    }

    static void copy_pixels_2_components(const void* source, void* destination, const size_t pixel_count) noexcept
    {
        memcpy(destination, source, pixel_count * sizeof(pair<sample_type>));
    }

    static void copy_pixels_3_components(const void* source, void* destination, const size_t pixel_count) noexcept
    {
        memcpy(destination, source, pixel_count * sizeof(triplet<sample_type>));
    }

    template<typename Transform>
    static void copy_pixels_3_components_transform(const void* source, void* destination, const size_t pixel_count) noexcept
    {
        copy_pixels_3_components_transform_impl(source, destination, pixel_count, typename Transform::inverse{});
    }

    template<typename Transform>
    static void copy_pixels_3_components_transform_impl(const void* source, void* destination, const size_t pixel_count,
                                                        Transform transform) noexcept
    {
        auto* s{static_cast<const triplet<sample_type>*>(source)};
        auto* d{static_cast<triplet<sample_type>*>(destination)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            const auto pixel{s[i]};
            d[i] = transform(pixel.v1, pixel.v2, pixel.v3);
        }
    }

    static void copy_pixels_4_components(const void* source, void* destination, const size_t pixel_count) noexcept
    {
        memcpy(destination, source, pixel_count * sizeof(quad<sample_type>));
    }
};

} // namespace charls
