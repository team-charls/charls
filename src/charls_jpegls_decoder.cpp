// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "charls/charls_jpegls_decoder.h"

#include "constants.h"
#include "jpeg_stream_reader.h"
#include "make_scan_codec.h"
#include "scan_decoder.h"
#include "util.h"

#include <memory>
#include <new>

using namespace charls;
using impl::throw_jpegls_error;
using std::byte;

struct charls_jpegls_decoder final
{
    void source(const span<const byte> source)
    {
        check_argument(source.data() || source.empty());
        check_operation(state_ == state::initial);

        reader_.source(source);
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

        state_ = reader_.end_of_image() ? state::completed : state::header_read;
    }

    [[nodiscard]]
    charls::frame_info frame_info_checked() const
    {
        check_header_read();
        return frame_info();
    }

    [[nodiscard]]
    int32_t near_lossless(int32_t /*component*/ = 0) const
    {
        check_header_read();

        // Note: The JPEG-LS standard allows to define different NEAR parameter for every scan.
        return reader_.parameters().near_lossless;
    }

    [[nodiscard]]
    charls::interleave_mode interleave_mode() const
    {
        check_header_read();

        // Note: The JPEG-LS standard allows to define different interleave modes for every scan.
        //       CharLS doesn't support mixed interleave modes, first scan determines the mode.
        return reader_.parameters().interleave_mode;
    }

    [[nodiscard]]
    charls::color_transformation color_transformation() const
    {
        check_header_read();
        return reader_.parameters().transformation;
    }

    [[nodiscard]]
    const jpegls_pc_parameters& preset_coding_parameters() const
    {
        check_header_read();
        return reader_.preset_coding_parameters();
    }

    [[nodiscard]]
    size_t destination_size(const size_t stride) const
    {
        const auto [width, height, bits_per_sample, component_count]{frame_info_checked()};

        if (stride == auto_calculate_stride)
        {
            return checked_mul(checked_mul(checked_mul(component_count, height), width), bit_to_byte_count(bits_per_sample));
        }

        switch (interleave_mode())
        {
        case interleave_mode::none: {
            const size_t minimum_stride{static_cast<size_t>(width) * bit_to_byte_count(bits_per_sample)};
            check_argument(stride >= minimum_stride, jpegls_errc::invalid_argument_stride);
            return checked_mul(checked_mul(stride, component_count), height) - (stride - minimum_stride);
        }

        case interleave_mode::line:
        case interleave_mode::sample: {
            const size_t minimum_stride{static_cast<size_t>(width) * component_count * bit_to_byte_count(bits_per_sample)};
            check_argument(stride >= minimum_stride, jpegls_errc::invalid_argument_stride);
            return checked_mul(stride, height) - (stride - minimum_stride);
        }
        }

        unreachable();
    }

    void at_comment(const callback_function<at_comment_handler> at_comment_callback) noexcept
    {
        reader_.at_comment(at_comment_callback);
    }

    void at_application_data(const callback_function<at_application_data_handler> at_application_data_callback) noexcept
    {
        reader_.at_application_data(at_application_data_callback);
    }

    [[nodiscard]]
    int32_t mapping_table_id(const size_t component_index) const
    {
        check_state_completed();
        check_argument(component_index < reader_.component_count());
        return reader_.mapping_table_id(component_index);
    }

    [[nodiscard]]
    std::optional<int32_t> mapping_table_index(const int32_t table_id) const
    {
        check_state_completed();
        check_argument_range(minimum_table_id, maximum_table_id, table_id);
        return reader_.mapping_table_index(static_cast<uint8_t>(table_id));
    }

    [[nodiscard]]
    int32_t mapping_table_count() const
    {
        check_state_completed();
        return reader_.mapping_table_count();
    }

    [[nodiscard]]
    table_info mapping_table_info(const int32_t index) const
    {
        check_table_index(index);
        return reader_.mapping_table_info(index);
    }

    void mapping_table_data(const int32_t index, const span<byte> table) const
    {
        check_table_index(index);
        check_argument(table.data() || table.empty());

        reader_.mapping_table_data(index, table);
    }

