// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <charls/charls.h>

#include "jpeg_stream_reader.h"
#include "util.h"

#include <cassert>
#include <memory>
#include <new>

using std::unique_ptr;
using namespace charls;

struct charls_jpegls_decoder final
{
    void source(const void* source_buffer, size_t source_size_bytes)
    {
        if (state_ != state::initial)
            throw jpegls_error{jpegls_errc::invalid_operation};

        source_buffer_ = source_buffer;
        size_ = source_size_bytes;

        ByteStreamInfo source{FromByteArrayConst(source_buffer_, size_)};
        reader_ = std::make_unique<JpegStreamReader>(source);
        state_ = state::source_set;
    }

    bool read_header(spiff_header* spiff_header)
    {
        if (state_ != state::source_set)
            throw jpegls_error{jpegls_errc::invalid_operation};

        bool spiff_header_found{};
        reader_->ReadHeader(spiff_header, &spiff_header_found);
        state_ = spiff_header_found ? state::spiff_header_read : state::spiff_header_not_found;

        return spiff_header_found;
    }

    void read_header()
    {
        if (state_ == state::initial || state_ >= state::header_read)
            throw jpegls_error{jpegls_errc::invalid_operation};

        if (state_ != state::spiff_header_not_found)
        {
            reader_->ReadHeader();
        }

        reader_->ReadStartOfScan(true);
        state_ = state::header_read;
    }

    charls::frame_info frame_info() const
    {
        if (state_ < state::header_read)
            throw jpegls_error{jpegls_errc::invalid_operation};

        const auto& metadata = reader_->GetMetadata();
        return {static_cast<uint32_t>(metadata.width), static_cast<uint32_t>(metadata.height), metadata.bitsPerSample, metadata.components};
    }

    int32_t near_lossless(int32_t /*component*/) const
    {
        if (state_ < state::header_read)
            throw jpegls_error{jpegls_errc::invalid_operation};

        // Note: The JPEG-LS standard allows to define different NEAR parameter for every scan.
        const auto& metadata = reader_->GetMetadata();
        return metadata.allowedLossyError;
    }

    charls::interleave_mode interleave_mode() const
    {
        if (state_ < state::header_read)
            throw jpegls_error{jpegls_errc::invalid_operation};

        // Note: The JPEG-LS standard allows to define different interleave modes for every scan.
        //       CharLS doesn't support mixed interleave modes, first scan determines the mode.
        const auto& metadata = reader_->GetMetadata();
        return metadata.interleaveMode;
    }

    const jpegls_pc_parameters& preset_coding_parameters() const
    {
        if (state_ < state::header_read)
            throw jpegls_error{jpegls_errc::invalid_operation};

        return reader_->GetCustomPreset();
    }

    size_t destination_size(const uint32_t stride) const
    {
        const charls::frame_info info{frame_info()};

        if (stride == 0)
        {
            return static_cast<size_t>(info.width) * info.height * info.component_count * (info.bits_per_sample <= 8 ? 1 : 2);
        }

        switch (interleave_mode())
        {
        case interleave_mode::none:
            return static_cast<size_t>(stride) * info.height * info.component_count;

        case interleave_mode::line:
        case interleave_mode::sample:
            return static_cast<size_t>(stride) * info.height;
        }

        ASSERT(false);
        return 0;
    }

    void decode(void* destination_buffer, size_t destination_size_bytes, uint32_t stride) const
    {
        if (state_ != state::header_read)
            throw jpegls_error{jpegls_errc::invalid_operation};

        if (stride != 0)
        {
            reader_->GetMetadata().stride = static_cast<int32_t>(stride);
        }

        const ByteStreamInfo destination = FromByteArray(destination_buffer, destination_size_bytes);
        reader_->Read(destination);
    }

    void output_bgr(char value) const noexcept
    {
        reader_->SetOutputBgr(value);
    }

    const JlsParameters& metadata() const noexcept
    {
        return reader_->GetMetadata();
    }

    void region(const JlsRect& rect) const noexcept
    {
        reader_->SetRect(rect);
    }

private:
    enum class state
    {
        initial,
        source_set,
        spiff_header_read,
        spiff_header_not_found,
        header_read,
        completed,
    };

    state state_{};
    unique_ptr<JpegStreamReader> reader_;
    const void* source_buffer_{};
    size_t size_{};
};


