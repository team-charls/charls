// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "util.h"

#include <cstring>

// During decoding, CharLS process one line at a time.
// Conversions include color transforms, line interleaved vs sample interleaved, masking out unused bits,
// accounting for line padding etc.

namespace charls {

struct process_decoded_line
{
    virtual ~process_decoded_line() = default;

    virtual void new_line_decoded(const void* source, size_t pixel_count, size_t source_stride) = 0;

protected:
    process_decoded_line() = default;
    process_decoded_line(const process_decoded_line&) = default;
    process_decoded_line(process_decoded_line&&) = default;
    process_decoded_line& operator=(const process_decoded_line&) = default;
    process_decoded_line& operator=(process_decoded_line&&) = default;
};


class process_decoded_single_component final : public process_decoded_line
{
public:
    process_decoded_single_component(std::byte* destination, const size_t destination_stride,
                                     const size_t bytes_per_pixel) noexcept :
        destination_{destination}, destination_stride_{destination_stride}, bytes_per_pixel_{bytes_per_pixel}
    {
        ASSERT(bytes_per_pixel == sizeof(std::byte) || bytes_per_pixel == sizeof(uint16_t));
    }

    void new_line_decoded(const void* source, const size_t pixel_count, size_t /* source_stride */) noexcept override
    {
        memcpy(destination_, source, pixel_count * bytes_per_pixel_);
        destination_ += destination_stride_;
    }

private:
    std::byte* destination_;
    size_t destination_stride_;
    size_t bytes_per_pixel_;
};


template<typename TransformType, typename SampleType>
void transform_line(triplet<SampleType>* destination, const triplet<SampleType>* source, const size_t pixel_count,
                    const TransformType& transform) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = transform(source[i].v1, source[i].v2, source[i].v3);
    }
}


template<typename SampleType>
void transform_line(quad<SampleType>* destination, const quad<SampleType>* source, const size_t pixel_count) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = source[i];
    }
}


template<typename SampleType>
void transform_line_to_quad(const SampleType* source, const size_t pixel_stride_in, quad<SampleType>* destination,
                            const size_t pixel_stride) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};

    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = {source[i], source[i + pixel_stride_in], source[i + 2 * pixel_stride_in],
                          source[i + 3 * pixel_stride_in]};
    }
}


template<typename TransformType, typename SampleType>
void transform_line_to_triplet(const SampleType* source, const size_t pixel_stride_in, triplet<SampleType>* destination,
                               const size_t pixel_stride, const TransformType& transform) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};

    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = transform(source[i], source[i + pixel_stride_in], source[i + 2 * pixel_stride_in]);
    }
}


template<typename TransformType>
class process_decoded_transformed final : public process_decoded_line
{
public:
    process_decoded_transformed(std::byte* destination, const size_t destination_stride, const int32_t component_count,
                                const interleave_mode interleave_mode) noexcept :
        destination_{destination},
        destination_stride_{destination_stride},
        component_count_{component_count},
        interleave_mode_{interleave_mode}
    {
    }

    void new_line_decoded(const void* source, const size_t pixel_count, const size_t source_stride) noexcept override
    {
        decode_transform(source, destination_, pixel_count, source_stride);
        destination_ += destination_stride_;
    }

    void decode_transform(const void* source, void* destination, const size_t pixel_count,
                          const size_t source_stride) noexcept
    {
        if (component_count_ == 3)
        {
            if (interleave_mode_ == interleave_mode::sample)
            {
                transform_line(static_cast<triplet<sample_type>*>(destination),
                               static_cast<const triplet<sample_type>*>(source), pixel_count, inverse_transform_);
            }
            else
            {
                transform_line_to_triplet(static_cast<const sample_type*>(source), source_stride,
                                          static_cast<triplet<sample_type>*>(destination), pixel_count, inverse_transform_);
            }
        }
        else if (component_count_ == 4)
        {
            if (interleave_mode_ == interleave_mode::sample)
            {
                transform_line(static_cast<quad<sample_type>*>(destination), static_cast<const quad<sample_type>*>(source),
                               pixel_count);
            }
            else if (interleave_mode_ == interleave_mode::line)
            {
                transform_line_to_quad(static_cast<const sample_type*>(source), source_stride,
                                       static_cast<quad<sample_type>*>(destination), pixel_count);
            }
        }
    }

private:
    using sample_type = typename TransformType::sample_type;

    std::byte* destination_;
    size_t destination_stride_;
    int32_t component_count_;
    interleave_mode interleave_mode_;
    typename TransformType::inverse inverse_transform_{};
};

} // namespace charls
