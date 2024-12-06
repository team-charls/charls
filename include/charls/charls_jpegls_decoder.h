// SPDX-FileCopyrightText: © 2020 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_error.h"

#ifdef __cplusplus
struct charls_jpegls_decoder;
extern "C" {
#else
#include <stddef.h>
typedef struct charls_jpegls_decoder charls_jpegls_decoder;
#endif

// The following functions define the public C API of the CharLS library.
// The C++ API is defined after the C API.

/// <summary>
/// Creates a JPEG-LS decoder instance, when finished with the instance destroy it with the function
/// charls_jpegls_decoder_destroy.
/// </summary>
/// <returns>A reference to a new created decoder instance, or a null pointer when the creation fails.</returns>
CHARLS_CHECK_RETURN CHARLS_RET_MAY_BE_NULL CHARLS_API_IMPORT_EXPORT charls_jpegls_decoder*
    CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_create(CHARLS_C_VOID) CHARLS_NOEXCEPT;

/// <summary>
/// Destroys a JPEG-LS decoder instance created with charls_jpegls_decoder_create and releases all internal resources
/// attached to it.
/// </summary>
/// <param name="decoder">Instance to destroy. If a null pointer is passed as argument, no action occurs.</param>
CHARLS_API_IMPORT_EXPORT void CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_destroy(CHARLS_IN_OPT const charls_jpegls_decoder* decoder) CHARLS_NOEXCEPT;

/// <summary>
/// Set the reference to a source buffer that contains the encoded JPEG-LS byte stream data.
/// This buffer needs to remain valid until the buffer is fully decoded.
/// </summary>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="source_buffer">Reference to the start of the source buffer.</param>
/// <param name="source_size_bytes">Size of the source buffer in bytes.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_set_source_buffer(CHARLS_IN charls_jpegls_decoder* decoder,
                                        CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer,
                                        size_t source_size_bytes) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Tries to read the SPIFF header from the source buffer.
/// If a SPIFF header exists its content will be put into the spiff_header parameter and header_found will be set to 1.
/// Call charls_jpegls_decoder_read_header to read the normal JPEG header afterward.
/// </summary>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="spiff_header">Output argument, will hold the SPIFF header when one could be found.</param>
/// <param name="header_found">Output argument, will hold 1 if a SPIFF header could be found, otherwise 0.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_spiff_header(CHARLS_IN charls_jpegls_decoder* decoder,
                                        CHARLS_OUT charls_spiff_header* spiff_header,
                                        CHARLS_OUT int32_t* header_found) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Reads the JPEG-LS header from the JPEG byte stream. After this function is called frame info can be retrieved.
/// </summary>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_header(CHARLS_IN charls_jpegls_decoder* decoder) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns information about the frame stored in the JPEG-LS byte stream.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="frame_info">Output argument, will hold the frame info when the function returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_frame_info(CHARLS_IN const charls_jpegls_decoder* decoder,
                                     CHARLS_OUT charls_frame_info* frame_info) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the NEAR parameter that was used to encode the scan. A value of 0 means lossless.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="component_index">The component index for which the NEAR parameter should be retrieved.</param>
/// <param name="near_lossless">Reference that will hold the value of the NEAR parameter.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_near_lossless(CHARLS_IN const charls_jpegls_decoder* decoder, int32_t component_index,
                                        CHARLS_OUT int32_t* near_lossless) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the interleave mode that was used to encode the scan.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="component_index">The component index for which the interleave mode should be retrieved.</param>
/// <param name="interleave_mode">Reference that will hold the value of the interleave mode.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_interleave_mode(CHARLS_IN const charls_jpegls_decoder* decoder, int32_t component_index,
                                          CHARLS_OUT charls_interleave_mode* interleave_mode) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

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
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_preset_coding_parameters(CHARLS_IN const charls_jpegls_decoder* decoder, int32_t reserved,
                                                   CHARLS_OUT charls_jpegls_pc_parameters* preset_coding_parameters)
    CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the color transformation that was used to encode the scan.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="color_transformation">Reference that will hold the value of the color transformation.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_color_transformation(CHARLS_IN const charls_jpegls_decoder* decoder,
                                               CHARLS_OUT charls_color_transformation* color_transformation) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

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
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_destination_size(CHARLS_IN const charls_jpegls_decoder* decoder, uint32_t stride,
                                           CHARLS_OUT size_t* destination_size_bytes) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Will decode the JPEG-LS byte stream from the source buffer into the destination buffer.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="destination_buffer">Byte array that holds the encoded bytes when the function returns.</param>
