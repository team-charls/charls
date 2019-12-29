// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_error.h"
#include "version.h"

#ifdef __cplusplus

#include <memory>
#include <utility>

#else

#include <stdbool.h>
#include <stddef.h>

#endif


#ifdef __cplusplus

struct charls_jpegls_decoder;
struct charls_jpegls_encoder;

extern "C" {

#else

typedef struct charls_jpegls_decoder charls_jpegls_decoder;
typedef struct charls_jpegls_encoder charls_jpegls_encoder;

#endif

// The following functions define the public C API of the CharLS library.
// The C++ API is defined after the C API.

/// <summary>
/// Creates a JPEG-LS decoder instance, when finished with the instance destroy it with the function charls_jpegls_decoder_destroy.
/// </summary>
/// <returns>A reference to a new created decoder instance, or a null pointer when the creation fails.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_decoder* CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_create(void) CHARLS_NOEXCEPT;

/// <summary>
/// Destroys a JPEG-LS decoder instance created with charls_jpegls_decoder_create and releases all internal resources attached to it.
/// </summary>
/// <param name="decoder">Instance to destroy. If a null pointer is passed as argument, no action occurs.</param>
CHARLS_API_IMPORT_EXPORT void CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_destroy(const charls_jpegls_decoder* decoder) CHARLS_NOEXCEPT;

/// <summary>
/// Set the reference to a source buffer that contains the encoded JPEG-LS byte stream data.
/// This buffer needs to remain valid until the buffer is fully decoded.
/// </summary>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="source_buffer">Reference to the start of the source buffer.</param>
/// <param name="source_size_bytes">Size of the source buffer in bytes.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_set_source_buffer(charls_jpegls_decoder* decoder, const void* source_buffer, size_t source_size_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Tries to read the SPIFF header from the source buffer.
/// If a SPIFF header exists its content will be put into the spiff_header parameter and header_found will be set to 1.
/// Call charls_jpegls_decoder_read_header to read the normal JPEG header afterwards.
/// </summary>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="spiff_header">Output argument, will hold the SPIFF header when one could be found.</param>
/// <param name="header_found">Output argument, will hold 1 if a SPIFF header could be found, otherwise 0.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_spiff_header(charls_jpegls_decoder* decoder, charls_spiff_header* spiff_header, int32_t* header_found) CHARLS_NOEXCEPT;

/// <summary>
/// Reads the JPEG-LS header from the JPEG byte stream. After this function is called frame info can be retrieved.
/// </summary>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_header(charls_jpegls_decoder* decoder) CHARLS_NOEXCEPT;

/// <summary>
/// Returns information about the frame stored in the JPEG-LS byte stream.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="frame_info">Output argument, will hold the frame info when the function returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_frame_info(const charls_jpegls_decoder* decoder, charls_frame_info* frame_info) CHARLS_NOEXCEPT;

/// <summary>
/// Returns the NEAR parameter that was used to encode the scan. A value of 0 means lossless.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="component">The component index for which the NEAR parameter should be retrieved.</param>
/// <param name="near_lossless">Reference that will hold the value of the NEAR parameter.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_near_lossless(const charls_jpegls_decoder* decoder, int32_t component, int32_t* near_lossless) CHARLS_NOEXCEPT;

/// <summary>
/// Returns the interleave mode that was used to encode the scan(s).
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="interleave_mode">Reference that will hold the value of the interleave mode.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_interleave_mode(const charls_jpegls_decoder* decoder, charls_interleave_mode* interleave_mode) CHARLS_NOEXCEPT;

/// <summary>
/// Returns the preset coding parameters used to encode the first scan.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="reserved">Reserved. Should be set to 0.</param>
/// <param name="preset_coding_parameters">Reference that will hold the values preset coding parameters.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_preset_coding_parameters(const charls_jpegls_decoder* decoder, int32_t reserved, charls_jpegls_pc_parameters* preset_coding_parameters) CHARLS_NOEXCEPT;

/// <summary>
/// Returns the size required for the destination buffer in bytes to hold the decoded pixel data.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
/// <param name="destination_size_bytes">Output argument, will hold the required size when the function returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_destination_size(const charls_jpegls_decoder* decoder, uint32_t stride, size_t* destination_size_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Will decode the JPEG-LS byte stream from the source buffer into the destination buffer.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="destination_buffer">Byte array that holds the encoded bytes when the function returns.</param>
/// <param name="destination_size_bytes">Length of the array in bytes. If the array is too small the function will return an error.</param>
/// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_decode_to_buffer(const charls_jpegls_decoder* decoder, void* destination_buffer, size_t destination_size_bytes, uint32_t stride) CHARLS_NOEXCEPT;


/// <summary>
/// Creates a JPEG-LS encoder instance, when finished with the instance destroy it with the function charls_jpegls_encoder_destroy.
/// </summary>
/// <returns>A reference to a new created encoder instance, or a null pointer when the creation fails.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_encoder* CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_create(void) CHARLS_NOEXCEPT;

/// <summary>
/// Destroys a JPEG-LS encoder instance created with charls_jpegls_encoder_create and releases all internal resources attached to it.
/// </summary>
/// <param name="encoder">Instance to destroy. If a null pointer is passed as argument, no action occurs.</param>
CHARLS_API_IMPORT_EXPORT void CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_destroy(const charls_jpegls_encoder* encoder) CHARLS_NOEXCEPT;

/// <summary>
/// Configures the frame that needs to be encoded. This information will be written to the Start of Frame segment.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="frame_info">Information about the frame that needs to be encoded.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_frame_info(charls_jpegls_encoder* encoder, const charls_frame_info* frame_info) CHARLS_NOEXCEPT;

/// <summary>
/// Configures the NEAR parameter the encoder should use. A value of 0 means lossless, 0 is also the default.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="near_lossless">Value of the NEAR parameter.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_near_lossless(charls_jpegls_encoder* encoder, int32_t near_lossless) CHARLS_NOEXCEPT;

/// <summary>
/// Configures the interleave mode the encoder should use. The default is none.
/// The encoder expects the input buffer in the same format as the interleave mode.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="interleave_mode">Value of the interleave mode.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_interleave_mode(charls_jpegls_encoder* encoder, charls_interleave_mode interleave_mode) CHARLS_NOEXCEPT;

/// <summary>
/// Configures the preset coding parameters the encoder should use.
/// If not set the encoder will use the default preset coding parameters as defined by the JPEG-LS standard.
/// Only when the coding parameters are different then the default parameters, they will be written to the
/// JPEG-LS stream during the encode phase.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="preset_coding_parameters">Reference to the preset coding parameters.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_preset_coding_parameters(charls_jpegls_encoder* encoder, const charls_jpegls_pc_parameters* preset_coding_parameters) CHARLS_NOEXCEPT;

/// <summary>
/// Configures the HP color transformation the encoder should use.
/// If not set the encoder will use no color transformation.
/// Color transformations are a HP extension and not defined by the JPEG-LS standard and can only be set for 3 component encodings.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="color_transformation">The color transformation parameters.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_color_transformation(charls_jpegls_encoder* encoder, charls_color_transformation color_transformation) CHARLS_NOEXCEPT;

/// <summary>
/// Returns the size in bytes, that the encoder expects are needed to hold the encoded image.
/// </summary>
/// <remarks>
/// Size for dynamic extras like SPIFF entries and other tables are not included in this size.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="size_in_bytes">Reference to the size that will be set when the functions returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_estimated_destination_size(const charls_jpegls_encoder* encoder, size_t* size_in_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Set the reference to the destination buffer that will contain the encoded JPEG-LS byte stream data after encoding.
/// This buffer needs to remain valid during the encoding process.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="destination_buffer">Reference to the start of the destination buffer.</param>
/// <param name="destination_size">Size of the destination buffer in bytes.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_destination_buffer(charls_jpegls_encoder* encoder, void* destination_buffer, size_t destination_size) CHARLS_NOEXCEPT;

/// <summary>
/// Writes a standard SPIFF header to the destination. The additional values are computed from the current encoder settings.
/// A SPIFF header is optional, but recommended for standalone JPEG-LS files.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="color_space">The color space of the image.</param>
/// <param name="resolution_units">The resolution units of the next 2 parameters.</param>
/// <param name="vertical_resolution">The vertical resolution.</param>
/// <param name="horizontal_resolution">The horizontal resolution.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_standard_spiff_header(charls_jpegls_encoder* encoder,
                                                  charls_spiff_color_space color_space,
                                                  charls_spiff_resolution_units resolution_units,
                                                  uint32_t vertical_resolution,
                                                  uint32_t horizontal_resolution) CHARLS_NOEXCEPT;

