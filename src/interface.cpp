// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include <charls/charls.h>
#include <charls/jpegls_error.h>

#include "jpeg_stream_reader.h"
#include "jpeg_stream_writer.h"
#include "jpegls_preset_coding_parameters.h"
#include "encoder_strategy.h"
#include "jls_codec_factory.h"
#include "util.h"
#include "constants.h"

using namespace charls;

namespace {

void VerifyInput(const ByteStreamInfo& destination, const JlsParameters& parameters)
{
    if (!destination.rawStream && !destination.rawData)
        throw jpegls_error{jpegls_errc::invalid_argument_destination};

    if (parameters.bitsPerSample < MinimumBitsPerSample || parameters.bitsPerSample > MaximumBitsPerSample)
        throw jpegls_error{jpegls_errc::invalid_argument_bits_per_sample};

    if (!(parameters.interleaveMode == InterleaveMode::None || parameters.interleaveMode == InterleaveMode::Sample || parameters.interleaveMode == InterleaveMode::Line))
        throw jpegls_error{jpegls_errc::invalid_argument_interleave_mode};

    if (parameters.components < 1 || parameters.components > MaximumComponentCount)
        throw jpegls_error{jpegls_errc::invalid_argument_component_count};

    if (destination.rawData &&
        destination.count < static_cast<size_t>(parameters.height) * parameters.width * parameters.components * (parameters.bitsPerSample > 8 ? 2 : 1))
        throw jpegls_error{jpegls_errc::destination_buffer_too_small};

    switch (parameters.components)
    {
    case 3:
    case 4:
        break;
    default:
        if (parameters.interleaveMode != InterleaveMode::None)
            throw jpegls_error{jpegls_errc::invalid_argument_interleave_mode};
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

void EncodeScan(const JlsParameters& params, int componentCount, ByteStreamInfo source, JpegStreamWriter& writer)
{
    JlsParameters info{params};
    info.components = componentCount;

    auto codec = JlsCodecFactory<EncoderStrategy>().CreateCodec(info, info.custom);
    std::unique_ptr<ProcessLine> processLine(codec->CreateProcess(source));
    ByteStreamInfo destination{writer.OutputStream()};
    const size_t bytesWritten = codec->EncodeScan(move(processLine), destination);

    // Synchronize the destination encapsulated in the writer (EncodeScan works on a local copy)
    writer.Seek(bytesWritten);
}

} // namespace


jpegls_errc JpegLsEncodeStream(ByteStreamInfo destination, size_t& bytesWritten,
                               ByteStreamInfo source, const JlsParameters& params)
{
    if (params.width < 1 || params.width > 65535)
        return jpegls_errc::invalid_argument_width;

    if (params.height < 1 || params.height > 65535)
        return jpegls_errc::invalid_argument_height;

    try
    {
        VerifyInput(source, params);

        JlsParameters info{params};
        if (info.stride == 0)
        {
            info.stride = info.width * ((info.bitsPerSample + 7) / 8);
            if (info.interleaveMode != InterleaveMode::None)
            {
                info.stride *= info.components;
            }
        }

        JpegStreamWriter writer{destination};

        writer.WriteStartOfImage();

        if (info.jfif.version != 0)
        {
            writer.WriteJpegFileInterchangeFormatSegment(info.jfif);
        }

        writer.WriteStartOfFrameSegment(info.width, info.height, info.bitsPerSample, info.components);

        if (info.colorTransformation != ColorTransformation::None)
        {
            writer.WriteColorTransformSegment(info.colorTransformation);
        }

        if (!IsDefault(info.custom))
        {
            writer.WriteJpegLSPresetParametersSegment(info.custom);
        }
        else if (info.bitsPerSample > 12)
        {
            const JpegLSPresetCodingParameters preset = ComputeDefault((1 << info.bitsPerSample) - 1, info.allowedLossyError);
            writer.WriteJpegLSPresetParametersSegment(preset);
        }

        if (info.interleaveMode == InterleaveMode::None)
        {
            const int32_t byteCountComponent = info.width * info.height * ((info.bitsPerSample + 7) / 8);
            for (int32_t component = 0; component < info.components; ++component)
            {
                writer.WriteStartOfScanSegment(1, info.allowedLossyError, info.interleaveMode);
                EncodeScan(info, 1, source, writer);

                // Synchronize the source stream (EncodeScan works on a local copy)
                SkipBytes(source, byteCountComponent);
            }
        }
        else
        {
            writer.WriteStartOfScanSegment(info.components, info.allowedLossyError, info.interleaveMode);
            EncodeScan(info, info.components, source, writer);
        }

        writer.WriteEndOfImage();

        bytesWritten = writer.GetBytesWritten();

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}


jpegls_errc JpegLsDecodeStream(ByteStreamInfo destination, ByteStreamInfo source, const JlsParameters* params)
{
    try
    {
        JpegStreamReader reader{source};

        if (params)
        {
            reader.SetInfo(*params);
        }

        reader.Read(destination);

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}


jpegls_errc JpegLsReadHeaderStream(ByteStreamInfo source, JlsParameters* params)
{
    try
    {
        JpegStreamReader reader{source};
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

extern "C" {

jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsEncode(void* destination, size_t destinationLength, size_t* bytesWritten, const void* source, size_t sourceLength, const struct JlsParameters* params, const void* /*reserved*/)
{
    if (!destination || !bytesWritten || !source || !params)
        return jpegls_errc::invalid_argument;

    const ByteStreamInfo sourceInfo{FromByteArrayConst(source, sourceLength)};
    const ByteStreamInfo destinationInfo{FromByteArray(destination, destinationLength)};

    return JpegLsEncodeStream(destinationInfo, *bytesWritten, sourceInfo, *params);
}


jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsReadHeader(const void* source, size_t sourceLength, JlsParameters* params, const void* /*reserved*/)
{
    return JpegLsReadHeaderStream(FromByteArrayConst(source, sourceLength), params);
}


jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsDecode(void* destination, size_t destinationLength, const void* source, size_t sourceLength, const struct JlsParameters* params, const void* /*reserved*/)
{
    const ByteStreamInfo compressedStream{FromByteArrayConst(source, sourceLength)};
    const ByteStreamInfo rawStreamInfo{FromByteArray(destination, destinationLength)};

    return JpegLsDecodeStream(rawStreamInfo, compressedStream, params);
}


jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsDecodeRect(void* destination, size_t destinationLength, const void* source, size_t sourceLength,
                 JlsRect roi, const JlsParameters* params, const void* /*reserved*/)
{
    try
    {
        const ByteStreamInfo sourceInfo{FromByteArrayConst(source, sourceLength)};
        JpegStreamReader reader{sourceInfo};
        const ByteStreamInfo destinationInfo{FromByteArray(destination, destinationLength)};

        if (params)
        {
            reader.SetInfo(*params);
        }

        reader.SetRect(roi);
        reader.Read(destinationInfo);

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}

}
