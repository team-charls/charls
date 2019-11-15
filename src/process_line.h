// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <charls/jpegls_error.h>
#include <charls/charls_legacy.h>

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

    void NewLineRequested(void* destination, int pixelCount, int /*byteStride*/) noexcept(false) override
    {
        std::memcpy(destination, rawData_, pixelCount * bytesPerPixel_);
        rawData_ += bytesPerLine_;
    }

    void NewLineDecoded(const void* source, int pixelCount, int /*sourceStride*/) noexcept(false) override
    {
        std::memcpy(rawData_, source, pixelCount * bytesPerPixel_);
        rawData_ += bytesPerLine_;
    }

private:
    uint8_t* rawData_;
    size_t bytesPerPixel_;
    size_t bytesPerLine_;
};


inline void ByteSwap(void* data, int count)
{
    if (static_cast<unsigned int>(count) & 1U)
        throw jpegls_error{jpegls_errc::invalid_encoded_data};

    const auto data32 = static_cast<unsigned int*>(data);
    for (auto i = 0; i < count / 4; i++)
    {
        const auto value = data32[i];
        data32[i] = ((value >> 8U) & 0x00FF00FFU) | ((value & 0x00FF00FFU) << 8U);
    }

    const auto data8 = static_cast<unsigned char*>(data);
    if ((count % 4) != 0)
    {
        std::swap(data8[count - 2], data8[count - 1]);
    }
}

class PostProcessSingleStream final : public ProcessLine
{
public:
    PostProcessSingleStream(std::basic_streambuf<char>* rawData, uint32_t stride, size_t bytesPerPixel) noexcept :
        rawData_{rawData},
        bytesPerPixel_{bytesPerPixel},
        bytesPerLine_{stride}
    {
    }

    void NewLineRequested(void* destination, int pixelCount, int /*destStride*/) override
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

    void NewLineDecoded(const void* source, int pixelCount, int /*sourceStride*/) override
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
void TransformLineToQuad(const T* ptypeInput, int32_t pixelStrideIn, Quad<T>* byteBuffer, int32_t pixelStride, TRANSFORM& transform) noexcept
{
    const int cpixel = std::min(pixelStride, pixelStrideIn);
    Quad<T>* ptypeBuffer = byteBuffer;

    for (auto x = 0; x < cpixel; ++x)
    {
        const Quad<T> pixel(transform(ptypeInput[x], ptypeInput[x + pixelStrideIn], ptypeInput[x + 2 * pixelStrideIn]), ptypeInput[x + 3 * pixelStrideIn]);
        ptypeBuffer[x] = pixel;
    }
}


template<typename TRANSFORM, typename T>
void TransformQuadToLine(const Quad<T>* byteInput, int32_t pixelStrideIn, T* ptypeBuffer, int32_t pixelStride, TRANSFORM& transform) noexcept
{
    const auto cpixel = std::min(pixelStride, pixelStrideIn);
    const Quad<T>* ptypeBufferIn = byteInput;

    for (auto x = 0; x < cpixel; ++x)
    {
        const Quad<T> color = ptypeBufferIn[x];
        const Quad<T> colorTransformed(transform(color.v1, color.v2, color.v3), color.v4);

        ptypeBuffer[x] = colorTransformed.v1;
        ptypeBuffer[x + pixelStride] = colorTransformed.v2;
        ptypeBuffer[x + 2 * pixelStride] = colorTransformed.v3;
        ptypeBuffer[x + 3 * pixelStride] = colorTransformed.v4;
    }
}


template<typename T>
void TransformRgbToBgr(T* pDest, int samplesPerPixel, int pixelCount) noexcept
{
    for (auto i = 0; i < pixelCount; ++i)
    {
        std::swap(pDest[0], pDest[2]);
        pDest += samplesPerPixel;
    }
}


template<typename TRANSFORM, typename T>
void TransformLine(Triplet<T>* pDest, const Triplet<T>* pSrc, int pixelCount, TRANSFORM& transform) noexcept
{
    for (auto i = 0; i < pixelCount; ++i)
    {
        pDest[i] = transform(pSrc[i].v1, pSrc[i].v2, pSrc[i].v3);
    }
}


template<typename TRANSFORM, typename T>
void TransformLine(Quad<T>* pDest, const Quad<T>* pSrc, int pixelCount, TRANSFORM& transform) noexcept
{
    for (auto i = 0; i < pixelCount; ++i)
    {
        pDest[i] = Quad<T>(transform(pSrc[i].v1, pSrc[i].v2, pSrc[i].v3), pSrc[i].v4);
    }
}


template<typename TRANSFORM, typename T>
void TransformLineToTriplet(const T* ptypeInput, int32_t pixelStrideIn, Triplet<T>* byteBuffer, int32_t pixelStride, TRANSFORM& transform) noexcept
{
    const auto cpixel = std::min(pixelStride, pixelStrideIn);
    Triplet<T>* ptypeBuffer = byteBuffer;

    for (auto x = 0; x < cpixel; ++x)
    {
        ptypeBuffer[x] = transform(ptypeInput[x], ptypeInput[x + pixelStrideIn], ptypeInput[x + 2 * pixelStrideIn]);
    }
}


template<typename TRANSFORM, typename T>
void TransformTripletToLine(const Triplet<T>* byteInput, int32_t pixelStrideIn, T* ptypeBuffer, int32_t pixelStride, TRANSFORM& transform) noexcept
{
    const auto cpixel = std::min(pixelStride, pixelStrideIn);
    const Triplet<T>* ptypeBufferIn = byteInput;

    for (auto x = 0; x < cpixel; ++x)
    {
        const Triplet<T> color = ptypeBufferIn[x];
        const Triplet<T> colorTransformed = transform(color.v1, color.v2, color.v3);

        ptypeBuffer[x] = colorTransformed.v1;
        ptypeBuffer[x + pixelStride] = colorTransformed.v2;
        ptypeBuffer[x + 2 * pixelStride] = colorTransformed.v3;
    }
}


template<typename TRANSFORM>
class ProcessTransformed final : public ProcessLine
{
public:
    ProcessTransformed(ByteStreamInfo rawStream, const JlsParameters& info, TRANSFORM transform) :
        params_{info},
        tempLine_(static_cast<size_t>(info.width) * info.components),
        buffer_(static_cast<size_t>(info.width) * info.components * sizeof(size_type)),
        transform_{transform},
        inverseTransform_{transform},
        rawPixels_{rawStream}
    {
    }