/// <summary>
/// Writes a SPIFF header to the destination.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="spiff_header">Reference to a SPIFF header that will be written to the destination.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_header(charls_jpegls_encoder* encoder, const charls_spiff_header* spiff_header) CHARLS_NOEXCEPT;

/// <summary>
/// Writes a SPIFF directory entry to the destination.
/// </summary>
/// <remarks>
/// Function should be called after writing a SPIFF header.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="entry_tag">The entry tag of the directory entry.</param>
/// <param name="entry_data">The entry data of the directory entry.</param>
/// <param name="entry_data_size">The size in bytes of the directory entry [0-65528].</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_entry(charls_jpegls_encoder* encoder, uint32_t entry_tag, const void* entry_data, size_t entry_data_size) CHARLS_NOEXCEPT;

/// <summary>
/// Encodes the passed buffer with the source image data to the destination.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="source_buffer">Byte array that holds the image data that needs to be encoded.</param>
/// <param name="source_size">Length of the array in bytes.</param>
/// <param name="stride">
/// The number of bytes from one row of pixels in memory to the next row of pixels in memory.
/// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
/// </param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_encode_from_buffer(charls_jpegls_encoder* encoder, const void* source_buffer, size_t source_size, uint32_t stride) CHARLS_NOEXCEPT;