extern "C" {

#if defined _MSC_VER && _MSC_VER < 1917
#pragma warning(disable : 26447) // Function is declared 'noexcept' but calls function '' which may throw exceptions (f.6). [false warning in VS 2017]
#endif

charls_jpegls_decoder* CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_create() noexcept
{
    MSVC_WARNING_SUPPRESS(26402 26409) // don't use new and delete + scoped object and move
    return new (std::nothrow) charls_jpegls_decoder;
    MSVC_WARNING_UNSUPPRESS()
}

void CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_destroy(const charls_jpegls_decoder* decoder) noexcept
{
    MSVC_WARNING_SUPPRESS(26401 26409) // don't use new and delete + non-owner.
    delete decoder;
    MSVC_WARNING_UNSUPPRESS()
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_set_source_buffer(charls_jpegls_decoder* decoder, const void* source_buffer, size_t source_size_bytes) noexcept
try
{
    if (!decoder || !source_buffer)
        return jpegls_errc::invalid_argument;

    decoder->source(source_buffer, source_size_bytes);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_spiff_header(charls_jpegls_decoder* const decoder, charls_spiff_header* spiff_header, int32_t* header_found) noexcept
try
{
    if (!decoder || !spiff_header || !header_found)
        return jpegls_errc::invalid_argument;

    *header_found = decoder->read_header(spiff_header);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_header(charls_jpegls_decoder* const decoder) noexcept
try
{
    if (!decoder)
        return jpegls_errc::invalid_argument;

    decoder->read_header();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_frame_info(const charls_jpegls_decoder* const decoder, charls_frame_info* frame_info) noexcept
try
{
    if (!decoder || !frame_info)
        return jpegls_errc::invalid_argument;

    *frame_info = decoder->frame_info();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_near_lossless(const charls_jpegls_decoder* decoder, int32_t component, int32_t* near_lossless) noexcept
try
{
    if (!decoder || !near_lossless)
        return jpegls_errc::invalid_argument;

    *near_lossless = decoder->near_lossless(component);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_interleave_mode(const charls_jpegls_decoder* decoder, charls_interleave_mode* interleave_mode) noexcept
try
{
    if (!decoder || !interleave_mode)
        return jpegls_errc::invalid_argument;

    *interleave_mode = decoder->interleave_mode();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_preset_coding_parameters(const charls_jpegls_decoder* decoder, int32_t /*reserved*/, charls_jpegls_pc_parameters* preset_coding_parameters) noexcept
try
{
    if (!decoder || !preset_coding_parameters)
        return jpegls_errc::invalid_argument;

    *preset_coding_parameters = decoder->preset_coding_parameters();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_destination_size(const struct charls_jpegls_decoder* decoder, const uint32_t stride, size_t* destination_size_bytes) noexcept
try
{
    if (!decoder || !destination_size_bytes)
        return jpegls_errc::invalid_argument;

    *destination_size_bytes = decoder->destination_size(stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_decode_to_buffer(const charls_jpegls_decoder* decoder, void* destination_buffer, size_t destination_size_bytes, uint32_t stride) noexcept
try
{
    if (!decoder || !destination_buffer)
        return jpegls_errc::invalid_argument;

    decoder->decode(destination_buffer, destination_size_bytes, stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsReadHeader(const void* source, size_t sourceLength, JlsParameters* params, char* errorMessage)
try
{
    if (!source || !params)
        return jpegls_errc::invalid_argument;

    charls_jpegls_decoder decoder;

    decoder.source(source, sourceLength);
    decoder.read_header();
    *params = decoder.metadata();

    const auto& preset{decoder.preset_coding_parameters()};
    params->custom.MaximumSampleValue = preset.maximum_sample_value;
    params->custom.Threshold1 = preset.threshold1;
    params->custom.Threshold2 = preset.threshold2;
    params->custom.Threshold3 = preset.threshold3;
    params->custom.ResetValue = preset.reset_value;

    clear_error_message(errorMessage);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), errorMessage);
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsDecode(void* destination, size_t destinationLength, const void* source, size_t sourceLength,
             const struct JlsParameters* params, char* errorMessage)
try
{
    if (!destination || !source)
        return jpegls_errc::invalid_argument;

    charls_jpegls_decoder decoder;
    decoder.source(source, sourceLength);
    decoder.read_header();

    int32_t stride{};
    if (params)
    {
        decoder.output_bgr(params->outputBgr);
        stride = params->stride;
    }

    decoder.decode(destination, destinationLength, static_cast<uint32_t>(stride));

    clear_error_message(errorMessage);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), errorMessage);
}

jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsDecodeRect(void* destination, size_t destinationLength, const void* source, size_t sourceLength,
                 JlsRect roi, const JlsParameters* params, char* errorMessage)
try
{
    if (!destination || !source)
        return jpegls_errc::invalid_argument;

    charls_jpegls_decoder decoder;
    decoder.source(source, sourceLength);
    decoder.read_header();

    int32_t stride{};
    if (params)
    {
        decoder.output_bgr(params->outputBgr);
        stride = params->stride;
    }

    decoder.region(roi);
    decoder.decode(destination, destinationLength, static_cast<uint32_t>(stride));

    clear_error_message(errorMessage);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), errorMessage);
}
}
