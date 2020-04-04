// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/charls_legacy.h>
#include <charls/jpegls_error.h>

#include "coding_parameters.h"
#include "util.h"

#include <algorithm>
#include <cstring>
#include <sstream>
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

class ProcessLine
{
public:
    virtual ~ProcessLine() = default;

    ProcessLine(const ProcessLine&) = delete;
    ProcessLine(ProcessLine&&) = delete;
    ProcessLine& operator=(const ProcessLine&) = delete;
    ProcessLine& operator=(ProcessLine&&) = delete;

    virtual void NewLineDecoded(const void* pSrc, int pixelCount, int sourceStride) = 0;
    virtual void NewLineRequested(void* pDest, int pixelCount, int destStride) = 0;

protected:
    ProcessLine() = default;
};


class PostProcessSingleComponent final : public ProcessLine
{
public:
    PostProcessSingleComponent(void* rawData, const uint32_t stride, const size_t bytesPerPixel) noexcept :
        rawData_{static_cast<uint8_t*>(rawData)},
        bytesPerPixel_{bytesPerPixel},
        bytesPerLine_{stride}
    {
    }

    void NewLineRequested(void* destination, const int pixelCount, int /*byteStride*/) noexcept(false) override
    {
        std::memcpy(destination, rawData_, pixelCount * bytesPerPixel_);
        rawData_ += bytesPerLine_;
    }

    void NewLineDecoded(const void* source, const int pixelCount, int /*sourceStride*/) noexcept(false) override
    {
        std::memcpy(rawData_, source, pixelCount * bytesPerPixel_);
        rawData_ += bytesPerLine_;
    }

private:
    uint8_t* rawData_;
    size_t bytesPerPixel_;
    size_t bytesPerLine_;
};


inline void ByteSwap(void* data, const int count)
{
    if (static_cast<unsigned int>(count) & 1U)
        throw jpegls_error{jpegls_errc::invalid_encoded_data};

    auto* const data32 = static_cast<unsigned int*>(data);
    for (auto i = 0; i < count / 4; ++i)
    {
        const auto value = data32[i];
        data32[i] = ((value >> 8U) & 0x00FF00FFU) | ((value & 0x00FF00FFU) << 8U);
    }

    auto* const data8 = static_cast<unsigned char*>(data);
    if ((count % 4) != 0)
    {
        std::swap(data8[count - 2], data8[count - 1]);
    }
}

class PostProcessSingleStream final : public ProcessLine
{
public:
    PostProcessSingleStream(std::basic_streambuf<char>* rawData, const uint32_t stride, const size_t bytesPerPixel) noexcept :
        rawData_{rawData},
        bytesPerPixel_{bytesPerPixel},
        bytesPerLine_{stride}
    {
    }

    void NewLineRequested(void* destination, const int pixelCount, int /*destStride*/) override
    {
        auto bytesToRead = static_cast<std::streamsize>(static_cast<std::streamsize>(pixelCount) * bytesPerPixel_);
        while (bytesToRead != 0)
        {
            const auto bytesRead = rawData_->sgetn(static_cast<char*>(destination), bytesToRead);
            if (bytesRead == 0)
                throw jpegls_error{jpegls_errc::destination_buffer_too_small};

            bytesToRead = bytesToRead - bytesRead;
        }

        if (bytesPerPixel_ == 2)
        {
            ByteSwap(static_cast<unsigned char*>(destination), 2 * pixelCount);
        }

        if (bytesPerLine_ - pixelCount * bytesPerPixel_ > 0)
        {
            rawData_->pubseekoff(static_cast<std::streamoff>(bytesPerLine_ - bytesToRead), std::ios_base::cur);
        }
    }

    void NewLineDecoded(const void* source, const int pixelCount, int /*sourceStride*/) override
    {
        const auto bytesToWrite = pixelCount * bytesPerPixel_;
        const auto bytesWritten = static_cast<size_t>(rawData_->sputn(static_cast<const char*>(source), static_cast<std::streamsize>(bytesToWrite)));
        if (bytesWritten != bytesToWrite)
            throw jpegls_error{jpegls_errc::destination_buffer_too_small};
    }

private:
    std::basic_streambuf<char>* rawData_;
    size_t bytesPerPixel_;
    size_t bytesPerLine_;
};


template<typename TRANSFORM, typename T>
void TransformLineToQuad(const T* ptypeInput, const int32_t pixelStrideIn, Quad<T>* byteBuffer, const int32_t pixelStride, TRANSFORM& transform) noexcept
{
    const int pixel_count = std::min(pixelStride, pixelStrideIn);
    Quad<T>* ptypeBuffer = byteBuffer;

    for (auto i = 0; i < pixel_count; ++i)
    {
        const Quad<T> pixel(transform(ptypeInput[i], ptypeInput[i + pixelStrideIn], ptypeInput[i + 2 * pixelStrideIn]), ptypeInput[i + 3 * pixelStrideIn]);
        ptypeBuffer[i] = pixel;
    }
}