/// <summary>
/// Returns the size in bytes, that are written to the destination.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="bytes_written">Reference to the size that will be set when the functions returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_bytes_written(const charls_jpegls_encoder* encoder, size_t* bytes_written) CHARLS_NOEXCEPT;


// Note: The 4 methods below are considered obsolete and will be removed in the next major update.

/// <summary>
/// Encodes a byte array with pixel data to a JPEG-LS encoded (compressed) byte array.
/// </summary>
/// <remarks>This method is considered obsolete and will be removed in the next major update.</remarks>
/// <param name="destination">Byte array that holds the encoded bytes when the function returns.</param>
/// <param name="destinationLength">Length of the array in bytes. If the array is too small the function will return an error.</param>
/// <param name="bytesWritten">This parameter will hold the number of bytes written to the destination byte array. Cannot be NULL.</param>
/// <param name="source">Byte array that holds the pixels that should be encoded.</param>
/// <param name="sourceLength">Length of the array in bytes.</param>
/// <param name="params">Parameter object that describes the pixel data and how to encode it.</param>
/// <param name="errorMessage">Character array of at least 256 characters or NULL. Hold the error message when a failure occurs, empty otherwise.</param>
CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION JpegLsEncode(
    void* destination,
    size_t destinationLength,
    size_t* bytesWritten,
    const void* source,
    size_t sourceLength,
    const struct JlsParameters* params,
    char* errorMessage);

/// <summary>
/// Retrieves the JPEG-LS header. This info can be used to pre-allocate the uncompressed output buffer.
/// </summary>
/// <remarks>This method will be removed in the next major update.</remarks>
/// <param name="source">Byte array that holds the JPEG-LS encoded data of which the header should be extracted.</param>
/// <param name="sourceLength">Length of the array in bytes.</param>
/// <param name="params">Parameter object that describes how the pixel data is encoded.</param>
/// <param name="errorMessage">Character array of at least 256 characters or NULL. Hold the error message when a failure occurs, empty otherwise.</param>
CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION JpegLsReadHeader(
    const void* source,
    size_t sourceLength,
    struct JlsParameters* params,
    char* errorMessage);

