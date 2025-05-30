// SPDX-FileCopyrightText: © 2019 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.hpp"

#include <charls/charls_jpegls_encoder.h>
#include <charls/version.h>

#include "color_transform.hpp"
#include "jpeg_stream_writer.hpp"
#include "jpegls_preset_coding_parameters.hpp"
#include "make_scan_codec.hpp"
#include "scan_encoder.hpp"
#include "util.hpp"

#include <new>

using namespace charls;
using impl::throw_jpegls_error;
using std::byte;

namespace charls { namespace {

constexpr bool has_option(encoding_options options, encoding_options option_to_test)
{
    using T = std::underlying_type_t<encoding_options>;
    return (static_cast<encoding_options>(static_cast<T>(options) & static_cast<T>(option_to_test))) == option_to_test;
}

}} // namespace charls

struct charls_jpegls_encoder final
{
    void destination(const span<byte> destination)
    {
        check_argument(destination);
        check_operation(state_ == state::initial);

        writer_.destination(destination);
        state_ = state::destination_set;
    }

    void frame_info(const charls_frame_info& frame_info)
    {
        check_argument(frame_info.width > 0, jpegls_errc::invalid_argument_width);
        check_argument(frame_info.height > 0, jpegls_errc::invalid_argument_height);
        check_argument_range(minimum_bits_per_sample, maximum_bits_per_sample, frame_info.bits_per_sample,
                             jpegls_errc::invalid_argument_bits_per_sample);
        check_argument_range(minimum_component_count, maximum_component_count, frame_info.component_count,
                             jpegls_errc::invalid_argument_component_count);

        frame_info_ = frame_info;
    }

    void interleave_mode(const interleave_mode interleave_mode)
    {
        check_interleave_mode(interleave_mode, jpegls_errc::invalid_argument_interleave_mode);

        interleave_mode_ = interleave_mode;
    }

    void near_lossless(const int32_t near_lossless)
    {
        check_argument_range(0, maximum_near_lossless, near_lossless, jpegls_errc::invalid_argument_near_lossless);

        near_lossless_ = near_lossless;
    }

    void encoding_options(const encoding_options encoding_options)
    {
        constexpr charls::encoding_options all_options = encoding_options::even_destination_size |
                                                         encoding_options::include_version_number |
                                                         encoding_options::include_pc_parameters_jai;
        check_argument(encoding_options >= encoding_options::none && encoding_options <= all_options,
                       jpegls_errc::invalid_argument_encoding_options);

        encoding_options_ = encoding_options;
    }

    void preset_coding_parameters(const jpegls_pc_parameters& preset_coding_parameters) noexcept
    {
        // Note: validation will be done just before decoding, as more info is needed for the validation.
        user_preset_coding_parameters_ = preset_coding_parameters;
    }

    void color_transformation(const color_transformation color_transformation)
    {
        check_argument_range(color_transformation::none, color_transformation::hp3, color_transformation,
                             jpegls_errc::invalid_argument_color_transformation);

        color_transformation_ = color_transformation;
    }

    void set_mapping_table_id(const int32_t component_index, const int32_t table_id)
    {
        check_argument_range(minimum_component_index, maximum_component_index, component_index);
        check_argument_range(0, maximum_mapping_table_id, table_id);

        writer_.set_mapping_table_id(static_cast<size_t>(component_index), table_id);
    }

    [[nodiscard]]
    size_t estimated_destination_size() const
    {
        check_operation(is_frame_info_configured());
        return checked_mul(checked_mul(checked_mul(frame_info_.width, frame_info_.height), static_cast<size_t>(frame_info_.component_count)),
                           bit_to_byte_count(frame_info_.bits_per_sample)) +
               1024 + spiff_header_size_in_bytes;
    }

    void write_spiff_header(const spiff_header& spiff_header)
    {
        check_argument(spiff_header.height > 0, jpegls_errc::invalid_argument_height);
        check_argument(spiff_header.width > 0, jpegls_errc::invalid_argument_width);
        write_spiff_header_core(spiff_header);
    }

