// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "encoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_stream_reader.h"
#include "jpeg_stream_writer.h"
#include "util.h"

using namespace charls;
using impl::throw_jpegls_error;

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
