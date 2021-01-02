// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <charls/charls_jpegls_decoder.h>

#include "jpeg_stream_reader.h"
#include "util.h"

#include <memory>
#include <new>

using std::unique_ptr;
using namespace charls;
using impl::throw_jpegls_error;

struct charls_jpegls_decoder final
{
    void source(IN_READS_BYTES_(source_size_bytes) const void* source_buffer, const size_t source_size_bytes)
        CHARLS_ATTRIBUTE((nonnull))
    {
        if (state_ != state::initial)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        source_buffer_ = source_buffer;
        size_ = source_size_bytes;

        reader_ = std::make_unique<jpeg_stream_reader>(byte_span{source_buffer, source_size_bytes});
        state_ = state::source_set;
    }

    bool read_header(OUT_ spiff_header* spiff_header)
    {
        if (state_ != state::source_set)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        bool spiff_header_found{};
        reader_->read_header(spiff_header, &spiff_header_found);
        state_ = spiff_header_found ? state::spiff_header_read : state::spiff_header_not_found;

        return spiff_header_found;
    }

    void read_header()
    {
        if (state_ == state::initial || state_ >= state::header_read)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        if (state_ != state::spiff_header_not_found)
        {
            reader_->read_header();
        }

        reader_->read_start_of_scan();
        state_ = state::header_read;
    }

    charls::frame_info frame_info() const
    {
        if (state_ < state::header_read)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        return reader_->frame_info();
    }

    int32_t near_lossless(int32_t /*component*/ = 0) const
    {
        if (state_ < state::header_read)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        // Note: The JPEG-LS standard allows to define different NEAR parameter for every scan.
        return reader_->parameters().near_lossless;
    }

    charls::interleave_mode interleave_mode() const
    {
        if (state_ < state::header_read)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        // Note: The JPEG-LS standard allows to define different interleave modes for every scan.
        //       CharLS doesn't support mixed interleave modes, first scan determines the mode.
        return reader_->parameters().interleave_mode;
    }

    charls::color_transformation color_transformation() const
    {
        if (state_ < state::header_read)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        return reader_->parameters().transformation;
    }

    const jpegls_pc_parameters& preset_coding_parameters() const
    {
        if (state_ < state::header_read)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        return reader_->preset_coding_parameters();
    }

    size_t destination_size(const size_t stride) const
    {
        const charls::frame_info info{frame_info()};

        if (stride == 0)
        {
            return static_cast<size_t>(info.component_count) * info.height * info.width *
                   bit_to_byte_count(info.bits_per_sample);
        }

        switch (interleave_mode())
        {
        case charls::interleave_mode::none:
            return stride * info.component_count * info.height;

        case charls::interleave_mode::line:
        case charls::interleave_mode::sample:
            return stride * info.height;
        }

        ASSERT(false);
        return 0;
    }

    void decode(OUT_WRITES_BYTES_(destination_size_bytes) void* destination_buffer, const size_t destination_size_bytes,
                const size_t stride) const CHARLS_ATTRIBUTE((nonnull))
    {
        if (state_ != state::header_read)
            throw_jpegls_error(jpegls_errc::invalid_operation);

        reader_->read({destination_buffer, destination_size_bytes}, stride);
    }

    void output_bgr(const bool value) const noexcept
    {
        reader_->output_bgr(value);
    }

    void region(const JlsRect& rect) const noexcept
    {
        reader_->rect(rect);
    }

private:
    enum class state
    {
        initial,
        source_set,
        spiff_header_read,
        spiff_header_not_found,
        header_read,
        completed
    };

    state state_{};
    unique_ptr<jpeg_stream_reader> reader_;
    const void* source_buffer_{};
    size_t size_{};
};


extern "C" {

CHARLS_CHECK_RETURN CHARLS_RET_MAY_BE_NULL charls_jpegls_decoder* CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_create() noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26402 26409)     // don't use new and delete + scoped object and move
    return new (std::nothrow) charls_jpegls_decoder; // NOLINT(cppcoreguidelines-owning-memory)
}

void CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_destroy(IN_OPT_ const charls_jpegls_decoder* decoder) noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26401 26409) // don't use new and delete + non-owner.
    delete decoder;                              // NOLINT(cppcoreguidelines-owning-memory)
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_set_source_buffer(IN_ charls_jpegls_decoder* decoder,
                                                                                  IN_READS_BYTES_(source_size_bytes)
                                                                                      const void* source_buffer,
                                                                                  const size_t source_size_bytes) noexcept