    void write_standard_spiff_header(const spiff_color_space color_space, const spiff_resolution_units resolution_units,
                                     const uint32_t vertical_resolution, const uint32_t horizontal_resolution)
    {
        check_operation(is_frame_info_configured());

        write_spiff_header_core({spiff_profile_id::none, frame_info_.component_count, frame_info_.height, frame_info_.width,
                                 color_space, frame_info_.bits_per_sample, spiff_compression_type::jpeg_ls, resolution_units,
                                 vertical_resolution, horizontal_resolution});
    }

    void write_spiff_entry(const uint32_t entry_tag, const span<const byte> entry_data)
    {
        check_argument(entry_data);
        check_argument(entry_tag != spiff_end_of_directory_entry_type);
        check_argument(entry_data.size() <= spiff_entry_max_data_size, jpegls_errc::invalid_argument_size);
        check_operation(state_ == state::spiff_header);

        writer_.write_spiff_directory_entry(entry_tag, entry_data);
    }

    void write_spiff_end_of_directory_entry()
    {
        check_operation(state_ == state::spiff_header);
        transition_to_tables_and_miscellaneous_state();
    }

    void write_comment(const span<const byte> comment)
    {
        check_argument(comment);
        check_argument(comment.size() <= segment_max_data_size, jpegls_errc::invalid_argument_size);
        check_state_can_write();

        transition_to_tables_and_miscellaneous_state();
        writer_.write_comment_segment(comment);
    }

    void write_application_data(const int32_t application_data_id, const span<const byte> application_data)
    {
        check_argument_range(minimum_application_data_id, maximum_application_data_id, application_data_id);
        check_argument(application_data);
        check_argument(application_data.size() <= segment_max_data_size, jpegls_errc::invalid_argument_size);
        check_state_can_write();

        transition_to_tables_and_miscellaneous_state();
        writer_.write_application_data_segment(application_data_id, application_data);
    }

    void write_mapping_table(const int32_t table_id, const int32_t entry_size, const span<const byte> table_data)
    {
        check_argument_range(minimum_mapping_table_id, maximum_mapping_table_id, table_id);
        check_argument_range(minimum_mapping_entry_size, maximum_mapping_entry_size, entry_size);
        check_argument(table_data);
        check_argument(table_data.size() >= static_cast<size_t>(entry_size), jpegls_errc::invalid_argument_size);
        check_state_can_write();

        transition_to_tables_and_miscellaneous_state();
        writer_.write_jpegls_preset_parameters_segment(table_id, entry_size, table_data);
    }

    void encode(const span<const byte> source, const size_t stride)
    {
        encode_components(source, frame_info_.component_count, stride);
    }

    void encode_components(span<const byte> source, const int32_t source_component_count, const size_t stride)
    {
        check_argument(source);
        check_state_can_write();
        check_operation(is_frame_info_configured());
        check_interleave_mode_against_component_count();
        const size_t scan_stride{check_stride_and_source_size(source.size(), stride, source_component_count)};

        const int32_t maximum_sample_value{calculate_maximum_sample_value(frame_info_.bits_per_sample)};
        if (UNLIKELY(
                !is_valid(user_preset_coding_parameters_, maximum_sample_value, near_lossless_, &preset_coding_parameters_)))
            throw_jpegls_error(jpegls_errc::invalid_argument_jpegls_pc_parameters);

        if (encoded_component_count_ == 0)
        {
            transition_to_tables_and_miscellaneous_state();
            write_color_transform_segment();
            write_start_of_frame_segment();
            write_jpegls_preset_parameters_segment(maximum_sample_value);
        }

        if (interleave_mode_ == interleave_mode::none)
        {
            const size_t byte_count_component{scan_stride * frame_info_.height};
            for (int32_t component{};;)
            {
                writer_.write_start_of_scan_segment(1, near_lossless_, interleave_mode_);
                encode_scan(source.data(), scan_stride, 1);

                ++component;
                if (component == source_component_count)
                    break;

                // Synchronize the source stream (encode_scan works on a local copy)
                source = source.subspan(byte_count_component);
            }
        }
        else
        {
            writer_.write_start_of_scan_segment(source_component_count, near_lossless_, interleave_mode_);
            encode_scan(source.data(), scan_stride, source_component_count);
        }

        encoded_component_count_ += source_component_count;
        if (encoded_component_count_ == frame_info_.component_count)
        {
            write_end_of_image();
        }
    }

