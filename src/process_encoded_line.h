// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.h"
#include "util.h"

#include <cstring>
#include <vector>


//
// This file defines the ProcessLine base class, its derivatives and helper functions.
// During coding/decoding, CharLS process one line at a time. The different ProcessLine implementations
// convert the uncompressed format to and from the internal format for encoding.
// Conversions include color transforms, line interleaved vs sample interleaved, masking out unused bits,
// accounting for line padding etc.
// This mechanism could be used to encode/decode images as they are received.
//

namespace charls {

struct process_encoded_line
{
    virtual ~process_encoded_line() = default;

    virtual void new_line_requested(void* destination, size_t pixel_count, size_t destination_stride) = 0;

protected:
    process_encoded_line() = default;
    process_encoded_line(const process_encoded_line&) = default;
    process_encoded_line(process_encoded_line&&) = default;
    process_encoded_line& operator=(const process_encoded_line&) = default;
    process_encoded_line& operator=(process_encoded_line&&) = default;
};


class process_encoded_single_component final : public process_encoded_line
{
public:
    process_encoded_single_component(const std::byte* source, const size_t stride, const size_t bytes_per_pixel) noexcept :
        source_{source}, bytes_per_pixel_{bytes_per_pixel}, stride_{stride}
    {
        ASSERT(bytes_per_pixel == sizeof(std::byte) || bytes_per_pixel == sizeof(uint16_t));
    }

    void new_line_requested(void* destination, const size_t pixel_count,
                            size_t /* destination_stride */) noexcept(false) override
    {
        memcpy(destination, source_, pixel_count * bytes_per_pixel_);
        source_ += stride_;
    }

private:
    const std::byte* source_;
    size_t bytes_per_pixel_;
    size_t stride_;
};


class process_encoded_single_component_masked final : public process_encoded_line
{
public:
    process_encoded_single_component_masked(const void* source, const size_t stride, const size_t bytes_per_pixel,
                                            const uint32_t bits_per_pixel) noexcept :
        source_{source},
        bytes_per_pixel_{bytes_per_pixel},
        stride_{stride},
        mask_{(1U << bits_per_pixel) - 1U},
        single_byte_pixel_{bytes_per_pixel_ == sizeof(std::byte)}
    {
        ASSERT(bytes_per_pixel == sizeof(std::byte) || bytes_per_pixel == sizeof(uint16_t));
    }

    void new_line_requested(void* destination, const size_t pixel_count,
                            const size_t /* destination_stride */) noexcept(false) override
    {
        if (single_byte_pixel_)
        {
            const auto* pixel_source{static_cast<const std::byte*>(source_)};
            auto* pixel_destination{static_cast<std::byte*>(destination)};
            for (size_t i{}; i != pixel_count; ++i)
            {
                pixel_destination[i] = pixel_source[i] & static_cast<std::byte>(mask_);
            }
        }
        else
        {
            const auto* pixel_source{static_cast<const uint16_t*>(source_)};
            auto* pixel_destination{static_cast<uint16_t*>(destination)};
            for (size_t i{}; i != pixel_count; ++i)
            {
                pixel_destination[i] = static_cast<uint16_t>(pixel_source[i] & mask_);
            }
        }

        source_ = static_cast<const std::byte*>(source_) + stride_;
    }

private:
    const void* source_;
    size_t bytes_per_pixel_;
    size_t stride_;
    uint32_t mask_;
    bool single_byte_pixel_;
};


template<typename Transform, typename PixelType>
void transform_line(triplet<PixelType>* destination, const triplet<PixelType>* source, const size_t pixel_count,
                    Transform& transform, const uint32_t mask) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = transform(source[i].v1 & mask, source[i].v2 & mask, source[i].v3 & mask);
    }
}


template<typename Transform, typename PixelType>
void transform_line(quad<PixelType>* destination, const quad<PixelType>* source, const size_t pixel_count,
                    Transform& transform, const uint32_t mask) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = transform(source[i].v1 & mask, source[i].v2 & mask, source[i].v3 & mask, source[i].v4 & mask);
    }
}


template<typename Transform, typename PixelType>
void transform_triplet_to_line(const triplet<PixelType>* source, const size_t pixel_stride_in, PixelType* destination,
                               const size_t pixel_stride, Transform& transform) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};
    const triplet<PixelType>* type_buffer_in{source};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const triplet<PixelType> color{type_buffer_in[i]};
        const triplet<PixelType> color_transformed{transform(color.v1, color.v2, color.v3)};

        destination[i] = color_transformed.v1;
        destination[i + pixel_stride] = color_transformed.v2;
        destination[i + 2 * pixel_stride] = color_transformed.v3;
    }
}


