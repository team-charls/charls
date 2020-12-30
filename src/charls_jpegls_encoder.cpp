// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <charls/charls_jpegls_encoder.h>

#include "encoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_stream_writer.h"
#include "jpegls_preset_coding_parameters.h"
#include "util.h"

#include <new>

using namespace charls;
using impl::throw_jpegls_error;
using std::unique_ptr;

struct charls_jpegls_encoder final
{
    void destination(const byte_span destination)
    {
        if (state_ != state::initial)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        writer_.destination(destination);
        state_ = state::destination_set;
    }

    void frame_info(const charls_frame_info& frame_info)
    {
        if (frame_info.width < 1 || frame_info.width > maximum_width)
            throw_jpegls_error(jpegls_errc::invalid_argument_width);

        if (frame_info.height < 1 || frame_info.height > maximum_height)
            throw_jpegls_error(jpegls_errc::invalid_argument_height);

        if (frame_info.bits_per_sample < minimum_bits_per_sample || frame_info.bits_per_sample > maximum_bits_per_sample)
            throw_jpegls_error(jpegls_errc::invalid_argument_bits_per_sample);

        if (frame_info.component_count < 1 || frame_info.component_count > maximum_component_count)
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
        if (color_transformation < charls::color_transformation::none ||
            color_transformation > charls::color_transformation::hp3)
            throw_jpegls_error(jpegls_errc::invalid_argument_color_transformation);

        color_transformation_ = color_transformation;
    }

    size_t estimated_destination_size() const
    {
        if (!is_frame_info_configured())
            throw_jpegls_error(jpegls_errc::invalid_operation);

        return static_cast<size_t>(frame_info_.component_count) * frame_info_.width * frame_info_.height *
                   bit_to_byte_count(frame_info_.bits_per_sample) +
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

        writer_.write_start_of_image();
        writer_.write_spiff_header_segment(spiff_header);
        state_ = state::spiff_header;
    }

    void write_standard_spiff_header(const spiff_color_space color_space, const spiff_resolution_units resolution_units,
                                     const uint32_t vertical_resolution, const uint32_t horizontal_resolution)
    {
        if (!is_frame_info_configured())
            throw_jpegls_error(jpegls_errc::invalid_operation);

        write_spiff_header({spiff_profile_id::none, frame_info_.component_count, frame_info_.height, frame_info_.width,
                            color_space, frame_info_.bits_per_sample, spiff_compression_type::jpeg_ls, resolution_units,
                            vertical_resolution, horizontal_resolution});
    }

    void write_spiff_entry(const uint32_t entry_tag, IN_READS_BYTES_(entry_data_size_bytes) const void* entry_data,
                           const size_t entry_data_size_bytes)
    {
        if (entry_tag == spiff_end_of_directory_entry_type)
            throw_jpegls_error(jpegls_errc::invalid_argument);

        if (entry_data_size_bytes > 65528)
            throw_jpegls_error(jpegls_errc::invalid_argument_spiff_entry_size);

        if (state_ != state::spiff_header)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        writer_.write_spiff_directory_entry(entry_tag, entry_data, entry_data_size_bytes);
    }

    void encode(byte_span source, size_t stride)
    {
        if (!is_frame_info_configured() || state_ == state::initial)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        if (stride == 0)
        {
            stride = static_cast<size_t>(frame_info_.width) * bit_to_byte_count(frame_info_.bits_per_sample);
            if (interleave_mode_ != charls::interleave_mode::none)
            {
                stride *= static_cast<size_t>(frame_info_.component_count);
            }
        }

        if (state_ == state::spiff_header)
        {
            writer_.write_spiff_end_of_directory_entry();
        }
        else
        {
            writer_.write_start_of_image();
        }

        writer_.write_start_of_frame_segment(frame_info_.width, frame_info_.height, frame_info_.bits_per_sample,
                                             frame_info_.component_count);

        if (color_transformation_ != charls::color_transformation::none)
        {
            if (!(frame_info_.bits_per_sample == 8 || frame_info_.bits_per_sample == 16))
                throw_jpegls_error(jpegls_errc::bit_depth_for_transform_not_supported);

            writer_.write_color_transform_segment(color_transformation_);
        }

        if (!is_default(preset_coding_parameters_))
        {
            writer_.write_jpegls_preset_parameters_segment(preset_coding_parameters_);
        }
        else if (frame_info_.bits_per_sample > 12)
        {
            const jpegls_pc_parameters preset{compute_default(
                static_cast<int32_t>(calculate_maximum_sample_value(frame_info_.bits_per_sample)), near_lossless_)};
            writer_.write_jpegls_preset_parameters_segment(preset);
        }

        if (interleave_mode_ == charls::interleave_mode::none)
        {
            const size_t byte_count_component{stride * frame_info_.height};
            for (int32_t component{}; component < frame_info_.component_count; ++component)
            {
                writer_.write_start_of_scan_segment(1, near_lossless_, interleave_mode_);
                encode_scan(source, stride, 1);

                // Synchronize the source stream (encode_scan works on a local copy)
                skip_bytes(source, byte_count_component);
            }
        }
        else
        {
            writer_.write_start_of_scan_segment(frame_info_.component_count, near_lossless_, interleave_mode_);
            encode_scan(source, stride, frame_info_.component_count);
        }

        writer_.write_end_of_image();
    }