    void create_abbreviated_format()
    {
        check_operation(state_ == state::tables_and_miscellaneous);
        write_end_of_image();
    }

    [[nodiscard]]
    size_t bytes_written() const noexcept
    {
        return writer_.bytes_written();
    }

    void rewind() noexcept
    {
        if (state_ == state::initial)
            return; // Nothing to do, stay in the same state.

        writer_.rewind();
        state_ = state::destination_set;
        encoded_component_count_ = 0;
    }

private:
    enum class state
    {
        initial,
        destination_set,
        spiff_header,
        tables_and_miscellaneous,
        completed
    };

    [[nodiscard]]
    bool is_frame_info_configured() const noexcept
    {
        return frame_info_.width != 0;
    }

    void write_spiff_header_core(const spiff_header& spiff_header)
    {
        check_operation(state_ == state::destination_set);

        writer_.write_start_of_image();
        writer_.write_spiff_header_segment(spiff_header);
        state_ = state::spiff_header;
    }

    void encode_scan(const byte* source, const size_t stride, const int32_t component_count)
    {
        const charls::frame_info frame_info{frame_info_.width, frame_info_.height, frame_info_.bits_per_sample,
                                            component_count};

        const auto encoder{make_scan_codec<scan_encoder>(frame_info, preset_coding_parameters_,
                                                         {near_lossless_, 0, interleave_mode_, color_transformation_})};
        const size_t bytes_written{encoder->encode_scan(source, stride, writer_.remaining_destination())};

        // Synchronize the destination encapsulated in the writer (encode_scan works on a local copy)
        writer_.advance_position(bytes_written);
    }

    [[nodiscard]]
    size_t check_stride_and_source_size(const size_t source_size, size_t stride, const int32_t source_component_count) const
    {
        const size_t minimum_stride{calculate_minimum_stride(source_component_count)};
        if (stride == auto_calculate_stride)
        {
            stride = minimum_stride;
        }
        else
        {
            if (UNLIKELY(stride < minimum_stride))
                throw_jpegls_error(jpegls_errc::invalid_argument_stride);
        }

        const size_t not_used_bytes_at_end{stride - minimum_stride};
        const size_t minimum_source_size{interleave_mode_ == interleave_mode::none
                                             ? (stride * source_component_count * frame_info_.height) -
                                                   not_used_bytes_at_end
                                             : (stride * frame_info_.height) - not_used_bytes_at_end};

        if (UNLIKELY(source_size < minimum_source_size))
            throw_jpegls_error(jpegls_errc::invalid_argument_size);

        return stride;
    }

    [[nodiscard]]
    size_t calculate_minimum_stride(const int32_t source_component_count) const noexcept
    {
        const auto stride{static_cast<size_t>(frame_info_.width) * bit_to_byte_count(frame_info_.bits_per_sample)};
        if (interleave_mode_ == interleave_mode::none)
            return stride;

        return stride * source_component_count;
    }

    void check_state_can_write() const
    {
        check_operation(state_ >= state::destination_set && state_ < state::completed);
    }

    void check_interleave_mode_against_component_count() const
    {
        if (UNLIKELY(frame_info_.component_count == 1 && interleave_mode_ != interleave_mode::none))
            throw_jpegls_error(jpegls_errc::invalid_argument_interleave_mode);
    }

