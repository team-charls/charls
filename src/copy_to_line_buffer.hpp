// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "color_transform.hpp"
#include "scan_codec.hpp"

#include <cstring>


// During encoding, CharLS processes one line at a time. The different implementations
// convert the uncompressed format to and from the internal format for encoding.
// Conversions include color transforms, line interleaved vs sample interleaved, masking out unused bits,
// accounting for line padding etc.

namespace charls {

using copy_to_line_buffer_fn = void (*)(const void* source, void* destination, size_t pixel_count, uint32_t mask) noexcept;

template<typename SampleType>
class copy_to_line_buffer final
{
public:
    [[nodiscard]]
    static copy_to_line_buffer_fn get_copy_function(const interleave_mode interleave_mode, const int32_t component_count,
                                                    const int32_t bits_per_sample,
                                                    const color_transformation color_transformation) noexcept
    {
        switch (interleave_mode)
        {
        case interleave_mode::none: {
            const bool mask_needed{bits_per_sample != sizeof(sample_type) * 8};
            return mask_needed ? &copy_samples_masked : &copy_samples;
        }

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

    static void copy_samples(const void* source, void* destination, const size_t pixel_count, uint32_t /*mask*/) noexcept
    {
        memcpy(destination, source, pixel_count * sizeof(sample_type));
    }

    static void copy_samples_masked(const void* source, void* destination, const size_t pixel_count, uint32_t mask) noexcept
    {
        auto* s{static_cast<const sample_type*>(source)};
        auto* d{static_cast<sample_type*>(destination)};
        const auto m{static_cast<sample_type>(mask)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            d[i] = static_cast<sample_type>(s[i] & m);
        }
    }

    static void copy_line_2_components(const void* source, void* destination, const size_t pixel_count,
                                       uint32_t mask) noexcept
    {
        auto* s{static_cast<const pair<sample_type>*>(source)};
        auto* d{static_cast<sample_type*>(destination)};
        const size_t pixel_stride{pixel_count_to_pixel_stride(pixel_count)};
        const auto m{static_cast<sample_type>(mask)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            const auto pixel{s[i]};

            d[i] = static_cast<sample_type>(pixel.v1 & m);
            d[i + pixel_stride] = static_cast<sample_type>(pixel.v2 & m);
        }
    }

    static void copy_line_3_components(const void* source, void* destination, const size_t pixel_count,
                                       uint32_t mask) noexcept
    {
        auto* s{static_cast<const triplet<sample_type>*>(source)};
        auto* d{static_cast<sample_type*>(destination)};
        const size_t pixel_stride{pixel_count_to_pixel_stride(pixel_count)};
        const auto m{static_cast<sample_type>(mask)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            const auto pixel{s[i]};

            d[i] = static_cast<sample_type>(pixel.v1 & m);
            d[i + pixel_stride] = static_cast<sample_type>(pixel.v2 & m);
            d[i + (2 * pixel_stride)] = static_cast<sample_type>(pixel.v3 & m);
        }
    }

    template<typename Transform>
    static void copy_line_3_components_transform(const void* source, void* destination, const size_t pixel_count,
                                                 uint32_t /*mask*/) noexcept
    {
        copy_line_3_components_transform_impl(source, destination, pixel_count, Transform{});
    }

    template<typename Transform>
    static void copy_line_3_components_transform_impl(const void* source, void* destination, const size_t pixel_count,
                                                      Transform transform) noexcept
    {
        auto* s{static_cast<const triplet<sample_type>*>(source)};
        auto* d{static_cast<sample_type*>(destination)};
        const size_t pixel_stride{pixel_count_to_pixel_stride(pixel_count)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            const auto pixel{s[i]};
            const auto transformed{transform(pixel.v1, pixel.v2, pixel.v3)};

            d[i] = transformed.v1;
            d[i + pixel_stride] = transformed.v2;
            d[i + (2 * pixel_stride)] = transformed.v3;
        }
    }

    static void copy_line_4_components(const void* source, void* destination, const size_t pixel_count,
                                       uint32_t mask) noexcept
    {
        auto* s{static_cast<const quad<sample_type>*>(source)};
        auto* d{static_cast<sample_type*>(destination)};
        const auto m{static_cast<sample_type>(mask)};
        const size_t pixel_stride{pixel_count_to_pixel_stride(pixel_count)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            const auto pixel{s[i]};

            d[i] = static_cast<sample_type>(pixel.v1 & m);
            d[i + pixel_stride] = static_cast<sample_type>(pixel.v2 & m);
            d[i + (2 * pixel_stride)] = static_cast<sample_type>(pixel.v3 & m);
            d[i + (3 * pixel_stride)] = static_cast<sample_type>(pixel.v4 & m);
        }
    }

    static void copy_pixels_2_components(const void* source, void* destination, const size_t pixel_count,
                                         uint32_t mask) noexcept
    {
        auto* s{static_cast<const pair<sample_type>*>(source)};
        auto* d{static_cast<pair<sample_type>*>(destination)};
        const auto m{static_cast<sample_type>(mask)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            const auto pixel{s[i]};
            d[i] = {static_cast<sample_type>(pixel.v1 & m), static_cast<sample_type>(pixel.v2 & m)};
        }
    }

    static void copy_pixels_3_components(const void* source, void* destination, const size_t pixel_count,
                                         uint32_t mask) noexcept
    {
        auto* s{static_cast<const triplet<sample_type>*>(source)};
        auto* d{static_cast<triplet<sample_type>*>(destination)};
        const auto m{static_cast<sample_type>(mask)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            const auto pixel{s[i]};
            d[i] = {static_cast<sample_type>(pixel.v1 & m), static_cast<sample_type>(pixel.v2 & m),
                    static_cast<sample_type>(pixel.v3 & m)};
        }
    }

    template<typename Transform>
    static void copy_pixels_3_components_transform(const void* source, void* destination, const size_t pixel_count,
                                                   uint32_t /*mask*/) noexcept
    {
        copy_pixels_3_components_transform_impl(source, destination, pixel_count, Transform{});
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

    static void copy_pixels_4_components(const void* source, void* destination, const size_t pixel_count,
                                         uint32_t mask) noexcept
    {
        auto* s{static_cast<const quad<sample_type>*>(source)};
        auto* d{static_cast<quad<sample_type>*>(destination)};
        const auto m{static_cast<sample_type>(mask)};

        for (size_t i{}; i != pixel_count; ++i)
        {
            const auto pixel{s[i]};
            d[i] = {static_cast<sample_type>(pixel.v1 & m), static_cast<sample_type>(pixel.v2 & m),
                    static_cast<sample_type>(pixel.v3 & m), static_cast<sample_type>(pixel.v4 & m)};
        }
    }
};

} // namespace charls
