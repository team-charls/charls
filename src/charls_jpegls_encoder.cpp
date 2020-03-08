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
using impl::throw_jpegls_error;
using std::unique_ptr;

struct charls_jpegls_encoder final
{
    charls_jpegls_encoder() = default;

    void destination(OUT_WRITES_BYTES_(size) void* destination, const size_t size)
    {
        if (state_ != state::initial)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        writer_.UpdateDestination(destination, size);
        state_ = state::destination_set;
    }

    void frame_info(const charls_frame_info& frame_info)
    {
        if (frame_info.width < 1 || frame_info.width > maximum_width)
            throw_jpegls_error(jpegls_errc::invalid_argument_width);

        if (frame_info.height < 1 || frame_info.height > maximum_height)
            throw_jpegls_error(jpegls_errc::invalid_argument_height);

        if (frame_info.bits_per_sample < MinimumBitsPerSample || frame_info.bits_per_sample > MaximumBitsPerSample)
            throw_jpegls_error(jpegls_errc::invalid_argument_bits_per_sample);

        if (frame_info.component_count < 1 || frame_info.component_count > MaximumComponentCount)
            throw_jpegls_error(jpegls_errc::invalid_argument_component_count);

        frame_info_ = frame_info;
    }

    void interleave_mode(const charls::interleave_mode interleave_mode)
    {
        if (interleave_mode < charls::interleave_mode::none || interleave_mode > charls::interleave_mode::sample)
            throw_jpegls_error(jpegls_errc::invalid_argument_interleave_mode);

        interleave_mode_ = interleave_mode;
    }

    void near_lossless(const int32_t near_lossless)
    {
        if (near_lossless < 0 || near_lossless > maximum_near_lossless)
            throw_jpegls_error(jpegls_errc::invalid_argument_near_lossless);

        near_lossless_ = near_lossless;
    }

    void preset_coding_parameters(const jpegls_pc_parameters& preset_coding_parameters)
    {
        if (!is_valid(preset_coding_parameters, UINT16_MAX, near_lossless_))
            throw_jpegls_error(jpegls_errc::invalid_argument_jpegls_pc_parameters);

        preset_coding_parameters_ = preset_coding_parameters;
    }

    void color_transformation(const charls::color_transformation color_transformation)
    {
        if (color_transformation < charls::color_transformation::none || color_transformation > charls::color_transformation::hp3)
            throw_jpegls_error(jpegls_errc::invalid_argument_color_transformation);

        color_transformation_ = color_transformation;
    }

    size_t estimated_destination_size() const
    {
        if (!is_frame_info_configured())
            throw_jpegls_error(jpegls_errc::invalid_operation);

        return static_cast<size_t>(frame_info_.width) * frame_info_.height *
                   frame_info_.component_count * (frame_info_.bits_per_sample < 9 ? 1 : 2) +
               1024 + spiff_header_size_in_bytes;
    }

    void write_spiff_header(const spiff_header& spiff_header)
    {
        if (spiff_header.height == 0)
            throw_jpegls_error(jpegls_errc::invalid_argument_height);

        if (spiff_header.width == 0)
            throw_jpegls_error(jpegls_errc::invalid_argument_width);

        if (state_ != state::destination_set)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        writer_.WriteStartOfImage();
        writer_.WriteSpiffHeaderSegment(spiff_header);
        state_ = state::spiff_header;
    }

    void write_standard_spiff_header(const spiff_color_space color_space,
                                     const spiff_resolution_units resolution_units,
                                     const uint32_t vertical_resolution,
                                     const uint32_t horizontal_resolution)
    {
        if (!is_frame_info_configured())
            throw_jpegls_error(jpegls_errc::invalid_operation);

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

    void write_spiff_entry(const uint32_t entry_tag,
                           IN_READS_BYTES_(entry_data_size_bytes) const void* entry_data,
                           const size_t entry_data_size_bytes)
    {
        if (entry_tag == spiff_end_of_directory_entry_type)
            throw_jpegls_error(jpegls_errc::invalid_argument);

        if (entry_data_size_bytes > 65528)
            throw_jpegls_error(jpegls_errc::invalid_argument_spiff_entry_size);

        if (state_ != state::spiff_header)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        writer_.WriteSpiffDirectoryEntry(entry_tag, entry_data, entry_data_size_bytes);
    }

    void encode(IN_READS_BYTES_(source_size_bytes) const void* source,
                const size_t source_size_bytes,
                uint32_t stride)
    {
        if (!is_frame_info_configured() || state_ == state::initial)
            throw_jpegls_error(jpegls_errc::invalid_operation);

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
            const jpegls_pc_parameters preset = compute_default(calculate_maximum_sample_value(frame_info_.bits_per_sample), near_lossless_);
            writer_.WriteJpegLSPresetParametersSegment(preset);
        }

        ByteStreamInfo sourceInfo = FromByteArrayConst(source, source_size_bytes);
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
        completed
    };

    bool is_frame_info_configured() const noexcept
    {
        return frame_info_.width != 0;
    }