    void transition_to_tables_and_miscellaneous_state()
    {
        if (state_ == state::tables_and_miscellaneous)
            return;

        if (state_ == state::spiff_header)
        {
            writer_.write_spiff_end_of_directory_entry();
        }
        else
        {
            writer_.write_start_of_image();
        }

        if (has_option(encoding_options::include_version_number))
        {
            constexpr std::string_view version_number{"charls " TO_STRING(CHARLS_VERSION_MAJOR) "." TO_STRING(
                CHARLS_VERSION_MINOR) "." TO_STRING(CHARLS_VERSION_PATCH)};
            writer_.write_comment_segment(as_bytes(span{version_number.data(), version_number.size() + 1}));
        }

        state_ = state::tables_and_miscellaneous;
    }

    void write_color_transform_segment()
    {
        if (color_transformation_ == color_transformation::none)
            return;

        if (UNLIKELY(!color_transformation_possible(frame_info_)))
            throw_jpegls_error(jpegls_errc::invalid_argument_color_transformation);

        writer_.write_color_transform_segment(color_transformation_);
    }

    void write_start_of_frame_segment()
    {
        if (writer_.write_start_of_frame_segment(frame_info_))
        {
            // Image dimensions are oversized and need to be written to a JPEG-LS preset parameters (LSE) segment.
            writer_.write_jpegls_preset_parameters_segment(frame_info_.height, frame_info_.width);
        }
    }

    void write_jpegls_preset_parameters_segment(const int32_t maximum_sample_value)
    {
        if (!is_default(user_preset_coding_parameters_, compute_default(maximum_sample_value, near_lossless_)) ||
            (has_option(encoding_options::include_pc_parameters_jai) && frame_info_.bits_per_sample > 12))
        {
            // Write the actual used values to the stream, not zero's.
            // Explicit values reduces the risk for decoding by other implementations.
            writer_.write_jpegls_preset_parameters_segment(preset_coding_parameters_);
        }
    }

    void write_end_of_image()
    {
        writer_.write_end_of_image(has_option(encoding_options::even_destination_size));
        state_ = state::completed;
    }

    [[nodiscard]]
    bool has_option(const charls::encoding_options option_to_test) const noexcept
    {
        return ::has_option(encoding_options_, option_to_test);
    }

    charls_frame_info frame_info_{};
    int32_t near_lossless_{};
    int32_t encoded_component_count_{};
    charls::interleave_mode interleave_mode_{};
    charls::color_transformation color_transformation_{};
    charls::encoding_options encoding_options_{};
    state state_{};
    jpeg_stream_writer writer_;
    jpegls_pc_parameters user_preset_coding_parameters_{};
    jpegls_pc_parameters preset_coding_parameters_{};
};

extern "C" {

USE_DECL_ANNOTATIONS charls_jpegls_encoder* CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_create() noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26402 26409)     // don't use new and delete + scoped object and move
    return new (std::nothrow) charls_jpegls_encoder; // NOLINT(cppcoreguidelines-owning-memory)
}


USE_DECL_ANNOTATIONS void CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_destroy(const charls_jpegls_encoder* encoder) noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26401 26409) // don't use new and delete + non-owner.
    delete encoder;                              // NOLINT(cppcoreguidelines-owning-memory)
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_destination_buffer(
    charls_jpegls_encoder* encoder, void* destination_buffer, const size_t destination_size_bytes) noexcept
