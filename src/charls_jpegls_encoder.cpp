// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <charls/charls.h>

#include "encoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_stream_writer.h"
#include "jpegls_preset_coding_parameters.h"
#include "util.h"

#include <cassert>
#include <new>

using namespace charls;
using std::unique_ptr;

struct charls_jpegls_encoder final
{
    charls_jpegls_encoder() = default;

    void destination(void* destination, const size_t size)
    {
        if (state_ != state::initial)
            throw jpegls_error{jpegls_errc::invalid_operation};

        writer_.UpdateDestination(destination, size);
        state_ = state::destination_set;
    }

    void frame_info(const charls_frame_info& frame_info)
    {
        if (frame_info.width < 1 || frame_info.width > maximum_width)
            throw jpegls_error{jpegls_errc::invalid_argument_width};

        if (frame_info.height < 1 || frame_info.height > maximum_height)
            throw jpegls_error{jpegls_errc::invalid_argument_height};

        if (frame_info.bits_per_sample < MinimumBitsPerSample || frame_info.bits_per_sample > MaximumBitsPerSample)
            throw jpegls_error{jpegls_errc::invalid_argument_bits_per_sample};

        if (frame_info.component_count < 1 || frame_info.component_count > MaximumComponentCount)
            throw jpegls_error{jpegls_errc::invalid_argument_component_count};

        frame_info_ = frame_info;
    }

    void interleave_mode(const charls::interleave_mode interleave_mode)
    {
        if (interleave_mode < charls::interleave_mode::none || interleave_mode > charls::interleave_mode::sample)
            throw jpegls_error{jpegls_errc::invalid_argument_interleave_mode};

        interleave_mode_ = interleave_mode;
    }

    void near_lossless(const int32_t near_lossless)
    {
        if (near_lossless < 0 || near_lossless > maximum_near_lossless)
            throw jpegls_error{jpegls_errc::invalid_argument_near_lossless};

        near_lossless_ = near_lossless;
    }

    void preset_coding_parameters(const jpegls_pc_parameters& preset_coding_parameters)
    {
        if (!is_valid(preset_coding_parameters, UINT16_MAX, near_lossless_))
            throw jpegls_error{jpegls_errc::invalid_argument_pc_parameters};

        preset_coding_parameters_ = preset_coding_parameters;
    }

    void color_transformation(const color_transformation color_transformation)
    {
        if (color_transformation < charls::color_transformation::none || color_transformation > charls::color_transformation::hp3)
            throw jpegls_error{jpegls_errc::invalid_argument_color_transformation};

        color_transformation_ = color_transformation;
    }

    size_t estimated_destination_size() const
    {
        if (!is_frame_info_configured())
            throw jpegls_error{jpegls_errc::invalid_operation};

        return static_cast<size_t>(frame_info_.width) * frame_info_.height *
                   frame_info_.component_count * (frame_info_.bits_per_sample < 9 ? 1 : 2) +
                   1024 + spiff_header_size_in_bytes;
    }

    void write_spiff_header(const spiff_header& spiff_header)
    {
        if (spiff_header.height == 0)
            throw jpegls_error{jpegls_errc::invalid_argument_height};

        if (spiff_header.width == 0)
            throw jpegls_error{jpegls_errc::invalid_argument_width};

        if (state_ != state::destination_set)
            throw jpegls_error{jpegls_errc::invalid_operation};

        writer_.WriteStartOfImage();
        writer_.WriteSpiffHeaderSegment(spiff_header);
        state_ = state::spiff_header;
    }

    void write_standard_spiff_header(spiff_color_space color_space, const spiff_resolution_units resolution_units, const uint32_t vertical_resolution, const uint32_t horizontal_resolution)
    {
        if (!is_frame_info_configured())
            throw jpegls_error{jpegls_errc::invalid_operation};

        write_spiff_header({spiff_profile_id::none,
                            frame_info_.component_count,
                            frame_info_.height,
                            frame_info_.width,
                            color_space,
                            frame_info_.bits_per_sample,
                            spiff_compression_type::jpeg_ls,
                            resolution_units,
                            vertical_resolution,
                            horizontal_resolution});
    }

    void write_spiff_entry(const uint32_t entry_tag, const void* entry_data, const size_t entry_data_size)
    {
        if (entry_tag == spiff_end_of_directory_entry_type)
            throw jpegls_error{jpegls_errc::invalid_argument};

        if (entry_data_size > 65528)
            throw jpegls_error{jpegls_errc::invalid_argument_spiff_entry_size};

        if (state_ != state::spiff_header)
            throw jpegls_error{jpegls_errc::invalid_operation};

        writer_.WriteSpiffDirectoryEntry(entry_tag, entry_data, entry_data_size);
    }