template<typename TRANSFORM, typename T>
void TransformQuadToLine(const Quad<T>* byteInput, const int32_t pixelStrideIn, T* ptypeBuffer, const int32_t pixelStride, TRANSFORM& transform) noexcept
{
    const auto pixel_count = std::min(pixelStride, pixelStrideIn);
    const Quad<T>* ptypeBufferIn = byteInput;

    for (auto i = 0; i < pixel_count; ++i)
    {
        const Quad<T> color = ptypeBufferIn[i];
        const Quad<T> colorTransformed(transform(color.v1, color.v2, color.v3), color.v4);

        ptypeBuffer[i] = colorTransformed.v1;
        ptypeBuffer[i + pixelStride] = colorTransformed.v2;
        ptypeBuffer[i + 2 * pixelStride] = colorTransformed.v3;
        ptypeBuffer[i + 3 * pixelStride] = colorTransformed.v4;
    }
}


template<typename T>
void TransformRgbToBgr(T* pDest, int samplesPerPixel, const int pixelCount) noexcept
{
    for (auto i = 0; i < pixelCount; ++i)
    {
        std::swap(pDest[0], pDest[2]);
        pDest += samplesPerPixel;
    }
}


template<typename TRANSFORM, typename T>
void TransformLine(Triplet<T>* pDest, const Triplet<T>* pSrc, const int pixelCount, TRANSFORM& transform) noexcept
{
    for (auto i = 0; i < pixelCount; ++i)
    {
        pDest[i] = transform(pSrc[i].v1, pSrc[i].v2, pSrc[i].v3);
    }
}


template<typename TRANSFORM, typename T>
void TransformLine(Quad<T>* pDest, const Quad<T>* pSrc, const int pixelCount, TRANSFORM& transform) noexcept
{
    for (auto i = 0; i < pixelCount; ++i)
    {
        pDest[i] = Quad<T>(transform(pSrc[i].v1, pSrc[i].v2, pSrc[i].v3), pSrc[i].v4);
    }
}


template<typename TRANSFORM, typename T>
void TransformLineToTriplet(const T* ptypeInput, const int32_t pixelStrideIn, Triplet<T>* byteBuffer, const int32_t pixelStride, TRANSFORM& transform) noexcept
{
    const auto pixel_count = std::min(pixelStride, pixelStrideIn);
    Triplet<T>* ptypeBuffer = byteBuffer;

    for (auto i = 0; i < pixel_count; ++i)
    {
        ptypeBuffer[i] = transform(ptypeInput[i], ptypeInput[i + pixelStrideIn], ptypeInput[i + 2 * pixelStrideIn]);
    }
}


template<typename TRANSFORM, typename T>
void TransformTripletToLine(const Triplet<T>* byteInput, const int32_t pixelStrideIn, T* ptypeBuffer, const int32_t pixelStride, TRANSFORM& transform) noexcept
{
    const auto pixel_count = std::min(pixelStride, pixelStrideIn);
    const Triplet<T>* ptypeBufferIn = byteInput;

    for (auto i = 0; i < pixel_count; ++i)
    {
        const Triplet<T> color = ptypeBufferIn[i];
        const Triplet<T> colorTransformed = transform(color.v1, color.v2, color.v3);

        ptypeBuffer[i] = colorTransformed.v1;
        ptypeBuffer[i + pixelStride] = colorTransformed.v2;
        ptypeBuffer[i + 2 * pixelStride] = colorTransformed.v3;
    }
}


template<typename TRANSFORM>
class ProcessTransformed final : public ProcessLine
{
public:
    ProcessTransformed(ByteStreamInfo rawStream, const uint32_t stride, const frame_info& info, const coding_parameters& parameters, TRANSFORM transform) :
        frame_info_{info},
        parameters_{parameters},
        stride_{stride},
        tempLine_(static_cast<size_t>(info.width) * info.component_count),
        buffer_(static_cast<size_t>(info.width) * info.component_count * sizeof(size_type)),
        transform_{transform},
        inverseTransform_{transform},
        rawPixels_{rawStream}
    {
    }

    void NewLineRequested(void* dest, const int pixelCount, const int destStride) override
    {
        if (!rawPixels_.rawStream)
        {
            Transform(rawPixels_.rawData, dest, pixelCount, destStride);
            rawPixels_.rawData += stride_;
            return;
        }

        Transform(rawPixels_.rawStream, dest, pixelCount, destStride);
    }

