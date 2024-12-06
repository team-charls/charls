// SPDX-FileCopyrightText: © 2020 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "charls_jpegls_decoder.h"
#include "validate_spiff_header.h"
#include "jpegls_error.hpp"

#ifndef CHARLS_BUILD_AS_CPP_MODULE
#include <functional>
#include <memory>
#include <utility>
#endif

CHARLS_EXPORT
namespace charls {

/// <summary>
/// JPEG-LS decoder class that encapsulates the C ABI interface calls and provide a native C++ interface.
/// </summary>
class jpegls_decoder final
{
public:
    /// <summary>
    /// Decodes a JPEG-LS buffer in 1 simple operation.
    /// </summary>
    /// <param name="source">Source container with the JPEG-LS encoded bytes.</param>
    /// <param name="destination">
    /// Destination container that will hold the image data on return. Container will be resized automatically.
    /// </param>
    /// <param name="maximum_size_in_bytes">
    /// The maximum output size that may be allocated, default is 94 MiB (enough to decode 8-bit color 8K image).
    /// </param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <exception cref="std::bad_alloc">Thrown when memory for the decoder could not be allocated.</exception>
    /// <returns>Frame info of the decoded image and the interleave mode.</returns>
    template<typename SourceContainer, typename DestinationContainer,
             typename DestinationContainerValueType = typename DestinationContainer::value_type>
    static std::pair<frame_info, interleave_mode> decode(const SourceContainer& source, DestinationContainer& destination,
                                                         const size_t maximum_size_in_bytes = size_t{7680} * 4320 * 3)
    {
        jpegls_decoder decoder{source, true};

        const size_t destination_size{decoder.get_destination_size()};
        if (destination_size > maximum_size_in_bytes)
            impl::throw_jpegls_error(jpegls_errc::not_enough_memory);

        destination.resize(destination_size / sizeof(DestinationContainerValueType));
        decoder.decode(destination);

        return std::make_pair(decoder.frame_info(), decoder.get_interleave_mode());
    }

    jpegls_decoder() = default;

    /// <summary>
    /// Constructs a jpegls_decoder instance.
    /// The passed container needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_buffer">Reference to the start of the source buffer.</param>
    /// <param name="source_size_bytes">Size of the source buffer in bytes.</param>
    /// <param name="parse_header">
    /// If true the SPIFF and JPEG header will be directly read from the source.
    /// </param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <exception cref="std::bad_alloc">Thrown when memory for the decoder could not be allocated.</exception>
    jpegls_decoder(CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer, const size_t source_size_bytes,
                   const bool parse_header = true)
    {
        source(source_buffer, source_size_bytes);
        if (parse_header)
        {
            read_spiff_header();
            read_header();
        }
    }

    /// <summary>
    /// Constructs a jpegls_decoder instance.
    /// The passed container needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_container">
    /// STL like container that provides the functions data() and size() and the type value_type.
    /// </param>
    /// <param name="parse_header">
    /// If true the SPIFF and JPEG header will be directly read from the source.
    /// </param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <exception cref="std::bad_alloc">Thrown when memory for the decoder could not be allocated.</exception>
    template<typename Container, typename ContainerValueType = typename Container::value_type>
    jpegls_decoder(const Container& source_container, const bool parse_header) :
        jpegls_decoder(source_container.data(), source_container.size() * sizeof(ContainerValueType), parse_header)
    {
    }

    /// <summary>
    /// Set the reference to a source buffer that contains the encoded JPEG-LS byte stream data.
    /// This buffer needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_buffer">Reference to the start of the source buffer.</param>
    /// <param name="source_size_bytes">Size of the source buffer in bytes.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
    jpegls_decoder& source(CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer,
                           const size_t source_size_bytes)
    {
        check_jpegls_errc(charls_jpegls_decoder_set_source_buffer(decoder(), source_buffer, source_size_bytes));
        return *this;
    }

    /// <summary>
    /// Set the reference to a source container that contains the encoded JPEG-LS byte stream data.
    /// This container needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_container">
    /// STL like container that provides the functions data() and size() and the type value_type.
    /// </param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    template<typename Container, typename ContainerValueType = typename Container::value_type>
    jpegls_decoder& source(const Container& source_container)
    {
        return source(source_container.data(), source_container.size() * sizeof(ContainerValueType));
    }