/// <summary>
/// Encodes a JPEG-LS encoded byte array to uncompressed pixel data byte array.
/// </summary>
/// <remarks>This method will be removed in the next major update.</remarks>
/// <param name="destination">Byte array that holds the uncompressed pixel data bytes when the function returns.</param>
/// <param name="destinationLength">Length of the array in bytes. If the array is too small the function will return an error.</param>
/// <param name="source">Byte array that holds the JPEG-LS encoded data that should be decoded.</param>
/// <param name="sourceLength">Length of the array in bytes.</param>
/// <param name="params">Parameter object that describes the pixel data and how to decode it.</param>
/// <param name="errorMessage">Character array of at least 256 characters or NULL. Hold the error message when a failure occurs, empty otherwise.</param>
CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION JpegLsDecode(
    void* destination,
    size_t destinationLength,
    const void* source,
    size_t sourceLength,
    const struct JlsParameters* params,
    char* errorMessage);

/// <remarks>This method will be removed in the next major update.</remarks>
CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION JpegLsDecodeRect(
    void* destination,
    size_t destinationLength,
    const void* source,
    size_t sourceLength,
    struct JlsRect roi,
    const struct JlsParameters* params,
    char* errorMessage);

#ifdef __cplusplus

} // extern "C"


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
    /// <param name="destination">Destination container that will hold the image data on return. Container will be resized automatically.</param>
    /// <param name="maximum_size_in_bytes">The maximum output size that may be allocated, default is 94 MiB (enough to decode 8 bit color 8K image).</param>
    /// <returns>Frame info of the decoded image and the interleave mode.</returns>
    template<typename SourceContainer, typename DestinationContainer, typename ValueType = typename DestinationContainer::value_type>
    static std::pair<frame_info, interleave_mode> decode(const SourceContainer& source, DestinationContainer& destination, const size_t maximum_size_in_bytes = 7680 * 4320 * 3)
    {
        jpegls_decoder decoder{source};

        decoder.read_header();

        const size_t destination_size{decoder.destination_size()};
        if (destination_size > maximum_size_in_bytes)
            throw jpegls_error(jpegls_errc::not_enough_memory);

        destination.resize(destination_size / sizeof(ValueType));
        decoder.decode(destination);

        return std::make_pair(decoder.frame_info(), decoder.interleave_mode());
    }

    jpegls_decoder() = default;

    /// <summary>
    /// Constructs a jpegls_decoder instance.
    /// The passed container needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_container">A STL like container that provides the functions data() and size() and the type value_type.</param>
    template<typename Container>
    explicit jpegls_decoder(const Container& source_container)
    {
        source(source_container);
    }

    ~jpegls_decoder() = default;

    jpegls_decoder(const jpegls_decoder&) = delete;
    jpegls_decoder(jpegls_decoder&&) noexcept = default;
    jpegls_decoder& operator=(const jpegls_decoder&) = delete;
    jpegls_decoder& operator=(jpegls_decoder&&) noexcept = default;

    /// <summary>
    /// Set the reference to a source buffer that contains the encoded JPEG-LS byte stream data.
    /// This buffer needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_buffer">Reference to the start of the source buffer.</param>
    /// <param name="source_size_bytes">Size of the source buffer in bytes.</param>
    jpegls_decoder& source(const void* source_buffer, const size_t source_size_bytes)
    {
        check_jpegls_errc(charls_jpegls_decoder_set_source_buffer(decoder_.get(), source_buffer, source_size_bytes));
        return *this;
    }

    /// <summary>
    /// Set the reference to a source container that contains the encoded JPEG-LS byte stream data.
    /// This container needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_container">A STL like container that provides the functions data() and size() and the type value_type.</param>
    template<typename Container, typename ValueType = typename Container::value_type>
    jpegls_decoder& source(const Container& source_container)
    {
        return source(source_container.data(), source_container.size() * sizeof(ValueType));
    }

    /// <summary>
    /// Tries to read the SPIFF header from the JPEG-LS stream.
    /// If a SPIFF header exists its will be returned otherwise the struct will be filled with default values.
    /// The header_found parameter will be set to true if the spiff header could be read.
    /// Call read_header to read the normal JPEG header afterwards.
    /// </summary>
    /// <param name="header_found">Output argument, will hold true if a SPIFF header could be found, otherwise false.</param>
    /// <returns>The SPIFF header.</returns>
    CHARLS_NO_DISCARD spiff_header read_spiff_header(bool& header_found) const
    {
        std::error_code ec;
        const spiff_header header{read_spiff_header(header_found, ec)};
        check_jpegls_errc(static_cast<jpegls_errc>(ec.value()));
        return header;
    }

    /// <summary>
    /// Tries to read the SPIFF header from the JPEG-LS stream.
    /// If a SPIFF header exists its will be returned otherwise the struct will be filled with default values.
    /// The header_found parameter will be set to true if the spiff header could be read.
    /// Call read_header to read the normal JPEG header afterwards.
    /// </summary>
    /// <param name="header_found">Output argument, will hold true if a SPIFF header could be found, otherwise false.</param>
    /// <param name="ec">The out-parameter for error reporting.</param>
    /// <returns>The SPIFF header.</returns>
    CHARLS_NO_DISCARD spiff_header read_spiff_header(bool& header_found, std::error_code& ec) const noexcept
    {
        spiff_header header{};
        int32_t found;
        ec = charls_jpegls_decoder_read_spiff_header(decoder_.get(), &header, &found);
        header_found = static_cast<bool>(found);
        return header;
    }

    /// <summary>
    /// Reads the JPEG-LS header from the beginning of the JPEG-LS byte stream or after the SPIFF header.
    /// After this function is called frame info and other info can be retrieved.
    /// </summary>
    jpegls_decoder& read_header()
    {
        check_jpegls_errc(charls_jpegls_decoder_read_header(decoder_.get()));
        return *this;
    }

    /// <summary>
    /// Reads the JPEG-LS header from the beginning of the JPEG-LS byte stream or after the SPIFF header.
    /// After this function is called frame info and other info can be retrieved.
    /// </summary>
    /// <param name="ec">The out-parameter for error reporting.</param>
    jpegls_decoder& read_header(std::error_code& ec) noexcept
    {
        ec = charls_jpegls_decoder_read_header(decoder_.get());
        return *this;
    }

    /// <summary>
    /// Returns information about the frame stored in the JPEG-LS byte stream.
    /// Function can be called after read_header.
    /// </summary>
    /// <returns>The frame info that describes the image stored in the JPEG-LS byte stream.</returns>
    CHARLS_NO_DISCARD charls::frame_info frame_info() const
    {
        charls::frame_info frame_info;
        check_jpegls_errc(charls_jpegls_decoder_get_frame_info(decoder_.get(), &frame_info));
        return frame_info;
    }

    /// <summary>
    /// Returns the NEAR parameter that was used to encode the scan. A value of 0 means lossless.
    /// </summary>
    /// <param name="component">The component index for which the NEAR parameter should be retrieved.</param>
    /// <returns>The value of the NEAR parameter.</returns>
    CHARLS_NO_DISCARD int32_t near_lossless(int32_t component = 0) const
    {
        int32_t near_lossless;
        check_jpegls_errc(charls_jpegls_decoder_get_near_lossless(decoder_.get(), component, &near_lossless));
        return near_lossless;
    }

    /// <summary>
    /// Returns the interleave mode that was used to encode the scan(s).
    /// </summary>
    /// <returns>The value of the interleave mode.</returns>
    CHARLS_NO_DISCARD charls::interleave_mode interleave_mode() const
    {
        charls::interleave_mode interleave_mode;
        check_jpegls_errc(charls_jpegls_decoder_get_interleave_mode(decoder_.get(), &interleave_mode));
        return interleave_mode;
    }

    /// <summary>
    /// Returns the preset coding parameters used to encode the first scan.
    /// </summary>
    /// <returns>The values of the JPEG-LS preset coding parameters.</returns>
    CHARLS_NO_DISCARD jpegls_pc_parameters preset_coding_parameters() const
    {
        jpegls_pc_parameters preset_coding_parameters;
        check_jpegls_errc(charls_jpegls_decoder_get_preset_coding_parameters(decoder_.get(), 0, &preset_coding_parameters));
        return preset_coding_parameters;
    }

    /// <summary>
    /// Returns the size required for the destination buffer in bytes to hold the decoded pixel data.
    /// Function can be read_header.
    /// </summary>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <returns>The required size in bytes of the destination buffer.</returns>
    CHARLS_NO_DISCARD size_t destination_size(const uint32_t stride = 0) const
    {
        size_t size_in_bytes;
        check_jpegls_errc(charls_jpegls_decoder_get_destination_size(decoder_.get(), stride, &size_in_bytes));
        return size_in_bytes;
    }

    /// <summary>
    /// Will decode the JPEG-LS byte stream set with source into the destination buffer.
    /// </summary>
    /// <param name="destination_buffer">Byte array that holds the encoded bytes when the function returns.</param>
    /// <param name="destination_size_bytes">Length of the array in bytes. If the array is too small the function will return an error.</param>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    void decode(void* destination_buffer, const size_t destination_size_bytes, const uint32_t stride = 0) const
    {
        check_jpegls_errc(charls_jpegls_decoder_decode_to_buffer(decoder_.get(), destination_buffer, destination_size_bytes, stride));
    }

    /// <summary>
    /// Will decode the JPEG-LS byte stream set with source into the destination container.
    /// </summary>
    /// <param name="destination_container">A STL like container that provides the functions data() and size() and the type value_type.</param>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    template<typename Container, typename ValueType = typename Container::value_type>
    void decode(Container& destination_container, const uint32_t stride = 0) const
    {
        decode(destination_container.data(), destination_container.size() * sizeof(ValueType), stride);
    }

    /// <summary>
    /// Will decode the JPEG-LS byte stream set with source and return a container with the decoded data.
    /// </summary>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <returns>Container with the decoded data.</returns>
    template<typename Container, typename ValueType = typename Container::value_type>
    auto decode(const uint32_t stride = 0) const
    {
        Container destination(destination_size() / sizeof(ValueType));

        decode(destination.data(), destination.size() * sizeof(ValueType), stride);
        return destination;
    }

