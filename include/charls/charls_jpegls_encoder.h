// SPDX-FileCopyrightText: © 2020 Team CharLS
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_error.h"

#ifdef __cplusplus
struct charls_jpegls_encoder;
extern "C" {
#else
#include <stddef.h>
typedef struct charls_jpegls_encoder charls_jpegls_encoder;
#endif

// The following functions define the public C API of the CharLS library.
// The C++ API is defined after the C API.

/// <summary>
/// Creates a JPEG-LS encoder instance, when finished with the instance destroy it with the function
/// charls_jpegls_encoder_destroy.
/// </summary>
/// <returns>A reference to a new created encoder instance, or a null pointer when the creation fails.</returns>
CHARLS_CHECK_RETURN CHARLS_RET_MAY_BE_NULL CHARLS_API_IMPORT_EXPORT charls_jpegls_encoder*
    CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_create(CHARLS_C_VOID) CHARLS_NOEXCEPT;

/// <summary>
/// Destroys a JPEG-LS encoder instance created with charls_jpegls_encoder_create and releases all internal resources
/// attached to it.
/// </summary>
/// <param name="encoder">Instance to destroy. If a null pointer is passed as argument, no action occurs.</param>
CHARLS_API_IMPORT_EXPORT void CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_destroy(CHARLS_IN_OPT const charls_jpegls_encoder* encoder) CHARLS_NOEXCEPT;

/// <summary>
/// Configures the frame that needs to be encoded. This information will be written to the Start of Frame segment.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="frame_info">Information about the frame that needs to be encoded.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_frame_info(CHARLS_IN charls_jpegls_encoder* encoder,
                                     CHARLS_IN const charls_frame_info* frame_info) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the NEAR parameter the encoder should use. A value of 0 means lossless, 0 is also the default.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="near_lossless">Value of the NEAR parameter.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_near_lossless(CHARLS_IN charls_jpegls_encoder* encoder, int32_t near_lossless) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the encoding options the encoder should use. Default is charls_encoding_options::include_pc_parameters_jai
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="encoding_options">Options to use.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_encoding_options(CHARLS_IN charls_jpegls_encoder* encoder,
                                           charls_encoding_options encoding_options) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the interleave mode the encoder should use. The default is none.
/// The encoder expects the input buffer in the same format as the interleave mode.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="interleave_mode">Value of the interleave mode.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_interleave_mode(CHARLS_IN charls_jpegls_encoder* encoder,
                                          charls_interleave_mode interleave_mode) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the preset coding parameters the encoder should use.
/// If not set the encoder will use the default preset coding parameters as defined by the JPEG-LS standard.
/// Only when the coding parameters are different from the default parameters or when `include_pc_parameters_jai` is set,
/// they will be written to the JPEG-LS stream during the encode phase.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="preset_coding_parameters">Reference to the preset coding parameters.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_preset_coding_parameters(CHARLS_IN charls_jpegls_encoder* encoder,
                                                   CHARLS_IN const charls_jpegls_pc_parameters* preset_coding_parameters)
    CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the HP color transformation the encoder should use.
/// If not set the encoder will use no color transformation.
/// Color transformations are an HP extension and not defined by the JPEG-LS standard and can only be set for 3 component
/// encodings.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="color_transformation">The color transformation parameters.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_color_transformation(CHARLS_IN charls_jpegls_encoder* encoder,
                                               charls_color_transformation color_transformation) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the mapping table ID the encoder should reference when encoding a component.
/// The referenced mapping table can be included in the stream or provided in another JPEG-LS abbreviated format stream.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="component_index">Index of the component. Component 0 is the start index.</param>
/// <param name="table_id">Mapping table ID that will be referenced by this component.</param>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_mapping_table_id(CHARLS_IN charls_jpegls_encoder* encoder, int32_t component_index,
                                           int32_t table_id) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the size in bytes, that the encoder expects are needed to hold the encoded image.
/// </summary>
/// <remarks>
/// Size for dynamic extras like SPIFF entries and other tables are not included in this size.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="size_in_bytes">Reference to the size that will be set when the function returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_estimated_destination_size(CHARLS_IN const charls_jpegls_encoder* encoder,
                                                     CHARLS_OUT size_t* size_in_bytes) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Set the reference to the destination buffer that will contain the encoded JPEG-LS byte stream data after encoding.
/// This buffer needs to remain valid during the encoding process.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="destination_buffer">Reference to the start of the destination buffer.</param>
/// <param name="destination_size_bytes">Size of the destination buffer in bytes.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(write_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_destination_buffer(CHARLS_IN charls_jpegls_encoder* encoder,
                                             CHARLS_OUT_WRITES_BYTES(destination_size_bytes) void* destination_buffer,
                                             size_t destination_size_bytes) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Writes a standard SPIFF header to the destination. The additional values are computed from the current encoder settings.
/// A SPIFF header is optional, but recommended for standalone JPEG-LS files.
/// The encoder will not validate the passed SPIFF header.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="color_space">The color space of the image.</param>
/// <param name="resolution_units">The resolution units of the next 2 parameters.</param>
/// <param name="vertical_resolution">The vertical resolution.</param>
/// <param name="horizontal_resolution">The horizontal resolution.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_standard_spiff_header(CHARLS_IN charls_jpegls_encoder* encoder,
                                                  charls_spiff_color_space color_space,
                                                  charls_spiff_resolution_units resolution_units,
                                                  uint32_t vertical_resolution,
                                                  uint32_t horizontal_resolution) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Writes a SPIFF header to the destination.