    /// <summary>
    /// Tries to read the SPIFF header from the JPEG-LS stream.
    /// If a SPIFF header exists it will be returned otherwise the struct will be filled with default values.
    /// The header_found parameter will be set to true if the spiff header could be read.
    /// Call read_header to read the normal JPEG header afterward.
    /// </summary>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>True if a valid SPIFF header could be found.</returns>
    bool read_spiff_header()
    {
        std::error_code ec;
        read_spiff_header(ec);
        check_jpegls_errc(static_cast<jpegls_errc>(ec.value()));
        return spiff_header_has_value_;
    }

    /// <summary>
    /// Tries to read the SPIFF header from the JPEG-LS stream.
    /// If a SPIFF header exists it will be returned otherwise the struct will be filled with default values.
    /// The header_found parameter will be set to true if the spiff header could be read.
    /// Call read_header to read the normal JPEG header afterward.
    /// </summary>
    /// <param name="ec">The out-parameter for error reporting.</param>
    /// <returns>True if a valid SPIFF header could be found.</returns>
    bool read_spiff_header(std::error_code& ec) noexcept
    {
        int32_t found;
        ec = charls_jpegls_decoder_read_spiff_header(decoder(), &spiff_header_, &found);
        spiff_header_has_value_ = found != 0;
        return spiff_header_has_value_;
    }

    /// <summary>
    /// Reads the JPEG-LS header from the beginning of the JPEG-LS byte stream or after the SPIFF header.
    /// After this function is called frame info and other info can be retrieved.
    /// </summary>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_decoder& read_header()
    {
        std::error_code ec;
        read_header(ec);
        check_jpegls_errc(static_cast<jpegls_errc>(ec.value()));
        return *this;
    }

    /// <summary>
    /// Reads the JPEG-LS header from the beginning of the JPEG-LS byte stream or after the SPIFF header.
    /// After this function is called frame info and other info can be retrieved.
    /// If a SPIFF header is present it will be validated against the information in the frame info.
    /// </summary>
    /// <param name="ec">The out-parameter for error reporting.</param>
    jpegls_decoder& read_header(CHARLS_OUT std::error_code& ec) noexcept
    {
        ec = charls_jpegls_decoder_read_header(decoder());
        if (ec == jpegls_errc::success)
        {
            ec = charls_jpegls_decoder_get_frame_info(decoder(), &frame_info_);
            if (ec == jpegls_errc::success && spiff_header_has_value_)
            {
                ec = charls_validate_spiff_header(&spiff_header_, &frame_info_);
            }
        }
        return *this;
    }

    /// <summary>
    /// Returns true if a valid SPIFF header was found.
    /// </summary>
    /// <returns>True when a SPIFF header was found, false otherwise.</returns>
    [[nodiscard]]
    bool spiff_header_has_value() const noexcept
    {
        return spiff_header_has_value_;
    }

    /// <summary>
    /// Returns the SPIFF header, if read and found.
    /// Function can be called after read_spiff_header and spiff_header_has_value.
    /// </summary>
    /// <returns>The SPIFF header.</returns>
    [[nodiscard]]
    const charls::spiff_header& spiff_header() const& noexcept
    {
        return spiff_header_;
    }

    /// <summary>
    /// Returns the SPIFF header, if read and found.
    /// Function can be called after read_spiff_header and spiff_header_has_value.
    /// </summary>
    /// <returns>The SPIFF header.</returns>
    [[nodiscard]]
    charls::spiff_header spiff_header() const&& noexcept
    {
        return spiff_header_;
    }

    /// <summary>
    /// Returns information about the frame stored in the JPEG-LS byte stream.
    /// Function can be called after read_header.
    /// </summary>
    /// <returns>The frame info that describes the image stored in the JPEG-LS byte stream.</returns>
    [[nodiscard]]
    const charls::frame_info& frame_info() const& noexcept
    {
        return frame_info_;
    }