private:
    CHARLS_NO_DISCARD static charls_jpegls_decoder* create_decoder()
    {
        charls_jpegls_decoder* decoder = charls_jpegls_decoder_create();
        if (!decoder)
            throw std::bad_alloc();

        return decoder;
    }

    static void destroy_decoder(const charls_jpegls_decoder* decoder) noexcept
    {
        charls_jpegls_decoder_destroy(decoder);
    }

    std::unique_ptr<charls_jpegls_decoder, void (*)(const charls_jpegls_decoder*)> decoder_{create_decoder(), destroy_decoder};
};


/// <summary>
/// JPEG-LS encoder class that encapsulates the C ABI interface calls and provide a native C++ interface.
/// </summary>
class jpegls_encoder final
{
public:
    /// <summary>
    /// Encoded pixel data in 1 simple operation into a JPEG-LS encoded buffer.
    /// </summary>
    /// <param name="source">Source container with the pixel data bytes that need to be encoded.</param>
    /// <param name="info">Information about the frame that needs to be encoded.</param>
    /// <param name="interleave_mode">Configures the interleave mode the encoder should use.</param>
    /// <returns>Container with the JPEG-LS encoded bytes.</returns>
    template<typename Container, typename ValueType = typename Container::value_type>
    static auto encode(const Container& source, const charls::frame_info& info, const charls::interleave_mode interleave_mode = charls::interleave_mode::none)
    {
        jpegls_encoder encoder;
        encoder.frame_info(info)
            .interleave_mode(interleave_mode);

        Container destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        return destination;
    }