try
{
    check_pointer(decoder)->source(check_pointer(source_buffer), source_size_bytes);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_read_spiff_header(
    IN_ charls_jpegls_decoder* const decoder, OUT_ charls_spiff_header* spiff_header, OUT_ int32_t* header_found) noexcept
try
{
    *check_pointer(header_found) = static_cast<int32_t>(check_pointer(decoder)->read_header(check_pointer(spiff_header)));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_header(IN_ charls_jpegls_decoder* const decoder) noexcept
try
{
    check_pointer(decoder)->read_header();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_frame_info(
    IN_ const charls_jpegls_decoder* const decoder, OUT_ charls_frame_info* frame_info) noexcept
try
{
    *check_pointer(frame_info) = check_pointer(decoder)->frame_info();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_near_lossless(
    IN_ const charls_jpegls_decoder* decoder, const int32_t component, OUT_ int32_t* near_lossless) noexcept
try
{
    *check_pointer(near_lossless) = check_pointer(decoder)->near_lossless(component);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_interleave_mode(
    IN_ const charls_jpegls_decoder* decoder, OUT_ charls_interleave_mode* interleave_mode) noexcept
try
{
    *check_pointer(interleave_mode) = check_pointer(decoder)->interleave_mode();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_preset_coding_parameters(IN_ const charls_jpegls_decoder* decoder, const int32_t /*reserved*/,
                                                   OUT_ charls_jpegls_pc_parameters* preset_coding_parameters) noexcept
try
{
    *check_pointer(preset_coding_parameters) = check_pointer(decoder)->preset_coding_parameters();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_color_transformation(
    IN_ const charls_jpegls_decoder* decoder, OUT_ charls_color_transformation* color_transformation) noexcept
try
{
    *check_pointer(color_transformation) = check_pointer(decoder)->color_transformation();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_destination_size(
    IN_ const charls_jpegls_decoder* decoder, const uint32_t stride, OUT_ size_t* destination_size_bytes) noexcept
try
{
    *check_pointer(destination_size_bytes) = check_pointer(decoder)->destination_size(stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_decode_to_buffer(
    IN_ const charls_jpegls_decoder* decoder, OUT_WRITES_BYTES_(destination_size_bytes) void* destination_buffer,
    const size_t destination_size_bytes, const uint32_t stride) noexcept
try
{
    check_pointer(decoder)->decode(check_pointer(destination_buffer), destination_size_bytes, stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION JpegLsReadHeader(IN_READS_BYTES_(source_length) const void* source,
                                                           const size_t source_length, OUT_ JlsParameters* params,
                                                           OUT_OPT_ char* error_message)
try
{
    charls_jpegls_decoder decoder;

    decoder.source(check_pointer(source), source_length);
    decoder.read_header();
    *check_pointer(params) = JlsParameters{};
    const frame_info info{decoder.frame_info()};
    params->height = static_cast<int32_t>(info.height);
    params->width = static_cast<int32_t>(info.width);
    params->bitsPerSample = info.bits_per_sample;
    params->components = info.component_count;
    params->interleaveMode = decoder.interleave_mode();
    params->allowedLossyError = decoder.near_lossless();
    params->colorTransformation = decoder.color_transformation();
    const int32_t component_count{params->interleaveMode == interleave_mode::none ? 1 : params->components};
    params->stride = params->width * component_count * static_cast<int32_t>(bit_to_byte_count(params->bitsPerSample));

    const auto& preset{decoder.preset_coding_parameters()};
    params->custom.MaximumSampleValue = preset.maximum_sample_value;
    params->custom.Threshold1 = preset.threshold1;
    params->custom.Threshold2 = preset.threshold2;
    params->custom.Threshold3 = preset.threshold3;
    params->custom.ResetValue = preset.reset_value;

    clear_error_message(error_message);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), error_message);
}

jpegls_errc CHARLS_API_CALLING_CONVENTION JpegLsDecode(OUT_WRITES_BYTES_(destination_length) void* destination,
                                                       const size_t destination_length,
                                                       IN_READS_BYTES_(source_length) const void* source,
                                                       const size_t source_length,
                                                       IN_OPT_ const struct JlsParameters* params,
                                                       OUT_OPT_ char* error_message)
try
{
    charls_jpegls_decoder decoder;
    decoder.source(check_pointer(source), source_length);
    decoder.read_header();

    int32_t stride{};
    if (params)
    {
        decoder.output_bgr(params->outputBgr != 0);
        stride = params->stride;
    }

    decoder.decode(check_pointer(destination), destination_length, static_cast<uint32_t>(stride));

    clear_error_message(error_message);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), error_message);
}

jpegls_errc CHARLS_API_CALLING_CONVENTION JpegLsDecodeRect(OUT_WRITES_BYTES_(destination_length) void* destination,
                                                           const size_t destination_length,
                                                           IN_READS_BYTES_(source_length) const void* source,
                                                           const size_t source_length, const JlsRect roi,
                                                           IN_OPT_ const JlsParameters* params, OUT_OPT_ char* error_message)
try
{
    charls_jpegls_decoder decoder;
    decoder.source(check_pointer(source), source_length);
    decoder.read_header();

    int32_t stride{};
    if (params)
    {
        decoder.output_bgr(params->outputBgr != 0);
        stride = params->stride;
    }

    decoder.region(roi);
    decoder.decode(check_pointer(destination), destination_length, static_cast<uint32_t>(stride));

    clear_error_message(error_message);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), error_message);
}
}