    void encode(const void* source, const size_t source_size, uint32_t stride)
    {
        if (!is_frame_info_configured() || state_ == state::initial)
            throw jpegls_error{jpegls_errc::invalid_operation};

        if (stride == 0)
        {
            stride = frame_info_.width * ((frame_info_.bits_per_sample + 7) / 8);
            if (interleave_mode_ != charls::interleave_mode::none)
            {
                stride *= frame_info_.component_count;
            }
        }

        if (state_ == state::spiff_header)
        {
            writer_.WriteSpiffEndOfDirectoryEntry();
        }
        else
        {
            writer_.WriteStartOfImage();
        }

        writer_.WriteStartOfFrameSegment(frame_info_.width, frame_info_.height, frame_info_.bits_per_sample, frame_info_.component_count);

        if (color_transformation_ != charls::color_transformation::none)
        {
            writer_.WriteColorTransformSegment(color_transformation_);
        }

        if (!is_default(preset_coding_parameters_))
        {
            writer_.WriteJpegLSPresetParametersSegment(preset_coding_parameters_);
        }
        else if (frame_info_.bits_per_sample > 12)
        {
            const jpegls_pc_parameters preset = compute_default((1 << frame_info_.bits_per_sample) - 1, near_lossless_);
            writer_.WriteJpegLSPresetParametersSegment(preset);
        }

        ByteStreamInfo sourceInfo = FromByteArrayConst(source, source_size);
        if (interleave_mode_ == charls::interleave_mode::none)
        {
            const int32_t byteCountComponent = frame_info_.width * frame_info_.height * ((frame_info_.bits_per_sample + 7) / 8);
            for (int32_t component = 0; component < frame_info_.component_count; ++component)
            {
                writer_.WriteStartOfScanSegment(1, near_lossless_, interleave_mode_);
                encode_scan(sourceInfo, stride, 1);

                // Synchronize the source stream (EncodeScan works on a local copy)
                SkipBytes(sourceInfo, byteCountComponent);
            }
        }
        else
        {
            writer_.WriteStartOfScanSegment(frame_info_.component_count, near_lossless_, interleave_mode_);
            encode_scan(sourceInfo, stride, frame_info_.component_count);
        }

        writer_.WriteEndOfImage();
    }

    size_t bytes_written() const noexcept
    {
        return writer_.GetBytesWritten();
    }

private:
    enum class state
    {
        initial,
        destination_set,
        spiff_header,
        completed,
    };

    bool is_frame_info_configured() const noexcept
    {
        return frame_info_.width != 0;
    }

    void encode_scan(ByteStreamInfo source, const uint32_t stride, const int32_t component_count)
    {
        JlsParameters info{};
        info.components = component_count;
        info.bitsPerSample = frame_info_.bits_per_sample;
        info.height = frame_info_.height;
        info.width = frame_info_.width;
        info.stride = stride;
        info.interleaveMode = interleave_mode_;
        info.allowedLossyError = near_lossless_;

        auto codec = JlsCodecFactory<EncoderStrategy>().CreateCodec(info, preset_coding_parameters_);
        unique_ptr<ProcessLine> processLine(codec->CreateProcess(source));
        ByteStreamInfo destination{writer_.OutputStream()};
        const size_t bytesWritten = codec->EncodeScan(move(processLine), destination);

        // Synchronize the destination encapsulated in the writer (EncodeScan works on a local copy)
        writer_.Seek(bytesWritten);
    }

    charls_frame_info frame_info_{};
    int32_t near_lossless_{};
    charls::interleave_mode interleave_mode_{};
    charls::color_transformation color_transformation_{};
    state state_{};
    JpegStreamWriter writer_;
    jpegls_pc_parameters preset_coding_parameters_{};
};