    void encode_scan(const ByteStreamInfo source, const uint32_t stride, const int32_t component_count)
    {
        const charls::frame_info frame_info{frame_info_.width, frame_info_.height, frame_info_.bits_per_sample, component_count};

        auto codec = JlsCodecFactory<EncoderStrategy>().CreateCodec(frame_info,
                                                                    {near_lossless_, interleave_mode_, color_transformation_, false},
                                                                    preset_coding_parameters_);
        unique_ptr<ProcessLine> processLine(codec->CreateProcess(source, stride));
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

charls_jpegls_encoder* CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_create() noexcept
{
    MSVC_WARNING_SUPPRESS(26402 26409)               // don't use new and delete + scoped object and move
    return new (std::nothrow) charls_jpegls_encoder; // NOLINT(cppcoreguidelines-owning-memory)
    MSVC_WARNING_UNSUPPRESS()
}

void CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_destroy(IN_OPT_ const charls_jpegls_encoder* encoder) noexcept
{
    MSVC_WARNING_SUPPRESS(26401 26409) // don't use new and delete + non-owner.
    delete encoder;                    // NOLINT(cppcoreguidelines-owning-memory)
    MSVC_WARNING_UNSUPPRESS()
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_destination_buffer(IN_ charls_jpegls_encoder* encoder,
                                             OUT_WRITES_BYTES_(destination_size_bytes) void* destination_buffer,
                                             const size_t destination_size_bytes) noexcept
try
{
    check_pointer(encoder)->destination(check_pointer(destination_buffer), destination_size_bytes);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_frame_info(IN_ charls_jpegls_encoder* encoder,
                                     IN_ const charls_frame_info* frame_info) noexcept
try
{
    check_pointer(encoder)->frame_info(*check_pointer(frame_info));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_near_lossless(IN_ charls_jpegls_encoder* encoder,
                                        const int32_t near_lossless) noexcept
try
{
    check_pointer(encoder)->near_lossless(near_lossless);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_interleave_mode(IN_ charls_jpegls_encoder* encoder,
                                          const charls_interleave_mode interleave_mode) noexcept
try
{
    check_pointer(encoder)->interleave_mode(interleave_mode);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_preset_coding_parameters(IN_ charls_jpegls_encoder* encoder,
                                                   IN_ const charls_jpegls_pc_parameters* preset_coding_parameters) noexcept
try
{
    check_pointer(encoder)->preset_coding_parameters(*check_pointer(preset_coding_parameters));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_color_transformation(IN_ charls_jpegls_encoder* encoder,
                                               const charls_color_transformation color_transformation) noexcept
try
{
    check_pointer(encoder)->color_transformation(color_transformation);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_estimated_destination_size(IN_ const charls_jpegls_encoder* encoder,
                                                     OUT_ size_t* size_in_bytes) noexcept
try
{
    *check_pointer(size_in_bytes) = check_pointer(encoder)->estimated_destination_size();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_bytes_written(IN_ const charls_jpegls_encoder* encoder,
                                        OUT_ size_t* bytes_written) noexcept
try
{
    *check_pointer(bytes_written) = check_pointer(encoder)->bytes_written();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_encode_from_buffer(IN_ charls_jpegls_encoder* encoder,
                                         IN_READS_BYTES_(source_size_bytes) const void* source_buffer,
                                         const size_t source_size_bytes,
                                         const uint32_t stride) noexcept
try
{
    check_pointer(encoder)->encode(check_pointer(source_buffer), source_size_bytes, stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_header(IN_ charls_jpegls_encoder* encoder,
                                         IN_ const charls_spiff_header* spiff_header) noexcept
try
{
    check_pointer(encoder)->write_spiff_header(*check_pointer(spiff_header));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_standard_spiff_header(IN_ charls_jpegls_encoder* encoder,
                                                  const charls_spiff_color_space color_space,
                                                  const charls_spiff_resolution_units resolution_units,
                                                  const uint32_t vertical_resolution,
                                                  const uint32_t horizontal_resolution) noexcept
try
{
    check_pointer(encoder)->write_standard_spiff_header(color_space, resolution_units, vertical_resolution, horizontal_resolution);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_entry(IN_ charls_jpegls_encoder* encoder,
                                        const uint32_t entry_tag,
                                        IN_READS_BYTES_(entry_data_size_bytes) const void* entry_data,
                                        const size_t entry_data_size_bytes) noexcept
try
{
    if (!entry_data && entry_data_size_bytes != 0)
        return jpegls_errc::invalid_argument;

    check_pointer(encoder)->write_spiff_entry(entry_tag, entry_data, entry_data_size_bytes);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsEncode(OUT_WRITES_BYTES_(destinationLength) void* destination,
             const size_t destinationLength,
             OUT_ size_t* bytesWritten,
             IN_READS_BYTES_(sourceLength) const void* source,
             const size_t sourceLength,
             IN_ const struct JlsParameters* params,
             OUT_OPT_ char* errorMessage)
try
{
    if (check_pointer(params)->jfif.version != 0)
        return jpegls_errc::invalid_argument;

    charls_jpegls_encoder encoder;
    encoder.destination(check_pointer(destination), destinationLength);
    encoder.near_lossless(params->allowedLossyError);

    encoder.frame_info({static_cast<uint32_t>(params->width), static_cast<uint32_t>(params->height), params->bitsPerSample, params->components});
    encoder.interleave_mode(params->interleaveMode);
    encoder.color_transformation(params->colorTransformation);
    const auto& pc = params->custom;
    encoder.preset_coding_parameters({pc.MaximumSampleValue, pc.Threshold1, pc.Threshold2, pc.Threshold3, pc.ResetValue});

    encoder.encode(check_pointer(source), sourceLength, params->stride);
    *check_pointer(bytesWritten) = encoder.bytes_written();

    clear_error_message(errorMessage);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), errorMessage);
}
}