    size_t bytes_written() const noexcept
    {
        return writer_.bytes_written();
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

    void encode_scan(const byte_span source, const size_t stride, const int32_t component_count)
    {
        const charls::frame_info frame_info{frame_info_.width, frame_info_.height, frame_info_.bits_per_sample,
                                            component_count};

        auto codec{jls_codec_factory<encoder_strategy>().create_codec(
            frame_info, {near_lossless_, interleave_mode_, color_transformation_, false}, preset_coding_parameters_)};
        unique_ptr<process_line> process_line(codec->create_process_line(source, stride));
        const size_t bytes_written{codec->encode_scan(move(process_line), writer_.remaining_destination())};

        // Synchronize the destination encapsulated in the writer (encode_scan works on a local copy)
        writer_.seek(bytes_written);
    }

    charls_frame_info frame_info_{};
    int32_t near_lossless_{};
    charls::interleave_mode interleave_mode_{};
    charls::color_transformation color_transformation_{};
    state state_{};
    jpeg_stream_writer writer_;
    jpegls_pc_parameters preset_coding_parameters_{};
};

extern "C" {

CHARLS_CHECK_RETURN CHARLS_RET_MAY_BE_NULL charls_jpegls_encoder* CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_create() noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26402 26409)     // don't use new and delete + scoped object and move
    return new (std::nothrow) charls_jpegls_encoder; // NOLINT(cppcoreguidelines-owning-memory)
}

void CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_destroy(IN_OPT_ const charls_jpegls_encoder* encoder) noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26401 26409) // don't use new and delete + non-owner.
    delete encoder;                              // NOLINT(cppcoreguidelines-owning-memory)
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_destination_buffer(
    IN_ charls_jpegls_encoder* encoder, OUT_WRITES_BYTES_(destination_size_bytes) void* destination_buffer,
    const size_t destination_size_bytes) noexcept
try
{
    check_pointer(encoder)->destination({check_pointer(destination_buffer), destination_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_frame_info(IN_ charls_jpegls_encoder* encoder, IN_ const charls_frame_info* frame_info) noexcept
try
{
    check_pointer(encoder)->frame_info(*check_pointer(frame_info));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_near_lossless(IN_ charls_jpegls_encoder* encoder,
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

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_interleave_mode(
    IN_ charls_jpegls_encoder* encoder, const charls_interleave_mode interleave_mode) noexcept
try
{
    check_pointer(encoder)->interleave_mode(interleave_mode);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_preset_coding_parameters(
    IN_ charls_jpegls_encoder* encoder, IN_ const charls_jpegls_pc_parameters* preset_coding_parameters) noexcept
try
{
    check_pointer(encoder)->preset_coding_parameters(*check_pointer(preset_coding_parameters));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_color_transformation(
    IN_ charls_jpegls_encoder* encoder, const charls_color_transformation color_transformation) noexcept
try
{
    check_pointer(encoder)->color_transformation(color_transformation);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_get_estimated_destination_size(
    IN_ const charls_jpegls_encoder* encoder, OUT_ size_t* size_in_bytes) noexcept
try
{
    *check_pointer(size_in_bytes) = check_pointer(encoder)->estimated_destination_size();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_get_bytes_written(IN_ const charls_jpegls_encoder* encoder,
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

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_encode_from_buffer(IN_ charls_jpegls_encoder* encoder,
                                                                                   IN_READS_BYTES_(source_size_bytes)
                                                                                       const void* source_buffer,
                                                                                   const size_t source_size_bytes,
                                                                                   const uint32_t stride) noexcept
try
{
    check_pointer(encoder)->encode({check_pointer(source_buffer), source_size_bytes}, stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_write_spiff_header(
    IN_ charls_jpegls_encoder* encoder, IN_ const charls_spiff_header* spiff_header) noexcept
try
{
    check_pointer(encoder)->write_spiff_header(*check_pointer(spiff_header));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_write_standard_spiff_header(
    IN_ charls_jpegls_encoder* encoder, const charls_spiff_color_space color_space,
    const charls_spiff_resolution_units resolution_units, const uint32_t vertical_resolution,
    const uint32_t horizontal_resolution) noexcept
try
{
    check_pointer(encoder)->write_standard_spiff_header(color_space, resolution_units, vertical_resolution,
                                                        horizontal_resolution);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_write_spiff_entry(
    IN_ charls_jpegls_encoder* encoder, const uint32_t entry_tag,
    IN_READS_BYTES_(entry_data_size_bytes) const void* entry_data, const size_t entry_data_size_bytes) noexcept
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


jpegls_errc CHARLS_API_CALLING_CONVENTION JpegLsEncode(OUT_WRITES_BYTES_(destination_length) void* destination,
                                                       const size_t destination_length, OUT_ size_t* bytes_written,
                                                       IN_READS_BYTES_(source_length) const void* source,
                                                       const size_t source_length, IN_ const struct JlsParameters* params,
                                                       OUT_OPT_ char* error_message)
try
{
    if (check_pointer(params)->jfif.version != 0)
        return jpegls_errc::invalid_argument;

    charls_jpegls_encoder encoder;
    encoder.destination({check_pointer(destination), destination_length});
    encoder.near_lossless(params->allowedLossyError);

    encoder.frame_info({static_cast<uint32_t>(params->width), static_cast<uint32_t>(params->height), params->bitsPerSample,
                        params->components});
    encoder.interleave_mode(params->interleaveMode);
    encoder.color_transformation(params->colorTransformation);
    const auto& pc{params->custom};
    encoder.preset_coding_parameters({pc.MaximumSampleValue, pc.Threshold1, pc.Threshold2, pc.Threshold3, pc.ResetValue});

    encoder.encode({check_pointer(source), source_length}, static_cast<uint32_t>(params->stride));
    *check_pointer(bytes_written) = encoder.bytes_written();

    clear_error_message(error_message);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), error_message);
}
}
