// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "constants.h"
#include "encoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_stream_reader.h"
#include "jpeg_stream_writer.h"
#include "jpegls_preset_coding_parameters.h"
#include "util.h"

using namespace charls;
using impl::throw_jpegls_error;

namespace {

void VerifyInput(const ByteStreamInfo& destination, const JlsParameters& parameters)
{
    if (!destination.rawStream && !destination.rawData)
        throw_jpegls_error(jpegls_errc::invalid_operation);

    if (parameters.bitsPerSample < MinimumBitsPerSample || parameters.bitsPerSample > MaximumBitsPerSample)
        throw_jpegls_error(jpegls_errc::invalid_argument_bits_per_sample);

    if (!(parameters.interleaveMode == interleave_mode::none || parameters.interleaveMode == interleave_mode::sample || parameters.interleaveMode == interleave_mode::line))
        throw_jpegls_error(jpegls_errc::invalid_argument_interleave_mode);

    if (parameters.components < 1 || parameters.components > MaximumComponentCount)
        throw_jpegls_error(jpegls_errc::invalid_argument_component_count);

    if (destination.rawData &&
        destination.count < static_cast<size_t>(parameters.height) * parameters.width * parameters.components * (parameters.bitsPerSample > 8 ? 2 : 1))
        throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

    switch (parameters.components)
    {
    case 3:
    case 4:
        break;
    default:
        if (parameters.interleaveMode != interleave_mode::none)
            throw_jpegls_error(jpegls_errc::invalid_argument_interleave_mode);
        break;
    }
}

void EncodeScan(const JlsParameters& params, const int componentCount, const ByteStreamInfo source, JpegStreamWriter& writer)
{
    const frame_info frame_info{static_cast<uint32_t>(params.width), static_cast<uint32_t>(params.height), params.bitsPerSample, componentCount};
    const coding_parameters codec_parameters{params.allowedLossyError, params.interleaveMode, params.colorTransformation, false};
    const jpegls_pc_parameters preset_coding_parameters{
        params.custom.MaximumSampleValue,
        params.custom.Threshold1,
        params.custom.Threshold2,
        params.custom.Threshold3,
        params.custom.ResetValue,
    };

    auto codec = JlsCodecFactory<EncoderStrategy>().CreateCodec(frame_info, codec_parameters, preset_coding_parameters);
    std::unique_ptr<ProcessLine> processLine(codec->CreateProcess(source, static_cast<uint32_t>(params.stride)));
    ByteStreamInfo destination{writer.OutputStream()};
    const size_t bytesWritten = codec->EncodeScan(move(processLine), destination);

    // Synchronize the destination encapsulated in the writer (EncodeScan works on a local copy)
    writer.Seek(bytesWritten);
}

} // namespace


jpegls_errc JpegLsEncodeStream(const ByteStreamInfo destination, size_t& bytesWritten,
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
            const auto default_preset_coding_parameters{compute_default((1U << static_cast<uint32_t>(info.bitsPerSample)) - 1, info.allowedLossyError)};
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
                SkipBytes(source, static_cast<size_t>(byteCountComponent));
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


jpegls_errc JpegLsDecodeStream(const ByteStreamInfo destination, const ByteStreamInfo source, const JlsParameters* /*params*/)
{
    try
    {
        JpegStreamReader reader{source};

        reader.ReadHeader();
        reader.ReadStartOfScan();

        reader.Read(destination, 0);

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}


jpegls_errc JpegLsReadHeaderStream(const ByteStreamInfo source, JlsParameters* params)
{
    try
    {
        JpegStreamReader reader{source};
        reader.ReadHeader();
        reader.ReadStartOfScan();
        *params = JlsParameters{};

        const auto& info = reader.frame_info();
        params->height = static_cast<int32_t>(info.height);
        params->width = static_cast<int32_t>(info.width);
        params->bitsPerSample = info.bits_per_sample;
        params->components = info.component_count;

        const auto& parameters = reader.parameters();
        params->interleaveMode = parameters.interleave_mode;
        params->allowedLossyError = parameters.near_lossless;
        params->colorTransformation = parameters.transformation;

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}