/// The encoder will not validate the passed SPIFF header.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="spiff_header">Reference to a SPIFF header that will be written to the destination.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_header(CHARLS_IN charls_jpegls_encoder* encoder,
                                         CHARLS_IN const charls_spiff_header* spiff_header) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Writes a SPIFF directory entry to the destination.
/// </summary>
/// <remarks>
/// Function should be called after writing a SPIFF header.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="entry_tag">The entry tag of the directory entry.</param>
/// <param name="entry_data">The data of the directory entry.</param>
/// <param name="entry_data_size_bytes">The size in bytes of the entry data [0-65528].</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 3, 4)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_entry(CHARLS_IN charls_jpegls_encoder* encoder, uint32_t entry_tag,
                                        CHARLS_IN_READS_BYTES(entry_data_size_bytes) const void* entry_data,
                                        size_t entry_data_size_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Writes a SPIFF end of directory entry to the destination.
/// The encoder will normally do this automatically. It is made available
/// for the scenario to create SPIFF headers in front of existing JPEG-LS streams.
/// </summary>
/// <remarks>
/// The end of directory also includes a SOI marker. This marker should be skipped from the JPEG-LS stream.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_end_of_directory_entry(CHARLS_IN charls_jpegls_encoder* encoder) CHARLS_NOEXCEPT;

/// <summary>
/// Writes a comment (COM) segment to the destination.
/// </summary>
/// <remarks>
/// Function should be called before encoding the image data.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="comment">The 'comment' bytes. Application specific, usually a human-readable string.</param>
/// <param name="comment_size_bytes">The size in bytes of the comment [0-65533].</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_comment(CHARLS_IN charls_jpegls_encoder* encoder,
                                    CHARLS_IN_READS_BYTES(comment_size_bytes) const void* comment,
                                    size_t comment_size_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Writes an application data (APPn) segment to the destination.
/// </summary>
/// <remarks>
/// Function should be called before encoding the image data.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="application_data_id">The ID of the application data segment in the range [0..15].</param>
/// <param name="application_data">The 'application data' bytes. Application specific.</param>
/// <param name="application_data_size_bytes">The size in bytes of the comment [0-65533].</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 3, 4)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_application_data(CHARLS_IN charls_jpegls_encoder* encoder, int32_t application_data_id,
                                             CHARLS_IN_READS_BYTES(application_data_size_bytes) const void* application_data,
                                             size_t application_data_size_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Writes a mapping table to the destination.
/// During decoding a component can reference a mapping table.
/// </summary>
/// <remarks>
/// No validation is performed if the table ID is unique and if the table size matches the required size.
/// During decoding the active maximum value determines the required size of the table.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="table_id">Mapping table ID. Unique identifier of the mapping table in the range [1..255]</param>
/// <param name="entry_size">Size in bytes of a single table entry.</param>
/// <param name="table_data">Byte array that holds the mapping table.</param>
/// <param name="table_data_size_bytes">The size in bytes of the table data.</param>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 4, 5)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_mapping_table(CHARLS_IN charls_jpegls_encoder* encoder, int32_t table_id, int32_t entry_size,
                                          CHARLS_IN_READS_BYTES(table_data_size_bytes) const void* table_data,
                                          size_t table_data_size_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Encodes the passed buffer with the source image data to the destination.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="source_buffer">Byte array that holds the image data that needs to be encoded.</param>
/// <param name="source_size_bytes">Length of the array in bytes.</param>
/// <param name="stride">
/// The number of bytes from one row of pixels in memory to the next row of pixels in memory.
/// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
/// </param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_encode_from_buffer(CHARLS_IN charls_jpegls_encoder* encoder,
                                         CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer,
                                         size_t source_size_bytes, uint32_t stride) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Encodes the passed buffer with the source image data to the destination.
/// This is an advanced method that provides more control how image data is encoded in JPEG-LS scans.
/// It should be called until all components are encoded.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="source_buffer">Byte array that holds the image data that needs to be encoded.</param>
/// <param name="source_size_bytes">Length of the array in bytes.</param>
/// <param name="source_component_count">The number of components present in the input source.</param>
/// <param name="stride">
/// The number of bytes from one row of pixels in memory to the next row of pixels in memory.
/// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
/// </param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_encode_components_from_buffer(CHARLS_IN charls_jpegls_encoder* encoder,
                                                    CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer,
                                                    size_t source_size_bytes, int32_t source_component_count,
                                                    uint32_t stride) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Creates a JPEG-LS stream in the abbreviated format that only contain mapping tables (See JPEG-LS standard, C.4).
/// These mapping tables must have been written to the stream first with the method
/// charls_jpegls_encoder_write_mapping_table.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_create_abbreviated_format(CHARLS_IN charls_jpegls_encoder* encoder) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the size in bytes, that are written to the destination.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="bytes_written">Reference to the size that will be set when the function returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_bytes_written(CHARLS_IN const charls_jpegls_encoder* encoder,
                                        CHARLS_OUT size_t* bytes_written) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Resets the write position of the destination buffer to the beginning.
/// All explicit configured options and settings will not be changed.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_rewind(CHARLS_IN charls_jpegls_encoder* encoder) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

#ifdef __cplusplus
} // extern "C"
#endif
