// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.h"

#include <cstring>


// During encoding, CharLS process one line at a time. The different implementations
// convert the uncompressed format to and from the internal format for encoding.
// Conversions include color transforms, line interleaved vs sample interleaved, masking out unused bits,
// accounting for line padding etc.

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
    process_encoded_single_component(const std::byte* source, const size_t source_stride,
                                     const size_t bytes_per_pixel) noexcept :
        source_{source}, source_stride_{source_stride}, bytes_per_pixel_{bytes_per_pixel}
    {
        ASSERT(bytes_per_pixel == sizeof(std::byte) || bytes_per_pixel == sizeof(uint16_t));
    }

    void new_line_requested(void* destination, const size_t pixel_count,
                            size_t /* destination_stride */) noexcept(false) override
    {
        memcpy(destination, source_, pixel_count * bytes_per_pixel_);
        source_ += source_stride_;
    }

private:
    const std::byte* source_;
    size_t source_stride_;
    size_t bytes_per_pixel_;
};


class process_encoded_single_component_masked final : public process_encoded_line
{
public:
    process_encoded_single_component_masked(const void* source, const size_t source_stride, const size_t bytes_per_pixel,
                                            const uint32_t bits_per_pixel) noexcept :
        source_{source},
        source_stride_{source_stride},
        bytes_per_pixel_{bytes_per_pixel},
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

        source_ = static_cast<const std::byte*>(source_) + source_stride_;
    }

private:
    const void* source_;
    size_t source_stride_;
    size_t bytes_per_pixel_;
    uint32_t mask_;
    bool single_byte_pixel_;
};


template<typename TransformType, typename SampleType>
void transform_line(triplet<SampleType>* destination, const triplet<SampleType>* source, const size_t pixel_count,
                    const TransformType& transform, const uint32_t mask) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = transform(source[i].v1 & mask, source[i].v2 & mask, source[i].v3 & mask);
    }
}


template<typename SampleType>
void transform_line(quad<SampleType>* destination, const quad<SampleType>* source, const size_t pixel_count,
                    const uint32_t mask) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = {static_cast<SampleType>(source[i].v1 & mask), static_cast<SampleType>(source[i].v2 & mask),
                          static_cast<SampleType>(source[i].v3 & mask), static_cast<SampleType>(source[i].v4 & mask)};
    }
}


template<typename TransformType, typename SampleType>
void transform_triplet_to_line(const triplet<SampleType>* source, const size_t pixel_stride_in, SampleType* destination,
                               const size_t pixel_stride, const TransformType& transform, const uint32_t mask) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const triplet<SampleType> color{source[i]};
        const triplet<SampleType> color_transformed{transform(color.v1 & mask, color.v2 & mask, color.v3 & mask)};

        destination[i] = color_transformed.v1;
        destination[i + pixel_stride] = color_transformed.v2;
        destination[i + 2 * pixel_stride] = color_transformed.v3;
    }
}


template<typename SampleType>
void transform_quad_to_line(const quad<SampleType>* source, const size_t pixel_stride_in, SampleType* destination,
                            const size_t pixel_stride, const uint32_t mask) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const quad<SampleType>& color{source[i]};

        destination[i] = static_cast<SampleType>(color.v1 & mask);
        destination[i + pixel_stride] = static_cast<SampleType>(color.v2 & mask);
        destination[i + 2 * pixel_stride] = static_cast<SampleType>(color.v3 & mask);
        destination[i + 3 * pixel_stride] = static_cast<SampleType>(color.v4 & mask);
    }
}


template<typename TransformType>
class process_encoded_transformed final : public process_encoded_line
{
public:
    process_encoded_transformed(const std::byte* const source, const size_t stride, const frame_info& frame,
                                const interleave_mode interleave_mode) noexcept :
        source_{source},
        stride_{stride},
        mask_{(1U << frame.bits_per_sample) - 1U},
        component_count_{frame.component_count},
        interleave_mode_{interleave_mode}
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
        if (component_count_ == 3)
        {
            if (interleave_mode_ == interleave_mode::sample)
            {
                transform_line(static_cast<triplet<sample_type>*>(destination),
                               static_cast<const triplet<sample_type>*>(source), pixel_count, transform_, mask_);
            }
            else
            {
                transform_triplet_to_line(static_cast<const triplet<sample_type>*>(source), pixel_count,
                                          static_cast<sample_type*>(destination), destination_stride, transform_, mask_);
            }
        }
        else if (component_count_ == 4)
        {
            if (interleave_mode_ == interleave_mode::sample)
            {
                transform_line(static_cast<quad<sample_type>*>(destination), static_cast<const quad<sample_type>*>(source),
                               pixel_count, mask_);
            }
            else if (interleave_mode_ == interleave_mode::line)
            {
                transform_quad_to_line(static_cast<const quad<sample_type>*>(source), pixel_count,
                                       static_cast<sample_type*>(destination), destination_stride, mask_);
            }
        }
    }

private:
    using sample_type = typename TransformType::sample_type;

    const std::byte* source_;
    size_t stride_;
    uint32_t mask_;
    int32_t component_count_;
    interleave_mode interleave_mode_;
    TransformType transform_{};
};

} // namespace charls