    /// <summary>
    /// Returns information about the frame stored in the JPEG-LS byte stream.
    /// Function can be called after read_header.
    /// </summary>
    /// <returns>The frame info that describes the image stored in the JPEG-LS byte stream.</returns>
    [[nodiscard]]
    charls::frame_info frame_info() const&& noexcept
    {
        return frame_info_;
    }

    /// <summary>
    /// Returns the NEAR parameter that was used to encode the scan. A value of 0 means lossless.
    /// </summary>
    /// <param name="component_index">The component index for which the NEAR parameter should be retrieved.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The value of the NEAR parameter.</returns>
    [[nodiscard]]
    int32_t get_near_lossless(const int32_t component_index = 0) const
    {
        int32_t near_lossless;
        check_jpegls_errc(charls_jpegls_decoder_get_near_lossless(decoder(), component_index, &near_lossless));
        return near_lossless;
    }

    /// <summary>
    /// Returns the interleave mode that was used to encode the scan.
    /// </summary>
    /// <param name="component_index">The component index for which the interleave mode should be retrieved.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The value of the interleave mode.</returns>
    [[nodiscard]]
    interleave_mode get_interleave_mode(const int32_t component_index = 0) const
    {
        interleave_mode interleave_mode;
        check_jpegls_errc(charls_jpegls_decoder_get_interleave_mode(decoder(), component_index, &interleave_mode));
        return interleave_mode;
    }

    /// <summary>
    /// Returns the preset coding parameters used to encode the first scan.
    /// </summary>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The values of the JPEG-LS preset coding parameters.</returns>
    [[nodiscard]]
    jpegls_pc_parameters preset_coding_parameters() const
    {
        jpegls_pc_parameters preset_coding_parameters;
        check_jpegls_errc(charls_jpegls_decoder_get_preset_coding_parameters(decoder(), 0, &preset_coding_parameters));
        return preset_coding_parameters;
    }

    /// <summary>
    /// Returns the HP color transformation that was used to encode the scan.
    /// </summary>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The value of the color transformation.</returns>
    [[nodiscard]]
    charls::color_transformation color_transformation() const
    {
        charls::color_transformation color_transformation;
        check_jpegls_errc(charls_jpegls_decoder_get_color_transformation(decoder(), &color_transformation));
        return color_transformation;
    }

    /// <summary>
    /// Returns the size required for the destination buffer in bytes to hold the decoded pixel data.
    /// Function can be called after read_header.
    /// </summary>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The required size in bytes of the destination buffer.</returns>
    [[nodiscard]]
    size_t get_destination_size(const uint32_t stride = 0) const
    {
        size_t size_in_bytes;
        check_jpegls_errc(charls_jpegls_decoder_get_destination_size(decoder(), stride, &size_in_bytes));
        return size_in_bytes;
    }

    /// <summary>
    /// Will decode the JPEG-LS byte stream set with source into the destination buffer.
    /// </summary>
    /// <param name="destination_buffer">Byte array that holds the encoded bytes when the function returns.</param>
    /// <param name="destination_size_bytes">Length of the destination buffer in bytes.</param>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    CHARLS_ATTRIBUTE_ACCESS((access(write_only, 2, 3)))
    void decode(CHARLS_OUT_WRITES_BYTES(destination_size_bytes) void* destination_buffer,
                const size_t destination_size_bytes, const uint32_t stride = 0)
    {
        check_jpegls_errc(
            charls_jpegls_decoder_decode_to_buffer(decoder(), destination_buffer, destination_size_bytes, stride));
    }

    /// <summary>
    /// Will decode the JPEG-LS byte stream set with source into the destination container.
    /// </summary>
    /// <param name="destination_container">
    /// STL like container that provides the functions data() and size() and the type value_type.
    /// </param>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    template<typename Container, typename ContainerValueType = typename Container::value_type>
    void decode(CHARLS_OUT Container& destination_container, const uint32_t stride = 0)
    {
        decode(destination_container.data(), destination_container.size() * sizeof(ContainerValueType), stride);
    }

