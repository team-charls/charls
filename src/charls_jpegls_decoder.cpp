// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "charls/charls_jpegls_decoder.h"
#include "jpeg_stream_reader.h"
#include "util.h"

#include <memory>
#include <new>

using namespace charls;

struct charls_jpegls_decoder final
{
    void source(const const_byte_span source)
    {
        check_argument(source.data() || source.empty());
        check_operation(state_ == state::initial);

        reader_.source({source.data(), source.size()});
        state_ = state::source_set;
    }

    bool read_header(CHARLS_OUT spiff_header* spiff_header)
    {
        check_operation(state_ == state::source_set);

        bool spiff_header_found{};
        reader_.read_header(spiff_header, &spiff_header_found);
        state_ = spiff_header_found ? state::spiff_header_read : state::spiff_header_not_found;

        return spiff_header_found;
    }

    void read_header()
    {
        check_operation(state_ >= state::source_set && state_ < state::header_read);

        if (state_ != state::spiff_header_not_found)
        {
            reader_.read_header();
        }

        state_ = state::header_read;
    }

    [[nodiscard]] charls::frame_info frame_info() const
    {
        check_operation(state_ >= state::header_read);
        return reader_.frame_info();
    }

    [[nodiscard]] int32_t near_lossless(int32_t /*component*/ = 0) const
    {
        check_operation(state_ >= state::header_read);

        // Note: The JPEG-LS standard allows to define different NEAR parameter for every scan.
        return reader_.parameters().near_lossless;
    }

    [[nodiscard]] charls::interleave_mode interleave_mode() const
    {
        check_operation(state_ >= state::header_read);

        // Note: The JPEG-LS standard allows to define different interleave modes for every scan.
        //       CharLS doesn't support mixed interleave modes, first scan determines the mode.
        return reader_.parameters().interleave_mode;
    }

    [[nodiscard]] charls::color_transformation color_transformation() const
    {
        check_operation(state_ >= state::header_read);
        return reader_.parameters().transformation;
    }

    [[nodiscard]] const jpegls_pc_parameters& preset_coding_parameters() const
    {
        check_operation(state_ >= state::header_read);
        return reader_.preset_coding_parameters();
    }

    [[nodiscard]] size_t destination_size(const size_t stride) const
    {
        const auto [width, height, bits_per_sample, component_count]{frame_info()};

        if (stride == auto_calculate_stride)
        {
            return checked_mul(checked_mul(checked_mul(component_count, height), width),
                               bit_to_byte_count(bits_per_sample));
        }

        switch (interleave_mode())
        {
        case charls::interleave_mode::none: {
            const size_t minimum_stride{static_cast<size_t>(width) * bit_to_byte_count(bits_per_sample)};
            check_argument(stride >= minimum_stride, jpegls_errc::invalid_argument_stride);
            return checked_mul(checked_mul(stride, component_count), height) - (stride - minimum_stride);
        }

        case charls::interleave_mode::line:
        case charls::interleave_mode::sample: {
            const size_t minimum_stride{static_cast<size_t>(width) * component_count *
                                        bit_to_byte_count(bits_per_sample)};
            check_argument(stride >= minimum_stride, jpegls_errc::invalid_argument_stride);
            return checked_mul(stride, height) - (stride - minimum_stride);
        }
        }

        ASSERT(false);
        return 0;
    }

    void at_comment(const callback_function<at_comment_handler> at_comment_callback) noexcept
    {
        reader_.at_comment(at_comment_callback);
    }

    void at_application_data(const callback_function<at_application_data_handler> at_application_data_callback) noexcept
    {
        reader_.at_application_data(at_application_data_callback);
    }

    void decode(const byte_span destination, const size_t stride)
    {
        check_argument(destination.data || destination.size == 0);
        check_operation(state_ == state::header_read);

        reader_.decode(destination, stride);
        reader_.read_end_of_image();

        state_ = state::completed;
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
    jpeg_stream_reader reader_;
};


extern "C" {

USE_DECL_ANNOTATIONS charls_jpegls_decoder* CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_create() noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26402 26409)     // don't use new and delete + scoped object and move
    return new (std::nothrow) charls_jpegls_decoder; // NOLINT(cppcoreguidelines-owning-memory)
}


USE_DECL_ANNOTATIONS void CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_destroy(const charls_jpegls_decoder* decoder) noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26401 26409) // don't use new and delete + non-owner.
    delete decoder;                              // NOLINT(cppcoreguidelines-owning-memory)
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_set_source_buffer(
    charls_jpegls_decoder* decoder, const void* source_buffer, const size_t source_size_bytes) noexcept
try
{
    check_pointer(decoder)->source({source_buffer, source_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_read_spiff_header(
    charls_jpegls_decoder* const decoder, charls_spiff_header* spiff_header, int32_t* header_found) noexcept
try
{
    *check_pointer(header_found) = static_cast<int32_t>(check_pointer(decoder)->read_header(check_pointer(spiff_header)));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_header(charls_jpegls_decoder* const decoder) noexcept
try
{
    check_pointer(decoder)->read_header();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_frame_info(const charls_jpegls_decoder* const decoder, charls_frame_info* frame_info) noexcept
try
{
    *check_pointer(frame_info) = check_pointer(decoder)->frame_info();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_near_lossless(
    const charls_jpegls_decoder* decoder, const int32_t component, int32_t* near_lossless) noexcept
try
{
    *check_pointer(near_lossless) = check_pointer(decoder)->near_lossless(component);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_interleave_mode(
    const charls_jpegls_decoder* decoder, charls_interleave_mode* interleave_mode) noexcept
try
{
    *check_pointer(interleave_mode) = check_pointer(decoder)->interleave_mode();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_preset_coding_parameters(const charls_jpegls_decoder* decoder, const int32_t /*reserved*/,
                                                   charls_jpegls_pc_parameters* preset_coding_parameters) noexcept
try
{
    *check_pointer(preset_coding_parameters) = check_pointer(decoder)->preset_coding_parameters();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS charls_jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_color_transformation(
    const charls_jpegls_decoder* decoder, charls_color_transformation* color_transformation) noexcept
try
{
    *check_pointer(color_transformation) = check_pointer(decoder)->color_transformation();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_destination_size(
    const charls_jpegls_decoder* decoder, const uint32_t stride, size_t* destination_size_bytes) noexcept
try
{
    *check_pointer(destination_size_bytes) = check_pointer(decoder)->destination_size(stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_decode_to_buffer(charls_jpegls_decoder* decoder, void* destination_buffer,
                                       const size_t destination_size_bytes, const uint32_t stride) noexcept
try
{
    check_pointer(decoder)->decode({destination_buffer, destination_size_bytes}, stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_at_comment(
    charls_jpegls_decoder* decoder, const charls_at_comment_handler handler, void* user_context) noexcept
try
{
    check_pointer(decoder)->at_comment({handler, user_context});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_at_application_data(
    charls_jpegls_decoder* decoder, const charls_at_application_data_handler handler, void* user_context) noexcept
try
{
    check_pointer(decoder)->at_application_data({handler, user_context});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

} // extern "C"