    void NewLineRequested(void* dest, int pixelCount, int destStride) override
    {
        if (!rawPixels_.rawStream)
        {
            Transform(rawPixels_.rawData, dest, pixelCount, destStride);
            rawPixels_.rawData += params_.stride;
            return;
        }

        Transform(rawPixels_.rawStream, dest, pixelCount, destStride);
    }

    void Transform(std::basic_streambuf<char>* rawStream, void* destination, int pixelCount, int destinationStride)
    {
        std::streamsize bytesToRead = static_cast<std::streamsize>(pixelCount) * params_.components * sizeof(size_type);
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
        if (params_.outputBgr)
        {
            memcpy(tempLine_.data(), source, sizeof(Triplet<size_type>) * pixelCount);
            TransformRgbToBgr(tempLine_.data(), params_.components, pixelCount);
            source = tempLine_.data();
        }

        if (params_.components == 3)
        {
            if (params_.interleaveMode == interleave_mode::sample)
            {
                TransformLine(static_cast<Triplet<size_type>*>(dest), static_cast<const Triplet<size_type>*>(source), pixelCount, transform_);
            }
            else
            {
                TransformTripletToLine(static_cast<const Triplet<size_type>*>(source), pixelCount, static_cast<size_type*>(dest), destStride, transform_);
            }
        }
        else if (params_.components == 4)
        {
            if (params_.interleaveMode == interleave_mode::sample)
            {
                TransformLine(static_cast<Quad<size_type>*>(dest), static_cast<const Quad<size_type>*>(source), pixelCount, transform_);
            }
            else if (params_.interleaveMode == interleave_mode::line)
            {
                TransformQuadToLine(static_cast<const Quad<size_type>*>(source), pixelCount, static_cast<size_type*>(dest), destStride, transform_);
            }
        }
    }

    void DecodeTransform(const void* pSrc, void* rawData, int pixelCount, int byteStride) noexcept
    {
        if (params_.components == 3)
        {
            if (params_.interleaveMode == interleave_mode::sample)
            {
                TransformLine(static_cast<Triplet<size_type>*>(rawData), static_cast<const Triplet<size_type>*>(pSrc), pixelCount, inverseTransform_);
            }
            else
            {
                TransformLineToTriplet(static_cast<const size_type*>(pSrc), byteStride, static_cast<Triplet<size_type>*>(rawData), pixelCount, inverseTransform_);
            }
        }
        else if (params_.components == 4)
        {
            if (params_.interleaveMode == interleave_mode::sample)
            {
                TransformLine(static_cast<Quad<size_type>*>(rawData), static_cast<const Quad<size_type>*>(pSrc), pixelCount, inverseTransform_);
            }
            else if (params_.interleaveMode == interleave_mode::line)
            {
                TransformLineToQuad(static_cast<const size_type*>(pSrc), byteStride, static_cast<Quad<size_type>*>(rawData), pixelCount, inverseTransform_);
            }
        }

        if (params_.outputBgr)
        {
            TransformRgbToBgr(static_cast<size_type*>(rawData), params_.components, pixelCount);
        }
    }

    void NewLineDecoded(const void* pSrc, int pixelCount, int sourceStride) override
    {
        if (rawPixels_.rawStream)
        {
            const std::streamsize bytesToWrite = static_cast<std::streamsize>(pixelCount) * params_.components * sizeof(size_type);
            DecodeTransform(pSrc, buffer_.data(), pixelCount, sourceStride);

            const auto bytesWritten = rawPixels_.rawStream->sputn(reinterpret_cast<char*>(buffer_.data()), bytesToWrite);
            if (bytesWritten != bytesToWrite)
                throw jpegls_error{jpegls_errc::destination_buffer_too_small};
        }
        else
        {
            DecodeTransform(pSrc, rawPixels_.rawData, pixelCount, sourceStride);
            rawPixels_.rawData += params_.stride;
        }
    }

private:
    using size_type = typename TRANSFORM::size_type;

    const JlsParameters& params_;
    std::vector<size_type> tempLine_;
    std::vector<uint8_t> buffer_;
    TRANSFORM transform_;
    typename TRANSFORM::Inverse inverseTransform_;
    ByteStreamInfo rawPixels_;
};

} // namespace charls