    void decode(span<byte> destination, size_t stride)
    {
        check_argument(destination.data() || destination.empty());
        check_operation(state_ == state::header_read);
        check_parameter_coherent();

        // Compute the stride for the uncompressed destination buffer.
        const size_t minimum_stride{calculate_minimum_stride()};
        if (stride == auto_calculate_stride)
        {
            stride = minimum_stride;
        }
        else
        {
            if (UNLIKELY(stride < minimum_stride))
                throw_jpegls_error(jpegls_errc::invalid_argument_stride);
        }

        // Compute the layout of the destination buffer.
        const size_t bytes_per_plane{stride * frame_info().height};
        const size_t plane_count{reader_.parameters().interleave_mode == interleave_mode::none ? frame_info().component_count
                                                                                               : 1U};
        if (const size_t minimum_destination_size = bytes_per_plane * plane_count - (stride - minimum_stride);
            UNLIKELY(destination.size() < minimum_destination_size))
            throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

        for (size_t plane{};;)
        {
            const auto decoder{make_scan_codec<scan_decoder>(frame_info(), reader_.get_validated_preset_coding_parameters(),
                                                             reader_.parameters())};
            const size_t bytes_read{decoder->decode_scan(reader_.remaining_source(), destination.data(), stride)};
            reader_.advance_position(bytes_read);

            ++plane;
            if (plane == plane_count)
                break;

            reader_.read_next_start_of_scan();
            destination = destination.subspan(bytes_per_plane);
        }

        reader_.read_end_of_image();

        state_ = state::completed;
    }

private:
    [[nodiscard]]
    const charls::frame_info& frame_info() const noexcept
    {
        return reader_.frame_info();
    }

    [[nodiscard]]
    size_t calculate_minimum_stride() const noexcept
    {
        const size_t components_in_plane_count{reader_.parameters().interleave_mode == interleave_mode::none
                                                   ? 1U
                                                   : static_cast<size_t>(frame_info().component_count)};
        return components_in_plane_count * frame_info().width * bit_to_byte_count(frame_info().bits_per_sample);
    }

    void check_header_read() const
    {
        check_operation(state_ >= state::header_read);
    }

    void check_state_completed() const
    {
        check_operation(state_ == state::completed);
    }

    void check_table_index(const int32_t index) const
    {
        check_argument_range(0, mapping_table_count() - 1, index);
    }

    void check_parameter_coherent() const
    {
        switch (frame_info().component_count)
        {
        case 4:
        case 3:
            break;
        default:
            if (UNLIKELY(reader_.parameters().interleave_mode != interleave_mode::none))
                throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

            break;
        }
    }

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
    check_pointer(decoder)->source({static_cast<const byte*>(source_buffer), source_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_read_spiff_header(
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
    *check_pointer(frame_info) = check_pointer(decoder)->frame_info_checked();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_near_lossless(
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


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_interleave_mode(
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


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
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


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_get_color_transformation(
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
    check_pointer(decoder)->decode({static_cast<byte*>(destination_buffer), destination_size_bytes}, stride);
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


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_decoder_get_mapping_table_id(
    const charls_jpegls_decoder* decoder, const int32_t component_index, int32_t* table_id) noexcept
try
{
    *check_pointer(table_id) = check_pointer(decoder)->mapping_table_id(component_index);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_decoder_get_mapping_table_index(const charls_jpegls_decoder* decoder, const int32_t table_id, int32_t* index) noexcept
try
{
    *check_pointer(index) = check_pointer(decoder)->mapping_table_index(table_id).value_or(mapping_table_missing);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_decoder_get_mapping_table_count(const charls_jpegls_decoder* decoder, int32_t* count) noexcept
try
{
    *check_pointer(count) = check_pointer(decoder)->mapping_table_count();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_decoder_get_mapping_table_info(
    const charls_jpegls_decoder* decoder, const int32_t index, table_info* table_info) noexcept
try
{
    *check_pointer(table_info) = check_pointer(decoder)->mapping_table_info(index);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_decoder_get_mapping_table_data(
    const charls_jpegls_decoder* decoder, const int32_t index, void* table_data, const size_t table_size_bytes) noexcept
try
{
    check_pointer(decoder)->mapping_table_data(index, {static_cast<byte*>(table_data), table_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

} // extern "C"