/// <param name="destination_size_bytes">
/// Length of the array in bytes. If the array is too small the function will return an error.
/// </param>
/// <param name="stride">
/// Number of bytes to the next line in the buffer, when zero, decoder will compute it.
/// </param> <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(write_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_decode_to_buffer(CHARLS_IN charls_jpegls_decoder* decoder,
                                       CHARLS_OUT_WRITES_BYTES(destination_size_bytes) void* destination_buffer,
                                       size_t destination_size_bytes, uint32_t stride) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Will install a function that will be called when a comment (COM) segment is found.
/// </summary>
/// <remarks>
/// Pass NULL or nullptr to uninstall the callback function.
/// The callback should return 0 if there are no errors.
/// It can return a non-zero value to abort decoding with a callback_failed error code.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="handler">Function pointer to the callback function.</param>
/// <param name="user_context">Free to use context data that will be provided to the callback function.</param>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_at_comment(CHARLS_IN charls_jpegls_decoder* decoder, charls_at_comment_handler handler,
                                 void* user_context) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull(1)));


/// <summary>
/// Will install a function that will be called when an application data (APPn) segment is found.
/// </summary>
/// <remarks>
/// Pass NULL or nullptr to uninstall the callback function.
/// The callback should return 0 if there are no errors.
/// It can return a non-zero value to abort decoding with a callback_failed error code.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="handler">Function pointer to the callback function.</param>
/// <param name="user_context">Free to use context data that will be provided to the callback function.</param>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_at_application_data(CHARLS_IN charls_jpegls_decoder* decoder,
                                          charls_at_application_data_handler handler, void* user_context) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull(1)));


/// <summary>
/// Returns the compressed data format of the JPEG-LS data stream.
/// </summary>
/// <remarks>
/// Function can be called after reading the header or after processing the complete JPEG-LS stream.
/// After reading the header the method may report unknown or abbreviated_table_specification.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="compressed_data_format">Current .</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_decoder_get_compressed_data_format(CHARLS_IN const charls_jpegls_decoder* decoder,
                                          CHARLS_OUT charls_compressed_data_format* compressed_data_format) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the mapping table ID referenced by the component or 0 when no mapping table is used.
/// </summary>
/// <remarks>
/// Function should be called after processing the complete JPEG-LS stream.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="component_index">Index of the component.</param>
/// <param name="table_id">Output argument, will hold the mapping table ID when the function returns or 0.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_decoder_get_mapping_table_id(CHARLS_IN const charls_jpegls_decoder* decoder, int32_t component_index,
                                    CHARLS_OUT int32_t* table_id) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Converts the mapping table ID to a mapping table index.
/// When the requested table is not present in the JPEG-LS stream the value -1 will be returned.
/// </summary>
/// <remarks>
/// Function should be called after processing the complete JPEG-LS stream.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="mapping_table_id">Mapping table ID to lookup.</param>
/// <param name="index">Output argument, will hold the mapping table index or -1 when the function returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_decoder_find_mapping_table_index(CHARLS_IN const charls_jpegls_decoder* decoder, int32_t mapping_table_id,
                                        CHARLS_OUT int32_t* index) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the count of mapping tables present in the JPEG-LS stream.
/// </summary>
/// <remarks>
/// Function should be called after processing the complete JPEG-LS stream.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="count">Output argument, will hold the mapping table count when the function returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_decoder_get_mapping_table_count(CHARLS_IN const charls_jpegls_decoder* decoder,
                                       CHARLS_OUT int32_t* count) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns information about a mapping table.
/// </summary>
/// <remarks>
/// Function should be called after processing the complete JPEG-LS stream.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="mapping_table_index">Index of the requested mapping table.</param>
/// <param name="mapping_table_info">
/// Output argument, will hold the mapping table information when the function returns.
/// </param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_decoder_get_mapping_table_info(CHARLS_IN const charls_jpegls_decoder* decoder, int32_t mapping_table_index,
                                      CHARLS_OUT charls_mapping_table_info* mapping_table_info) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns a mapping table.
/// </summary>
/// <remarks>
/// Function should be called after processing the complete JPEG-LS stream.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="mapping_table_index">Index of the requested mapping table.</param>
/// <param name="mapping_table_data">
/// Output argument, will hold the data of the mapping table when the function returns.
/// </param>
/// <param name="mapping_table_size_bytes">Length of the mapping table buffer in bytes.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(write_only, 3, 4)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_decoder_get_mapping_table_data(CHARLS_IN const charls_jpegls_decoder* decoder, int32_t mapping_table_index,
                                      CHARLS_OUT_WRITES_BYTES(mapping_table_size_bytes) void* mapping_table_data,
                                      size_t mapping_table_size_bytes) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

#ifdef __cplusplus

} // extern "C"

#endif