    /// <summary>
    /// Will decode the JPEG-LS byte stream set with source and return a container with the decoded data.
    /// </summary>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>Container with the decoded data.</returns>
    template<typename Container, typename ContainerValueType = typename Container::value_type>
    [[nodiscard]]
    Container decode(const uint32_t stride = 0)
    {
        Container destination(get_destination_size() / sizeof(ContainerValueType));

        decode(destination.data(), destination.size() * sizeof(ContainerValueType), stride);
        return destination;
    }

    /// <summary>
    /// Will install a function that will be called when a comment (COM) segment is found.
    /// </summary>
    /// <remarks>
    /// Pass a nullptr to uninstall the callback function.
    /// The callback can throw an exception to abort the decoding process.
    /// This abort will be returned as a callback_failed error code.
    /// </remarks>
    /// <param name="comment_handler">Function object to the comment handler.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_decoder& at_comment(std::function<void(const void* data, size_t size)> comment_handler)
    {
        comment_handler_ = std::move(comment_handler);
        check_jpegls_errc(
            charls_jpegls_decoder_at_comment(decoder(), comment_handler_ ? &at_comment_callback : nullptr, this));
        return *this;
    }

    /// <summary>
    /// Will install a function that will be called when an application data (APPn) segment is found.
    /// </summary>
    /// <remarks>
    /// Pass a nullptr to uninstall the callback function.
    /// The callback can throw an exception to abort the decoding process.
    /// This abort will be returned as a callback_failed error code.
    /// </remarks>
    /// <param name="application_data_handler">Function object to the application data handler.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_decoder& at_application_data(
        std::function<void(int32_t application_data_id, const void* data, size_t size)> application_data_handler)
    {
        application_data_handler_ = std::move(application_data_handler);
        check_jpegls_errc(charls_jpegls_decoder_at_application_data(
            decoder(), application_data_handler_ ? &at_application_data_callback : nullptr, this));
        return *this;
    }

    /// <summary>
    /// Returns the compressed data format of the JPEG-LS data stream.
    /// </summary>
    /// <remarks>
    /// Function can be called after reading the header or after processing the complete JPEG-LS stream.
    /// After reading the header the method may report unknown or abbreviated_table_specification.
    /// </remarks>
    /// <returns>The compressed data format.</returns>
    [[nodiscard]]
    charls::compressed_data_format compressed_data_format() const
    {
        charls::compressed_data_format format;
        check_jpegls_errc(charls_decoder_get_compressed_data_format(decoder(), &format));
        return format;
    }

    /// <summary>
    /// Returns the mapping table ID referenced by the component or 0 when no mapping table is used.
    /// </summary>
    /// <remarks>
    /// Function should be called after processing the complete JPEG-LS stream.
    /// </remarks>
    /// <param name="component_index">Index of the component.</param>
    /// <returns>The mapping table ID or 0 when no mapping table is referenced by the component.</returns>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    [[nodiscard]]
    int32_t get_mapping_table_id(const int32_t component_index) const
    {
        int32_t table_id;
        check_jpegls_errc(charls_decoder_get_mapping_table_id(decoder(), component_index, &table_id));
        return table_id;
    }

    /// <summary>
    /// Converts the mapping table ID to a mapping table index.
    /// </summary>
    /// <remarks>
    /// Function should be called after processing the complete JPEG-LS stream.
    /// </remarks>
    /// <param name="mapping_table_id">Mapping table ID to lookup.</param>
    /// <returns>
    /// The index of the mapping table or -1 (mapping_table_missing) when the table is not present in the JPEG-LS stream.
    /// </returns>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    [[nodiscard]]
    int32_t find_mapping_table_index(const int32_t mapping_table_id) const
    {
        int32_t index;
        check_jpegls_errc(charls_decoder_find_mapping_table_index(decoder(), mapping_table_id, &index));
        return index;
    }

    /// <summary>
    /// Returns the count of mapping tables present in the JPEG-LS stream.
    /// </summary>
    /// <remarks>
    /// Function should be called after processing the complete JPEG-LS stream.
    /// </remarks>
    /// <returns>The number of mapping tables present in the JPEG-LS stream.</returns>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    [[nodiscard]]
    int32_t mapping_table_count() const
    {
        int32_t count;
        check_jpegls_errc(charls_decoder_get_mapping_table_count(decoder(), &count));
        return count;
    }

