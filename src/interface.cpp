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

void verify_input(const byte_stream_info& destination, const JlsParameters& parameters)
{
    if (!destination.rawStream && !destination.rawData)
        throw_jpegls_error(jpegls_errc::invalid_operation);

    if (parameters.bitsPerSample < minimum_bits_per_sample || parameters.bitsPerSample > maximum_bits_per_sample)
        throw_jpegls_error(jpegls_errc::invalid_argument_bits_per_sample);

    if (!(parameters.interleaveMode == interleave_mode::none || parameters.interleaveMode == interleave_mode::sample || parameters.interleaveMode == interleave_mode::line))
        throw_jpegls_error(jpegls_errc::invalid_argument_interleave_mode);

    if (parameters.components < 1 || parameters.components > maximum_component_count)
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

void encode_scan(const JlsParameters& params, const int component_count, const byte_stream_info source, jpeg_stream_writer& writer)
{
    const frame_info frame_info{static_cast<uint32_t>(params.width), static_cast<uint32_t>(params.height), params.bitsPerSample, component_count};
    const coding_parameters codec_parameters{params.allowedLossyError, params.interleaveMode, params.colorTransformation, false};
    const jpegls_pc_parameters preset_coding_parameters{
        params.custom.MaximumSampleValue,
        params.custom.Threshold1,
        params.custom.Threshold2,
        params.custom.Threshold3,
        params.custom.ResetValue,
    };

    auto codec = jls_codec_factory<encoder_strategy>().create_codec(frame_info, codec_parameters, preset_coding_parameters);
    std::unique_ptr<process_line> process_line(codec->create_process_line(source, static_cast<uint32_t>(params.stride)));
    byte_stream_info destination{writer.output_stream()};
    const size_t bytes_written = codec->encode_scan(move(process_line), destination);

    // Synchronize the destination encapsulated in the writer (encode_scan works on a local copy)
    writer.seek(bytes_written);
}

} // namespace


jpegls_errc JpegLsEncodeStream(const byte_stream_info destination, size_t& bytes_written,
                               byte_stream_info source, const JlsParameters& params)
{
    if (params.width < 1 || params.width > 65535)
        return jpegls_errc::invalid_argument_width;

    if (params.height < 1 || params.height > 65535)
        return jpegls_errc::invalid_argument_height;

    try
    {
        verify_input(source, params);

        JlsParameters info{params};
        if (info.stride == 0)
        {
            info.stride = info.width * static_cast<int32_t>(bit_to_byte_count(info.bitsPerSample));
            if (info.interleaveMode != interleave_mode::none)
            {
                info.stride *= info.components;
            }
        }

        jpeg_stream_writer writer{destination};

        writer.write_start_of_image();

        writer.write_start_of_frame_segment(info.width, info.height, info.bitsPerSample, info.components);

        if (info.colorTransformation != color_transformation::none)
        {
            writer.write_color_transform_segment(info.colorTransformation);
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
            writer.write_jpegls_preset_parameters_segment(preset_coding_parameters);
        }
        else if (info.bitsPerSample > 12)
        {
            const auto default_preset_coding_parameters{compute_default((1U << static_cast<uint32_t>(info.bitsPerSample)) - 1, info.allowedLossyError)};
            writer.write_jpegls_preset_parameters_segment(default_preset_coding_parameters);
        }

        if (info.interleaveMode == interleave_mode::none)
        {
            const int32_t byte_count_component = info.width * info.height * static_cast<int32_t>(bit_to_byte_count(info.bitsPerSample));
            for (int32_t component = 0; component < info.components; ++component)
            {
                writer.write_start_of_scan_segment(1, info.allowedLossyError, info.interleaveMode);
                encode_scan(info, 1, source, writer);

                // Synchronize the source stream (encode_scan works on a local copy)
                skip_bytes(source, static_cast<size_t>(byte_count_component));
            }
        }
        else
        {
            writer.write_start_of_scan_segment(info.components, info.allowedLossyError, info.interleaveMode);
            encode_scan(info, info.components, source, writer);
        }

        writer.write_end_of_image();

        bytes_written = writer.bytes_written();

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}


jpegls_errc JpegLsDecodeStream(const byte_stream_info destination, const byte_stream_info source, const JlsParameters* /*params*/)
{
    try
    {
        jpeg_stream_reader reader{source};

        reader.read_header();
        reader.read_start_of_scan();

        reader.read(destination, 0);

        return jpegls_errc::success;
    }
    catch (...)
    {
        return to_jpegls_errc();
    }
}


jpegls_errc JpegLsReadHeaderStream(const byte_stream_info source, JlsParameters* params)
{
    try
    {
        jpeg_stream_reader reader{source};
        reader.read_header();
        reader.read_start_of_scan();
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
