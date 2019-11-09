// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

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
        throw jpegls_error{jpegls_errc::invalid_operation};

    if (parameters.bitsPerSample < MinimumBitsPerSample || parameters.bitsPerSample > MaximumBitsPerSample)
        throw jpegls_error{jpegls_errc::invalid_argument_bits_per_sample};

    if (!(parameters.interleaveMode == interleave_mode::none || parameters.interleaveMode == interleave_mode::sample || parameters.interleaveMode == interleave_mode::line))
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
        if (parameters.interleaveMode != interleave_mode::none)
            throw jpegls_error{jpegls_errc::invalid_argument_interleave_mode};
        break;
    }
}

void EncodeScan(const JlsParameters& params, int componentCount, ByteStreamInfo source, JpegStreamWriter& writer)
{
    JlsParameters info{params};
    info.components = componentCount;

    const jpegls_pc_parameters preset_coding_parameters{
        info.custom.MaximumSampleValue,
        info.custom.Threshold1,
        info.custom.Threshold2,
        info.custom.Threshold3,
        info.custom.ResetValue,
    };

    auto codec = JlsCodecFactory<EncoderStrategy>().CreateCodec(info, preset_coding_parameters);
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
            if (info.interleaveMode != interleave_mode::none)
            {
                info.stride *= info.components;
            }
        }

        JpegStreamWriter writer{destination};

        writer.WriteStartOfImage();

        writer.WriteStartOfFrameSegment(info.width, info.height, info.bitsPerSample, info.components);

        if (info.colorTransformation != color_transformation::none)
        {
            writer.WriteColorTransformSegment(info.colorTransformation);
        }

        const jpegls_pc_parameters preset_coding_parameters{
            info.custom.MaximumSampleValue,
            info.custom.Threshold1,
            info.custom.Threshold2,
            info.custom.Threshold3,
            info.custom.ResetValue,
        };

        if (!is_default(preset_coding_parameters))
        {
            writer.WriteJpegLSPresetParametersSegment(preset_coding_parameters);
        }
        else if (info.bitsPerSample > 12)
        {
            const auto default_preset_coding_parameters{compute_default((1 << info.bitsPerSample) - 1, info.allowedLossyError)};
            writer.WriteJpegLSPresetParametersSegment(default_preset_coding_parameters);
        }

        if (info.interleaveMode == interleave_mode::none)
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

        reader.ReadHeader();
        reader.ReadStartOfScan(true);

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