extern "C" {

#if defined _MSC_VER && _MSC_VER < 1917
#pragma warning(disable : 26447) // Function is declared 'noexcept' but calls function '' which may throw exceptions (f.6). [false warning in VS 2017]
#endif

charls_jpegls_encoder* CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_create() noexcept
{
    MSVC_WARNING_SUPPRESS(26402 26409) // don't use new and delete + scoped object and move
    return new (std::nothrow) charls_jpegls_encoder;
    MSVC_WARNING_UNSUPPRESS()
}

void CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_destroy(const charls_jpegls_encoder* encoder) noexcept
{
    MSVC_WARNING_SUPPRESS(26401 26409) // don't use new and delete + non-owner.
    delete encoder;
    MSVC_WARNING_UNSUPPRESS()
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_destination_buffer(charls_jpegls_encoder* encoder, void* destination_buffer, size_t destination_size) noexcept
try
{
    if (!encoder || !destination_buffer)
        return jpegls_errc::invalid_argument;

    encoder->destination(destination_buffer, destination_size);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_frame_info(charls_jpegls_encoder* encoder, const charls_frame_info* frame_info) noexcept
try
{
    if (!encoder || !frame_info)
        return jpegls_errc::invalid_argument;

    encoder->frame_info(*frame_info);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_near_lossless(charls_jpegls_encoder* encoder, int32_t near_lossless) noexcept
try
{
    if (!encoder)
        return jpegls_errc::invalid_argument;

    encoder->near_lossless(near_lossless);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_interleave_mode(charls_jpegls_encoder* encoder, charls_interleave_mode interleave_mode) noexcept
try
{
    if (!encoder)
        return jpegls_errc::invalid_argument;

    encoder->interleave_mode(interleave_mode);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_preset_coding_parameters(charls_jpegls_encoder* encoder, const charls_jpegls_pc_parameters* preset_coding_parameters) noexcept
try
{
    if (!encoder || !preset_coding_parameters)
        return jpegls_errc::invalid_argument;

    encoder->preset_coding_parameters(*preset_coding_parameters);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_color_transformation(charls_jpegls_encoder* encoder, charls_color_transformation color_transformation) noexcept
try
{
    if (!encoder)
        return jpegls_errc::invalid_argument;

    encoder->color_transformation(color_transformation);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_estimated_destination_size(const charls_jpegls_encoder* encoder, size_t* size_in_bytes) noexcept
try
{
    if (!encoder || !size_in_bytes)
        return jpegls_errc::invalid_argument;

    *size_in_bytes = encoder->estimated_destination_size();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_bytes_written(const charls_jpegls_encoder* encoder, size_t* bytes_written) noexcept
{
    if (!encoder || !bytes_written)
        return jpegls_errc::invalid_argument;

    *bytes_written = encoder->bytes_written();
    return jpegls_errc::success;
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_encode_from_buffer(charls_jpegls_encoder* encoder, const void* source_buffer, const size_t source_size, const uint32_t stride) noexcept
try
{
    if (!encoder || !source_buffer)
        return jpegls_errc::invalid_argument;

    encoder->encode(source_buffer, source_size, stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_header(charls_jpegls_encoder* encoder, const charls_spiff_header* spiff_header) noexcept
try
{
    if (!encoder || !spiff_header)
        return jpegls_errc::invalid_argument;

    encoder->write_spiff_header(*spiff_header);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_standard_spiff_header(charls_jpegls_encoder* encoder, const charls_spiff_color_space color_space,
                                                  const charls_spiff_resolution_units resolution_units, const uint32_t vertical_resolution, const uint32_t horizontal_resolution) noexcept
try
{
    if (!encoder)
        return jpegls_errc::invalid_argument;

    encoder->write_standard_spiff_header(color_space, resolution_units, vertical_resolution, horizontal_resolution);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_entry(charls_jpegls_encoder* encoder, const uint32_t entry_tag, const void* entry_data, const size_t entry_data_size) noexcept
try
{
    if (!encoder || (!entry_data && entry_data_size != 0))
        return jpegls_errc::invalid_argument;

    encoder->write_spiff_entry(entry_tag, entry_data, entry_data_size);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsEncode(void* destination, size_t destinationLength, size_t* bytesWritten, const void* source, size_t sourceLength, const struct JlsParameters* params, char* errorMessage)
try
{
    if (!destination || !bytesWritten || !source || !params || params->jfif.version)
        return jpegls_errc::invalid_argument;

    charls_jpegls_encoder encoder;
    encoder.destination(destination, destinationLength);
    encoder.frame_info({static_cast<uint32_t>(params->width), static_cast<uint32_t>(params->height), params->bitsPerSample, params->components});
    encoder.near_lossless(params->allowedLossyError);
    encoder.interleave_mode(params->interleaveMode);
    encoder.color_transformation(params->colorTransformation);
    const auto& pc = params->custom;
    encoder.preset_coding_parameters({pc.MaximumSampleValue, pc.Threshold1, pc.Threshold2, pc.Threshold3, pc.ResetValue});

    encoder.encode(source, sourceLength, params->stride);
    *bytesWritten = encoder.bytes_written();

    clear_error_message(errorMessage);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), errorMessage);
}
}
