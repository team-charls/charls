// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include <charls/charls.h>
#include <charls/jpegls_error.h>

#include "jpeg_marker_segment.h"
#include "jpeg_stream_reader.h"
#include "jpeg_stream_writer.h"
#include "util.h"
#include "constants.h"

using namespace charls;

namespace
{
void VerifyInput(const ByteStreamInfo& uncompressedStream, const JlsParameters& parameters)
{
    if (!uncompressedStream.rawStream && !uncompressedStream.rawData)
        throw jpegls_error(jpegls_errc::invalid_argument_destination);

    if (parameters.bitsPerSample < MinimumBitsPerSample || parameters.bitsPerSample > MaximumBitsPerSample)
        throw jpegls_error(jpegls_errc::invalid_argument_bits_per_sample);

    if (!(parameters.interleaveMode == InterleaveMode::None || parameters.interleaveMode == InterleaveMode::Sample || parameters.interleaveMode == InterleaveMode::Line))
        throw jpegls_error(jpegls_errc::invalid_argument_interleave_mode);

    if (parameters.components < 1 || parameters.components > MaximumComponentCount)
        throw jpegls_error(jpegls_errc::invalid_argument_component_count);

    if (uncompressedStream.rawData)
    {
        if (uncompressedStream.count < static_cast<size_t>(parameters.height) * parameters.width * parameters.components * (parameters.bitsPerSample > 8 ? 2 : 1))
            throw jpegls_error(jpegls_errc::destination_buffer_too_small);
    }

    switch (parameters.components)
    {
    case 3:
        break;
    case 4:
        if (parameters.interleaveMode == InterleaveMode::Sample)
            throw jpegls_error(jpegls_errc::invalid_argument_interleave_mode);
        break;
    default:
        if (parameters.interleaveMode != InterleaveMode::None)
            throw jpegls_error(jpegls_errc::invalid_argument_interleave_mode);
        break;
    }
}


jpegls_errc to_jpegls_errc() noexcept
{
    try
    {
        // re-trow the exception.
        throw;
    }
    catch (const jpegls_error& error)
    {
        return static_cast<jpegls_errc>(error.code().value());
    }
    catch (const std::bad_alloc&)
    {
        return jpegls_errc::not_enough_memory;
    }
    catch (...)
    {
        return jpegls_errc::unexpected_failure;
    }
}

} // namespace


jpegls_errc JpegLsEncodeStream(ByteStreamInfo compressedStreamInfo, size_t& bytesWritten,
                               ByteStreamInfo rawStreamInfo, const struct JlsParameters& params)
{
    if (params.width < 1 || params.width > 65535)
        return jpegls_errc::invalid_argument_width;

    if (params.height < 1 || params.height > 65535)
        return jpegls_errc::invalid_argument_height;

    try
    {
        VerifyInput(rawStreamInfo, params);

        JlsParameters info = params;
        if (info.stride == 0)
        {
            info.stride = info.width * ((info.bitsPerSample + 7) / 8);
            if (info.interleaveMode != InterleaveMode::None)
            {
                info.stride *= info.components;
            }
        }

        JpegStreamWriter writer;
        if (info.jfif.version)
        {
            writer.AddSegment(JpegMarkerSegment::CreateJpegFileInterchangeFormatSegment(info.jfif));
        }

        writer.AddSegment(JpegMarkerSegment::CreateStartOfFrameSegment(info.width, info.height, info.bitsPerSample, info.components));

        if (info.colorTransformation != ColorTransformation::None)
        {
            writer.AddColorTransform(info.colorTransformation);
        }

        if (info.interleaveMode == InterleaveMode::None)
        {
            const int32_t byteCountComponent = info.width * info.height * ((info.bitsPerSample + 7) / 8);
            for (int32_t component = 0; component < info.components; ++component)
            {
                writer.AddScan(rawStreamInfo, info);
                SkipBytes(rawStreamInfo, byteCountComponent);
            }
        }
        else
        {
            writer.AddScan(rawStreamInfo, info);
        }

        writer.Write(compressedStreamInfo);
        bytesWritten = writer.GetBytesWritten();

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}


jpegls_errc JpegLsDecodeStream(ByteStreamInfo rawStream, ByteStreamInfo compressedStream, const JlsParameters* info)
{
    try
    {
        JpegStreamReader reader(compressedStream);

        if (info)
        {
            reader.SetInfo(*info);
        }

        reader.Read(rawStream);

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}


jpegls_errc JpegLsReadHeaderStream(ByteStreamInfo rawStreamInfo, JlsParameters* params)
{
    try
    {
        JpegStreamReader reader(rawStreamInfo);
        reader.ReadHeader();
        reader.ReadStartOfScan(true);
        *params = reader.GetMetadata();

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}

extern "C"
{
    jpegls_errc CHARLS_API_CALLING_CONVENTION
    JpegLsEncode(void* destination, size_t destinationLength, size_t* bytesWritten, const void* source, size_t sourceLength, const struct JlsParameters* params, const void* /*reserved*/)
    {
        if (!destination || !bytesWritten || !source || !params)
            return jpegls_errc::invalid_argument;

        const ByteStreamInfo rawStreamInfo = FromByteArrayConst(source, sourceLength);
        const ByteStreamInfo compressedStreamInfo = FromByteArray(destination, destinationLength);

        return JpegLsEncodeStream(compressedStreamInfo, *bytesWritten, rawStreamInfo, *params);
    }


    jpegls_errc CHARLS_API_CALLING_CONVENTION
    JpegLsReadHeader(const void* compressedData, size_t compressedLength, JlsParameters* params, const void* /*reserved*/)
    {
        return JpegLsReadHeaderStream(FromByteArrayConst(compressedData, compressedLength), params);
    }


    jpegls_errc CHARLS_API_CALLING_CONVENTION
    JpegLsDecode(void* destination, size_t destinationLength, const void* source, size_t sourceLength, const struct JlsParameters* params, const void* /*reserved*/)
    {
        const ByteStreamInfo compressedStream = FromByteArrayConst(source, sourceLength);
        const ByteStreamInfo rawStreamInfo = FromByteArray(destination, destinationLength);

        return JpegLsDecodeStream(rawStreamInfo, compressedStream, params);
    }


    jpegls_errc CHARLS_API_CALLING_CONVENTION
    JpegLsDecodeRect(void* uncompressedData, size_t uncompressedLength, const void* compressedData, size_t compressedLength,
                     JlsRect roi, const JlsParameters* info, const void* /*reserved*/)
    {
        try
        {
            const ByteStreamInfo compressedStream = FromByteArrayConst(compressedData, compressedLength);
            JpegStreamReader reader(compressedStream);

            const ByteStreamInfo rawStreamInfo = FromByteArray(uncompressedData, uncompressedLength);

            if (info)
            {
                reader.SetInfo(*info);
            }

            reader.SetRect(roi);
            reader.Read(rawStreamInfo);

            return jpegls_errc::success;
        }
        catch (...)
        {
            return to_jpegls_errc();
        }
    }
}