    void Transform(std::basic_streambuf<char>* rawStream, void* destination, const int pixelCount, const int destinationStride)
    {
        std::streamsize bytesToRead = static_cast<std::streamsize>(pixelCount) * frame_info_.component_count * sizeof(size_type);
        while (bytesToRead != 0)
        {
            const auto read = rawStream->sgetn(reinterpret_cast<char*>(buffer_.data()), bytesToRead);
            if (read == 0)
                throw jpegls_error{jpegls_errc::source_buffer_too_small};

            bytesToRead -= read;
        }
        Transform(buffer_.data(), destination, pixelCount, destinationStride);
    }

    void Transform(const void* source, void* dest, int pixelCount, int destStride) noexcept
    {
        if (parameters_.output_bgr)
        {
            memcpy(tempLine_.data(), source, sizeof(Triplet<size_type>) * pixelCount);
            TransformRgbToBgr(tempLine_.data(), frame_info_.component_count, pixelCount);
            source = tempLine_.data();
        }

        if (frame_info_.component_count == 3)
        {
            if (parameters_.interleave_mode == interleave_mode::sample)
            {
                TransformLine(static_cast<Triplet<size_type>*>(dest), static_cast<const Triplet<size_type>*>(source), pixelCount, transform_);
            }
            else
            {
                TransformTripletToLine(static_cast<const Triplet<size_type>*>(source), pixelCount, static_cast<size_type*>(dest), destStride, transform_);
            }
        }
        else if (frame_info_.component_count == 4)
        {
            if (parameters_.interleave_mode == interleave_mode::sample)
            {
                TransformLine(static_cast<Quad<size_type>*>(dest), static_cast<const Quad<size_type>*>(source), pixelCount, transform_);
            }
            else if (parameters_.interleave_mode == interleave_mode::line)
            {
                TransformQuadToLine(static_cast<const Quad<size_type>*>(source), pixelCount, static_cast<size_type*>(dest), destStride, transform_);
            }
        }
    }

    void DecodeTransform(const void* pSrc, void* rawData, int pixelCount, int byteStride) noexcept
    {
        if (frame_info_.component_count == 3)
        {
            if (parameters_.interleave_mode == interleave_mode::sample)
            {
                TransformLine(static_cast<Triplet<size_type>*>(rawData), static_cast<const Triplet<size_type>*>(pSrc), pixelCount, inverseTransform_);
            }
            else
            {
                TransformLineToTriplet(static_cast<const size_type*>(pSrc), byteStride, static_cast<Triplet<size_type>*>(rawData), pixelCount, inverseTransform_);
            }
        }
        else if (frame_info_.component_count == 4)
        {
            if (parameters_.interleave_mode == interleave_mode::sample)
            {
                TransformLine(static_cast<Quad<size_type>*>(rawData), static_cast<const Quad<size_type>*>(pSrc), pixelCount, inverseTransform_);
            }
            else if (parameters_.interleave_mode == interleave_mode::line)
            {
                TransformLineToQuad(static_cast<const size_type*>(pSrc), byteStride, static_cast<Quad<size_type>*>(rawData), pixelCount, inverseTransform_);
            }
        }

        if (parameters_.output_bgr)
        {
            TransformRgbToBgr(static_cast<size_type*>(rawData), frame_info_.component_count, pixelCount);
        }
    }

    void NewLineDecoded(const void* pSrc, const int pixelCount, const int sourceStride) override
    {
        if (rawPixels_.rawStream)
        {
            const std::streamsize bytesToWrite = static_cast<std::streamsize>(pixelCount) * frame_info_.component_count * sizeof(size_type);
            DecodeTransform(pSrc, buffer_.data(), pixelCount, sourceStride);

            const auto bytesWritten = rawPixels_.rawStream->sputn(reinterpret_cast<char*>(buffer_.data()), bytesToWrite);
            if (bytesWritten != bytesToWrite)
                throw jpegls_error{jpegls_errc::destination_buffer_too_small};
        }
        else
        {
            DecodeTransform(pSrc, rawPixels_.rawData, pixelCount, sourceStride);
            rawPixels_.rawData += stride_;
        }
    }

private:
    using size_type = typename TRANSFORM::size_type;

    const frame_info& frame_info_;
    const coding_parameters& parameters_;
    const uint32_t stride_;
    std::vector<size_type> tempLine_;
    std::vector<uint8_t> buffer_;
    TRANSFORM transform_;
    typename TRANSFORM::Inverse inverseTransform_;
    ByteStreamInfo rawPixels_;
};

} // namespace charls