try
{
    check_pointer(encoder)->destination({static_cast<byte*>(destination_buffer), destination_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_frame_info(charls_jpegls_encoder* encoder, const charls_frame_info* frame_info) noexcept
try
{
    check_pointer(encoder)->frame_info(*check_pointer(frame_info));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_near_lossless(charls_jpegls_encoder* encoder, const int32_t near_lossless) noexcept
try
{
    check_pointer(encoder)->near_lossless(near_lossless);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_encoding_options(
    charls_jpegls_encoder* encoder, const charls_encoding_options encoding_options) noexcept
try
{
    check_pointer(encoder)->encoding_options(encoding_options);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_interleave_mode(
    charls_jpegls_encoder* encoder, const charls_interleave_mode interleave_mode) noexcept
try
{
    check_pointer(encoder)->interleave_mode(interleave_mode);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_preset_coding_parameters(
    charls_jpegls_encoder* encoder, const charls_jpegls_pc_parameters* preset_coding_parameters) noexcept
try
{
    check_pointer(encoder)->preset_coding_parameters(*check_pointer(preset_coding_parameters));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_color_transformation(
    charls_jpegls_encoder* encoder, const charls_color_transformation color_transformation) noexcept
try
{
    check_pointer(encoder)->color_transformation(color_transformation);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_mapping_table_id(
    charls_jpegls_encoder* encoder, const int32_t component_index, const int32_t table_id) noexcept
try
{
    check_pointer(encoder)->set_mapping_table_id(component_index, table_id);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_estimated_destination_size(const charls_jpegls_encoder* encoder, size_t* size_in_bytes) noexcept
try
{
    *check_pointer(size_in_bytes) = check_pointer(encoder)->estimated_destination_size();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_header(charls_jpegls_encoder* encoder, const charls_spiff_header* spiff_header) noexcept
try
{
    check_pointer(encoder)->write_spiff_header(*check_pointer(spiff_header));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_write_standard_spiff_header(
    charls_jpegls_encoder* encoder, const charls_spiff_color_space color_space,
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


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_entry(charls_jpegls_encoder* encoder, const uint32_t entry_tag, const void* entry_data,
                                        const size_t entry_data_size_bytes) noexcept
try
{
    check_pointer(encoder)->write_spiff_entry(entry_tag, {static_cast<const byte*>(entry_data), entry_data_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_end_of_directory_entry(charls_jpegls_encoder* encoder) noexcept
try
{
    check_pointer(encoder)->write_spiff_end_of_directory_entry();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_write_comment(
    charls_jpegls_encoder* encoder, const void* comment, const size_t comment_size_bytes) noexcept
try
{
    check_pointer(encoder)->write_comment({static_cast<const byte*>(comment), comment_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_application_data(charls_jpegls_encoder* encoder, const int32_t application_data_id,
                                             const void* application_data, const size_t application_data_size_bytes) noexcept
try
{
    check_pointer(encoder)->write_application_data(
        application_data_id, {static_cast<const byte*>(application_data), application_data_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_mapping_table(charls_jpegls_encoder* encoder, const int32_t table_id, const int32_t entry_size,
                                          const void* table_data, size_t table_data_size_bytes) noexcept
try
{
    check_pointer(encoder)->write_mapping_table(table_id, entry_size,
                                                {static_cast<const byte*>(table_data), table_data_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_encode_from_buffer(charls_jpegls_encoder* encoder, const void* source_buffer,
                                         const size_t source_size_bytes, const uint32_t stride) noexcept
try
{
    check_pointer(encoder)->encode({static_cast<const byte*>(source_buffer), source_size_bytes}, stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_encode_components_from_buffer(
    charls_jpegls_encoder* encoder, const void* source_buffer, const size_t source_size_bytes,
    const int32_t source_component_count, const uint32_t stride) noexcept
try
{
    check_pointer(encoder)->encode_components({static_cast<const byte*>(source_buffer), source_size_bytes},
                                              source_component_count, stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_create_abbreviated_format(charls_jpegls_encoder* encoder) noexcept
try
{
    check_pointer(encoder)->create_abbreviated_format();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_bytes_written(const charls_jpegls_encoder* encoder, size_t* bytes_written) noexcept
try
{
    *check_pointer(bytes_written) = check_pointer(encoder)->bytes_written();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_rewind(charls_jpegls_encoder* encoder) noexcept
try
{
    check_pointer(encoder)->rewind();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}

} // extern "C"