    /// <summary>
    /// Returns information about a mapping table.
    /// </summary>
    /// <remarks>
    /// Function should be called after processing the complete JPEG-LS stream.
    /// </remarks>
    /// <param name="index">Index of the requested mapping table.</param>
    /// <returns>Mapping table information</returns>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    [[nodiscard]]
    mapping_table_info get_mapping_table_info(const int32_t index) const
    {
        mapping_table_info info;
        check_jpegls_errc(charls_decoder_get_mapping_table_info(decoder(), index, &info));
        return info;
    }

    /// <summary>
    /// Returns the data of a mapping table.
    /// </summary>
    /// <remarks>
    /// Function should be called after processing the complete JPEG-LS stream.
    /// </remarks>
    /// <param name="index">Index of the requested mapping table.</param>
    /// <param name="table_data">Output argument, will hold the data of mapping table when the function returns.</param>
    /// <param name="table_size_bytes">Length of the table buffer in bytes.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    CHARLS_ATTRIBUTE_ACCESS((access(write_only, 3, 4)))
    void get_mapping_table_data(const int32_t index, CHARLS_OUT_WRITES_BYTES(table_size_bytes) void* table_data,
                                const size_t table_size_bytes) const
    {
        check_jpegls_errc(charls_decoder_get_mapping_table_data(decoder(), index, table_data, table_size_bytes));
    }

    /// <summary>
    /// Returns the data of a mapping table.
    /// </summary>
    /// <remarks>
    /// Function should be called after processing the complete JPEG-LS stream.
    /// </remarks>
    /// <param name="index">Index of the requested mapping table.</param>
    /// <param name="table_data">Output argument, will hold data of the mapping table when the function returns.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    template<typename Container, typename ContainerValueType = typename Container::value_type>
    void get_mapping_table_data(const int32_t index, Container& table_data) const
    {
        get_mapping_table_data(index, table_data.data(), table_data.size() * sizeof(ContainerValueType));
    }

private:
    [[nodiscard]]
    charls_jpegls_decoder* decoder() noexcept
    {
        return decoder_.get();
    }

    [[nodiscard]]
    const charls_jpegls_decoder* decoder() const noexcept
    {
        return decoder_.get();
    }

    [[nodiscard]]
    static charls_jpegls_decoder* create_decoder()
    {
        charls_jpegls_decoder* decoder{charls_jpegls_decoder_create()};
        if (!decoder)
            throw std::bad_alloc();

        return decoder;
    }

    static void destroy_decoder(CHARLS_IN_OPT const charls_jpegls_decoder* decoder) noexcept
    {
        charls_jpegls_decoder_destroy(decoder);
    }

    static int32_t CHARLS_API_CALLING_CONVENTION at_comment_callback(const void* data, const size_t size,
                                                                     void* user_context) noexcept
    {
        try
        {
            static_cast<jpegls_decoder*>(user_context)->comment_handler_(data, size);
            return 0;
        }
        catch (...)
        {
            return 1; // will trigger jpegls_errc::callback_failed.
        }
    }

    static int32_t CHARLS_API_CALLING_CONVENTION at_application_data_callback(const int32_t application_data_id,
                                                                              const void* data, const size_t size,
                                                                              void* user_context) noexcept
    {
        try
        {
            static_cast<jpegls_decoder*>(user_context)->application_data_handler_(application_data_id, data, size);
            return 0;
        }
        catch (...)
        {
            return 1; // will trigger jpegls_errc::callback_failed.
        }
    }

    std::unique_ptr<charls_jpegls_decoder, void (*)(const charls_jpegls_decoder*)> decoder_{create_decoder(),
                                                                                            &destroy_decoder};
    bool spiff_header_has_value_{};
    charls::spiff_header spiff_header_{};
    charls::frame_info frame_info_{};
    std::function<void(const void*, size_t)> comment_handler_{};
    std::function<void(int32_t, const void*, size_t)> application_data_handler_{};
};

} // namespace charls