template<typename Transform, typename PixelType>
void transform_triplet_to_line(const triplet<PixelType>* source, const size_t pixel_stride_in, PixelType* destination,
                               const size_t pixel_stride, Transform& transform, const uint32_t mask) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};
    const triplet<PixelType>* type_buffer_in{source};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const triplet<PixelType> color{type_buffer_in[i]};
        const triplet<PixelType> color_transformed{transform(color.v1 & mask, color.v2 & mask, color.v3 & mask)};

        destination[i] = color_transformed.v1;
        destination[i + pixel_stride] = color_transformed.v2;
        destination[i + 2 * pixel_stride] = color_transformed.v3;
    }
}


template<typename TransformType, typename PixelType>
void transform_quad_to_line(const quad<PixelType>* source, const size_t pixel_stride_in, PixelType* destination,
                            const size_t pixel_stride, TransformType& transform) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const quad<PixelType> color{source[i]};
        const quad<PixelType> color_transformed(transform(color.v1, color.v2, color.v3, color.v4));

        destination[i] = color_transformed.v1;
        destination[i + pixel_stride] = color_transformed.v2;
        destination[i + 2 * pixel_stride] = color_transformed.v3;
        destination[i + 3 * pixel_stride] = color_transformed.v4;
    }
}


template<typename TransformType, typename PixelType>
void transform_quad_to_line(const quad<PixelType>* source, const size_t pixel_stride_in, PixelType* destination,
                            const size_t pixel_stride, TransformType& transform, const uint32_t mask) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const quad<PixelType> color{source[i]};
        const quad<PixelType> color_transformed(
            transform(color.v1 & mask, color.v2 & mask, color.v3 & mask, color.v4 & mask));

        destination[i] = color_transformed.v1;
        destination[i + pixel_stride] = color_transformed.v2;
        destination[i + 2 * pixel_stride] = color_transformed.v3;
        destination[i + 3 * pixel_stride] = color_transformed.v4;
    }
}


template<typename TransformType>
class process_encoded_transformed final : public process_encoded_line
{
public:
    process_encoded_transformed(const std::byte* const source, const size_t stride, const frame_info& info,
                                const coding_parameters& parameters, TransformType transform) :
        frame_info_{&info},
        parameters_{&parameters},
        stride_{stride},
        temp_line_(static_cast<size_t>(info.component_count) * info.width),
        buffer_(static_cast<size_t>(info.component_count) * info.width * sizeof(size_type)),
        transform_{transform},
        inverse_transform_{transform},
        source_{source},
        mask_{(1U << info.bits_per_sample) - 1U}
    {
    }

    void new_line_requested(void* destination, const size_t pixel_count,
                            const size_t destination_stride) noexcept(false) override
    {
        encode_transform(source_, destination, pixel_count, destination_stride);
        source_ += stride_;
    }

    void encode_transform(const void* source, void* destination, const size_t pixel_count,
                          const size_t destination_stride) noexcept
    {
        if (frame_info_->component_count == 3)
        {
            if (parameters_->interleave_mode == interleave_mode::sample)
            {
                transform_line(static_cast<triplet<size_type>*>(destination), static_cast<const triplet<size_type>*>(source),
                               pixel_count, transform_, mask_);
            }
            else
            {
                transform_triplet_to_line(static_cast<const triplet<size_type>*>(source), pixel_count,
                                          static_cast<size_type*>(destination), destination_stride, transform_, mask_);
            }
        }
        else if (frame_info_->component_count == 4)
        {
            if (parameters_->interleave_mode == interleave_mode::sample)
            {
                transform_line(static_cast<quad<size_type>*>(destination), static_cast<const quad<size_type>*>(source),
                               pixel_count, transform_, mask_);
            }
            else if (parameters_->interleave_mode == interleave_mode::line)
            {
                transform_quad_to_line(static_cast<const quad<size_type>*>(source), pixel_count,
                                       static_cast<size_type*>(destination), destination_stride, transform_, mask_);
            }
        }
    }

private:
    using size_type = typename TransformType::size_type;

    const frame_info* frame_info_;
    const coding_parameters* parameters_;
    size_t stride_;
    std::vector<size_type> temp_line_;
    std::vector<uint8_t> buffer_;
    TransformType transform_;
    typename TransformType::inverse inverse_transform_;
    const std::byte* source_;
    uint32_t mask_;
};

} // namespace charls