    jpegls_encoder() = default;
    ~jpegls_encoder() = default;

    jpegls_encoder(const jpegls_encoder&) = delete;
    jpegls_encoder(jpegls_encoder&&) noexcept = default;
    jpegls_encoder& operator=(const jpegls_encoder&) = delete;
    jpegls_encoder& operator=(jpegls_encoder&&) noexcept = default;

    /// <summary>
    /// Configures the frame that needs to be encoded.
    /// This information will be written to the Start of Frame (SOF) segment during the encode phase.
    /// </summary>
    /// <param name="frame_info">Information about the frame that needs to be encoded.</param>
    jpegls_encoder& frame_info(const frame_info& frame_info)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_frame_info(encoder_.get(), &frame_info));
        return *this;
    }

    /// <summary>
    /// Configures the NEAR parameter the encoder should use. A value of 0 means lossless, this is also the default.
    /// </summary>
    /// <param name="near_lossless">Value of the NEAR parameter.</param>
    jpegls_encoder& near_lossless(const int32_t near_lossless)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_near_lossless(encoder_.get(), near_lossless));
        return *this;
    }

    /// <summary>
    /// Configures the interleave mode the encoder should use. The default is none.
    /// The encoder expects the input buffer in the same format as the interleave mode.
    /// </summary>
    /// <param name="interleave_mode">Value of the interleave mode.</param>
    jpegls_encoder& interleave_mode(const interleave_mode interleave_mode)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_interleave_mode(encoder_.get(), interleave_mode));
        return *this;
    }

    /// <summary>
    /// Configures the preset coding parameters the encoder should use.
    /// If not set the encoder will use the default preset coding parameters as defined by the JPEG-LS standard.
    /// Only when the coding parameters are different then the default parameters, they will be written to the
    /// JPEG-LS stream during the encode phase.
    /// </summary>
    /// <param name="preset_coding_parameters">Reference to the preset coding parameters.</param>
    jpegls_encoder& preset_coding_parameters(const jpegls_pc_parameters& preset_coding_parameters)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_preset_coding_parameters(encoder_.get(), &preset_coding_parameters));
        return *this;
    }

    /// <summary>
    /// Configures the HP color transformation the encoder should use.
    /// If not set the encoder will use no color transformation.
    /// Color transformations are a HP extension and not defined by the JPEG-LS standard and can only be set for 3 component encodings.
    /// </summary>
    /// <param name="color_transformation">The color transformation parameters.</param>
    jpegls_encoder& color_transformation(const color_transformation color_transformation)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_color_transformation(encoder_.get(), color_transformation));
        return *this;
    }

    /// <summary>
    /// Returns the size in bytes, that the encoder expects are needed to hold the encoded image.
    /// </summary>
    /// <remarks>
    /// Size for dynamic extras like SPIFF entries and other tables are not included in this size.
    /// </remarks>
    /// <returns>The estimated size in bytes needed to hold the encoded image.</returns>
    CHARLS_NO_DISCARD size_t estimated_destination_size() const
    {
        size_t size_in_bytes;
        check_jpegls_errc(charls_jpegls_encoder_get_estimated_destination_size(encoder_.get(), &size_in_bytes));
        return size_in_bytes;
    }

    /// <summary>
    /// Set the reference to the destination buffer that will contain the encoded JPEG-LS byte stream data after encoding.
    /// This buffer needs to remain valid during the encoding process.
    /// </summary>
    /// <param name="destination_buffer">Reference to the start of the destination buffer.</param>
    /// <param name="destination_size_bytes">Size of the destination buffer in bytes.</param>
    jpegls_encoder& destination(void* destination_buffer, const size_t destination_size_bytes)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_destination_buffer(encoder_.get(), destination_buffer, destination_size_bytes));
        return *this;
    }

    /// <summary>
    /// Set the the container that will contain the encoded JPEG-LS byte stream data after encoding.
    /// This container needs to remain valid during the encoding process.
    /// </summary>
    /// <param name="destination_container">The STL like container, that supports the functions data() and size() and the typedef value_type.</param>
    template<typename Container, typename ValueType = typename Container::value_type>
    jpegls_encoder& destination(Container& destination_container)
    {
        return destination(destination_container.data(), destination_container.size() * sizeof(ValueType));
    }

    /// <summary>
    /// Writes a standard SPIFF header to the destination. The additional values are computed from the current encoder settings.
    /// </summary>
    /// <param name="color_space">The color space of the image.</param>
    /// <param name="resolution_units">The resolution units of the next 2 parameters.</param>
    /// <param name="vertical_resolution">The vertical resolution.</param>
    /// <param name="horizontal_resolution">The horizontal resolution.</param>
    jpegls_encoder& write_standard_spiff_header(const spiff_color_space color_space,
                                                const spiff_resolution_units resolution_units = spiff_resolution_units::aspect_ratio,
                                                const uint32_t vertical_resolution = 1,
                                                const uint32_t horizontal_resolution = 1)
    {
        check_jpegls_errc(charls_jpegls_encoder_write_standard_spiff_header(encoder_.get(), color_space, resolution_units, vertical_resolution, horizontal_resolution));
        return *this;
    }

    /// <summary>
    /// Writes a SPIFF header to the destination.
    /// </summary>
    /// <param name="header">Reference to a SPIFF header that will be written to the destination.</param>
    jpegls_encoder& write_spiff_header(const spiff_header& header)
    {
        check_jpegls_errc(charls_jpegls_encoder_write_spiff_header(encoder_.get(), &header));
        return *this;
    }

    /// <summary>
    /// Writes a SPIFF directory entry to the destination.
    /// </summary>
    /// <param name="entry_tag">The entry tag of the directory entry.</param>
    /// <param name="entry_data">The entry data of the directory entry.</param>
    /// <param name="entry_data_size">The size in bytes of the directory entry [0-65528].</param>
    template<typename IntDerivedType>
    jpegls_encoder& write_spiff_entry(const IntDerivedType entry_tag, const void* entry_data, const size_t entry_data_size)
    {
        check_jpegls_errc(charls_jpegls_encoder_write_spiff_entry(encoder_.get(), static_cast<uint32_t>(entry_tag), entry_data, entry_data_size));
        return *this;
    }

    /// <summary>
    /// Encodes the passed buffer with the source image data to the destination.
    /// </summary>
    /// <param name="source_buffer">Byte array that holds the image data that needs to be encoded.</param>
    /// <param name="source_size_bytes">Length of the array in bytes.</param>
    /// <param name="stride">
    /// The number of bytes from one row of pixels in memory to the next row of pixels in memory.
    /// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
    /// </param>
    /// <returns>The number of bytes written to the destination.</returns>
    size_t encode(const void* source_buffer, const size_t source_size_bytes, const uint32_t stride = 0) const
    {
        check_jpegls_errc(charls_jpegls_encoder_encode_from_buffer(encoder_.get(), source_buffer, source_size_bytes, stride));
        return bytes_written();
    }

    /// <summary>
    /// Encodes the passed STL like container with the source image data to the destination.
    /// </summary>
    /// <param name="source_container">Container that holds the image data that needs to be encoded.</param>
    /// <param name="stride">
    /// The number of bytes from one row of pixels in memory to the next row of pixels in memory.
    /// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
    /// </param>
    /// <returns>The number of bytes written to the destination.</returns>
    template<typename Container, typename ValueType = typename Container::value_type>
    size_t encode(const Container& source_container, const uint32_t stride = 0) const
    {
        return encode(source_container.data(), source_container.size() * sizeof(ValueType), stride);
    }

    /// <summary>
    /// Returns the size in bytes, that are written to the destination.
    /// </summary>
    /// <returns>The bytes written.</returns>
    CHARLS_NO_DISCARD size_t bytes_written() const
    {
        size_t bytes_written;
        check_jpegls_errc(charls_jpegls_encoder_get_bytes_written(encoder_.get(), &bytes_written));
        return bytes_written;
    }

private:
    CHARLS_NO_DISCARD static charls_jpegls_encoder* create_encoder()
    {
        charls_jpegls_encoder* encoder = charls_jpegls_encoder_create();
        if (!encoder)
            throw std::bad_alloc();

        return encoder;
    }

    static void destroy_encoder(const charls_jpegls_encoder* encoder) noexcept
    {
        charls_jpegls_encoder_destroy(encoder);
    }

    std::unique_ptr<charls_jpegls_encoder, void (*)(const charls_jpegls_encoder*)> encoder_{create_encoder(), destroy_encoder};
};

} // namespace charls


#endif

// Undefine CHARLS macros to prevent global namespace pollution
#if !defined(CHARLS_LIBRARY_BUILD)
#undef CHARLS_API_IMPORT_EXPORT
#undef CHARLS_NO_DISCARD
#undef CHARLS_ENUM_DEPRECATED
#undef CHARLS_FINAL
#undef CHARLS_NOEXCEPT
#endif
