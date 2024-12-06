// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "annotations.h"
#include "api_abi.h"

#ifdef __cplusplus

#ifndef CHARLS_BUILD_AS_CPP_MODULE
#include <cstddef>
#include <cstdint>
#include <system_error>
#endif

namespace charls::impl {

#else
#include <stddef.h>
#include <stdint.h>
#endif

// The following enum values are for C applications, for C++ the enums are defined after these definitions.
// For the documentation, see the C++ enums definitions.

CHARLS_RETURN_TYPE_SUCCESS(return == 0)
enum charls_jpegls_errc
{
    CHARLS_JPEGLS_ERRC_SUCCESS = 0,

    // Runtime errors:

    CHARLS_JPEGLS_ERRC_NOT_ENOUGH_MEMORY = 1,
    CHARLS_JPEGLS_ERRC_CALLBACK_FAILED = 2,
    CHARLS_JPEGLS_ERRC_DESTINATION_TOO_SMALL = 3,
    CHARLS_JPEGLS_ERRC_NEED_MORE_DATA = 4,
    CHARLS_JPEGLS_ERRC_INVALID_DATA = 5,
    CHARLS_JPEGLS_ERRC_ENCODING_NOT_SUPPORTED = 6,
    CHARLS_JPEGLS_ERRC_PARAMETER_VALUE_NOT_SUPPORTED = 7,
    CHARLS_JPEGLS_ERRC_COLOR_TRANSFORM_NOT_SUPPORTED = 8,
    CHARLS_JPEGLS_ERRC_JPEGLS_PRESET_EXTENDED_PARAMETER_TYPE_NOT_SUPPORTED = 9,
    CHARLS_JPEGLS_ERRC_JPEG_MARKER_START_BYTE_NOT_FOUND = 10,
    CHARLS_JPEGLS_ERRC_START_OF_IMAGE_MARKER_NOT_FOUND = 11,
    CHARLS_JPEGLS_ERRC_INVALID_SPIFF_HEADER = 12,
    CHARLS_JPEGLS_ERRC_UNKNOWN_JPEG_MARKER_FOUND = 13,
    CHARLS_JPEGLS_ERRC_UNEXPECTED_START_OF_SCAN_MARKER = 14,
    CHARLS_JPEGLS_ERRC_INVALID_MARKER_SEGMENT_SIZE = 15,
    CHARLS_JPEGLS_ERRC_DUPLICATE_START_OF_IMAGE_MARKER = 16,
    CHARLS_JPEGLS_ERRC_DUPLICATE_START_OF_FRAME_MARKER = 17,
    CHARLS_JPEGLS_ERRC_DUPLICATE_COMPONENT_ID_IN_SOF_SEGMENT = 18,
    CHARLS_JPEGLS_ERRC_UNEXPECTED_END_OF_IMAGE_MARKER = 19,
    CHARLS_JPEGLS_ERRC_INVALID_JPEGLS_PRESET_PARAMETER_TYPE = 20,
    CHARLS_JPEGLS_ERRC_MISSING_END_OF_SPIFF_DIRECTORY = 21,
    CHARLS_JPEGLS_ERRC_UNEXPECTED_RESTART_MARKER = 22,
    CHARLS_JPEGLS_ERRC_RESTART_MARKER_NOT_FOUND = 23,
    CHARLS_JPEGLS_ERRC_END_OF_IMAGE_MARKER_NOT_FOUND = 24,
    CHARLS_JPEGLS_ERRC_UNEXPECTED_DEFINE_NUMBER_OF_LINES_MARKER = 25,
    CHARLS_JPEGLS_ERRC_DEFINE_NUMBER_OF_LINES_MARKER_NOT_FOUND = 26,
    CHARLS_JPEGLS_ERRC_UNKNOWN_COMPONENT_ID = 27,
    CHARLS_JPEGLS_ERRC_ABBREVIATED_FORMAT_AND_SPIFF_HEADER_MISMATCH = 28,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_WIDTH = 29,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_HEIGHT = 30,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_BITS_PER_SAMPLE = 31,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_COMPONENT_COUNT = 32,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_INTERLEAVE_MODE = 33,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_NEAR_LOSSLESS = 34,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_JPEGLS_PRESET_PARAMETERS = 35,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_COLOR_TRANSFORMATION = 36,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_MAPPING_TABLE_ID = 37,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_MAPPING_TABLE_CONTINUATION = 38,

    // Logic errors:

    CHARLS_JPEGLS_ERRC_INVALID_OPERATION = 100,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT = 101,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_WIDTH = 102,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_HEIGHT = 103,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_BITS_PER_SAMPLE = 104,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_COMPONENT_COUNT = 105,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_INTERLEAVE_MODE = 106,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_NEAR_LOSSLESS = 107,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_JPEGLS_PC_PARAMETERS = 108,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_COLOR_TRANSFORMATION = 109,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_SIZE = 110,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_STRIDE = 111,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_ENCODING_OPTIONS = 112,
};

enum charls_interleave_mode
{
    CHARLS_INTERLEAVE_MODE_NONE = 0,
    CHARLS_INTERLEAVE_MODE_LINE = 1,
    CHARLS_INTERLEAVE_MODE_SAMPLE = 2
};

enum charls_compressed_data_format
{
    CHARLS_COMPRESSED_DATA_FORMAT_UNKNOWN = 0,
    CHARLS_COMPRESSED_DATA_FORMAT_INTERCHANGE = 1,
    CHARLS_COMPRESSED_DATA_FORMAT_ABBREVIATED_IMAGE_DATA = 2,
    CHARLS_COMPRESSED_DATA_FORMAT_ABBREVIATED_TABLE_SPECIFICATION = 3
};

enum charls_encoding_options
{
    CHARLS_ENCODING_OPTIONS_NONE = 0,
    CHARLS_ENCODING_OPTIONS_EVEN_DESTINATION_SIZE = 1,
    CHARLS_ENCODING_OPTIONS_INCLUDE_VERSION_NUMBER = 2,
    CHARLS_ENCODING_OPTIONS_INCLUDE_PC_PARAMETERS_JAI = 4
};

enum charls_color_transformation
{
    CHARLS_COLOR_TRANSFORMATION_NONE = 0,
    CHARLS_COLOR_TRANSFORMATION_HP1 = 1,
    CHARLS_COLOR_TRANSFORMATION_HP2 = 2,
    CHARLS_COLOR_TRANSFORMATION_HP3 = 3
};

enum charls_spiff_profile_id
{
    CHARLS_SPIFF_PROFILE_ID_NONE = 0,
    CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_BASE = 1,
    CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_PROGRESSIVE = 2,
    CHARLS_SPIFF_PROFILE_ID_BI_LEVEL_FACSIMILE = 3,
    CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_FACSIMILE = 4
};

enum charls_spiff_color_space
{
    CHARLS_SPIFF_COLOR_SPACE_BI_LEVEL_BLACK = 0,
    CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_709_VIDEO = 1,
    CHARLS_SPIFF_COLOR_SPACE_NONE = 2,
    CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_601_1_RGB = 3,
    CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_601_1_VIDEO = 4,
    CHARLS_SPIFF_COLOR_SPACE_GRAYSCALE = 8,
    CHARLS_SPIFF_COLOR_SPACE_PHOTO_YCC = 9,
    CHARLS_SPIFF_COLOR_SPACE_RGB = 10,
    CHARLS_SPIFF_COLOR_SPACE_CMY = 11,
    CHARLS_SPIFF_COLOR_SPACE_CMYK = 12,
    CHARLS_SPIFF_COLOR_SPACE_YCCK = 13,
    CHARLS_SPIFF_COLOR_SPACE_CIE_LAB = 14,
    CHARLS_SPIFF_COLOR_SPACE_BI_LEVEL_WHITE = 15
};

enum charls_spiff_compression_type
{
    CHARLS_SPIFF_COMPRESSION_TYPE_UNCOMPRESSED = 0,
    CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_HUFFMAN = 1,
    CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_READ = 2,
    CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_MODIFIED_READ = 3,
    CHARLS_SPIFF_COMPRESSION_TYPE_JBIG = 4,
    CHARLS_SPIFF_COMPRESSION_TYPE_JPEG = 5,
    CHARLS_SPIFF_COMPRESSION_TYPE_JPEG_LS = 6
};

enum charls_spiff_resolution_units
{
    CHARLS_SPIFF_RESOLUTION_UNITS_ASPECT_RATIO = 0,
    CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_INCH = 1,
    CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_CENTIMETER = 2
};

enum charls_spiff_entry_tag
{
    CHARLS_SPIFF_ENTRY_TAG_TRANSFER_CHARACTERISTICS = 2,
    CHARLS_SPIFF_ENTRY_TAG_COMPONENT_REGISTRATION = 3,
    CHARLS_SPIFF_ENTRY_TAG_IMAGE_ORIENTATION = 4,
    CHARLS_SPIFF_ENTRY_TAG_THUMBNAIL = 5,
    CHARLS_SPIFF_ENTRY_TAG_IMAGE_TITLE = 6,
    CHARLS_SPIFF_ENTRY_TAG_IMAGE_DESCRIPTION = 7,
    CHARLS_SPIFF_ENTRY_TAG_TIME_STAMP = 8,
    CHARLS_SPIFF_ENTRY_TAG_VERSION_IDENTIFIER = 9,
    CHARLS_SPIFF_ENTRY_TAG_CREATOR_IDENTIFICATION = 10,
    CHARLS_SPIFF_ENTRY_TAG_PROTECTION_INDICATOR = 11,
    CHARLS_SPIFF_ENTRY_TAG_COPYRIGHT_INFORMATION = 12,
    CHARLS_SPIFF_ENTRY_TAG_CONTACT_INFORMATION = 13,
    CHARLS_SPIFF_ENTRY_TAG_TILE_INDEX = 14,
    CHARLS_SPIFF_ENTRY_TAG_SCAN_INDEX = 15,
    CHARLS_SPIFF_ENTRY_TAG_SET_REFERENCE = 16
};

enum charls_constants
{
    CHARLS_MAPPING_TABLE_MISSING = -1
};

#ifdef __cplusplus
} // namespace charls::impl

CHARLS_EXPORT
namespace charls {

/// <summary>
/// Defines the result values that are returned by the CharLS API functions.
/// </summary>
CHARLS_RETURN_TYPE_SUCCESS(return == 0)
enum class [[nodiscard]] jpegls_errc
{
    /// <summary>
    /// The operation completed without errors.
    /// </summary>
    success = impl::CHARLS_JPEGLS_ERRC_SUCCESS,

    // Runtime errors:

    /// <summary>
    /// This error is returned when the implementation could not allocate memory for its internal buffers.
    /// </summary>
    not_enough_memory = impl::CHARLS_JPEGLS_ERRC_NOT_ENOUGH_MEMORY,

    /// <summary>
    /// This error is returned when a callback function returns a nonzero value.
    /// </summary>
    callback_failed = impl::CHARLS_JPEGLS_ERRC_CALLBACK_FAILED,

    /// <summary>
    /// The destination buffer is too small to hold all the output.
    /// </summary>
    destination_too_small = impl::CHARLS_JPEGLS_ERRC_DESTINATION_TOO_SMALL,

    /// <summary>
    /// The source buffer is too small, more input data was expected.
    /// </summary>
    need_more_data = impl::CHARLS_JPEGLS_ERRC_NEED_MORE_DATA,

    /// <summary>
    /// This error is returned when the encoded bit stream contains a general structural problem.
    /// </summary>
    invalid_data = impl::CHARLS_JPEGLS_ERRC_INVALID_DATA,

    /// <summary>
    /// This error is returned when an encoded frame is found that is not encoded with the JPEG-LS algorithm.
    /// </summary>
    encoding_not_supported = impl::CHARLS_JPEGLS_ERRC_ENCODING_NOT_SUPPORTED,

    /// <summary>
    /// This error is returned when the JPEG stream contains a parameter value that is not supported by this implementation.
    /// </summary>
    parameter_value_not_supported = impl::CHARLS_JPEGLS_ERRC_PARAMETER_VALUE_NOT_SUPPORTED,

    /// <summary>
    /// The color transform is not supported.
    /// </summary>
    color_transform_not_supported = impl::CHARLS_JPEGLS_ERRC_COLOR_TRANSFORM_NOT_SUPPORTED,

    /// <summary>
    /// This error is returned when the stream contains an unsupported type parameter in the JPEG-LS segment.
    /// </summary>
    jpegls_preset_extended_parameter_type_not_supported =
        impl::CHARLS_JPEGLS_ERRC_JPEGLS_PRESET_EXTENDED_PARAMETER_TYPE_NOT_SUPPORTED,

    /// <summary>
    /// This error is returned when the algorithm expect a 0xFF code (indicates start of a JPEG marker) but none was found.
    /// </summary>
    jpeg_marker_start_byte_not_found = impl::CHARLS_JPEGLS_ERRC_JPEG_MARKER_START_BYTE_NOT_FOUND,

    /// <summary>
    /// This error is returned when the first JPEG marker is not the SOI marker.
    /// </summary>
    start_of_image_marker_not_found = impl::CHARLS_JPEGLS_ERRC_START_OF_IMAGE_MARKER_NOT_FOUND,

    /// <summary>
    /// This error is returned when the SPIFF header is invalid.
    /// </summary>
    invalid_spiff_header = impl::CHARLS_JPEGLS_ERRC_INVALID_SPIFF_HEADER,

    /// <summary>
    /// This error is returned when an unknown JPEG marker code is found in the encoded bit stream.
    /// </summary>
    unknown_jpeg_marker_found = impl::CHARLS_JPEGLS_ERRC_UNKNOWN_JPEG_MARKER_FOUND,

    /// <summary>
    /// This error is returned when the stream contains an unexpected SOS marker.
    /// </summary>
    unexpected_start_of_scan_marker = impl::CHARLS_JPEGLS_ERRC_UNEXPECTED_START_OF_SCAN_MARKER,

    /// <summary>
    /// This error is returned when the segment size of a marker segment is invalid.
    /// </summary>
    invalid_marker_segment_size = impl::CHARLS_JPEGLS_ERRC_INVALID_MARKER_SEGMENT_SIZE,

    /// <summary>
    /// This error is returned when the stream contains more than one SOI marker.
    /// </summary>
    duplicate_start_of_image_marker = impl::CHARLS_JPEGLS_ERRC_DUPLICATE_START_OF_IMAGE_MARKER,

    /// <summary>
    /// This error is returned when the stream contains more than one SOF marker.
    /// </summary>
    duplicate_start_of_frame_marker = impl::CHARLS_JPEGLS_ERRC_DUPLICATE_START_OF_FRAME_MARKER,

    /// <summary>
    /// This error is returned when the stream contains duplicate component identifiers in the SOF segment.
    /// </summary>
    duplicate_component_id_in_sof_segment = impl::CHARLS_JPEGLS_ERRC_DUPLICATE_COMPONENT_ID_IN_SOF_SEGMENT,

    /// <summary>
    /// This error is returned when the stream contains an unexpected EOI marker.
    /// </summary>
    unexpected_end_of_image_marker = impl::CHARLS_JPEGLS_ERRC_UNEXPECTED_END_OF_IMAGE_MARKER,

    /// <summary>
    /// This error is returned when the stream contains an invalid type parameter in the JPEG-LS segment.
    /// </summary>
    invalid_jpegls_preset_parameter_type = impl::CHARLS_JPEGLS_ERRC_INVALID_JPEGLS_PRESET_PARAMETER_TYPE,

    /// <summary>
    /// This error is returned when the stream contains a SPIFF header but not an SPIFF end-of-directory entry.
    /// </summary>
    missing_end_of_spiff_directory = impl::CHARLS_JPEGLS_ERRC_MISSING_END_OF_SPIFF_DIRECTORY,

    /// <summary>
    /// This error is returned when a restart marker is found outside the encoded entropy data.
    /// </summary>
    unexpected_restart_marker = impl::CHARLS_JPEGLS_ERRC_UNEXPECTED_RESTART_MARKER,

    /// <summary>
    /// This error is returned when an expected restart marker is not found. It may indicate data corruption in the JPEG-LS
    /// byte stream.
    /// </summary>
    restart_marker_not_found = impl::CHARLS_JPEGLS_ERRC_RESTART_MARKER_NOT_FOUND,

    /// <summary>
    /// This error is returned when the End of Image (EOI) marker could not be found.
    /// </summary>
    end_of_image_marker_not_found = impl::CHARLS_JPEGLS_ERRC_END_OF_IMAGE_MARKER_NOT_FOUND,

    /// <summary>
    /// This error is returned when the stream contains an unexpected DefineNumberOfLines (DNL) marker.
    /// </summary>
    unexpected_define_number_of_lines_marker = impl::CHARLS_JPEGLS_ERRC_UNEXPECTED_DEFINE_NUMBER_OF_LINES_MARKER,

    /// <summary>
    /// This error is returned when the DefineNumberOfLines (DNL) marker could not be found.
    /// </summary>
    define_number_of_lines_marker_not_found = impl::CHARLS_JPEGLS_ERRC_DEFINE_NUMBER_OF_LINES_MARKER_NOT_FOUND,

    /// <summary>
    /// This error is returned when an unknown component ID in a scan is detected.
    /// </summary>
    unknown_component_id = impl::CHARLS_JPEGLS_ERRC_UNKNOWN_COMPONENT_ID,

    /// <summary>
    /// This error is returned for stream with only mapping tables and a spiff header.
    /// </summary>
    abbreviated_format_and_spiff_header_mismatch = impl::CHARLS_JPEGLS_ERRC_ABBREVIATED_FORMAT_AND_SPIFF_HEADER_MISMATCH,

    /// <summary>
    /// This error is returned when the stream contains a width parameter defined more than once or in an incompatible way.
    /// </summary>
    invalid_parameter_width = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_WIDTH,

    /// <summary>
    /// This error is returned when the stream contains a height parameter defined more than once in an incompatible way.
    /// </summary>
    invalid_parameter_height = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_HEIGHT,

    /// <summary>
    /// This error is returned when the stream contains a bits per sample (sample precision) parameter outside the range
    /// [2,16]
    /// </summary>
    invalid_parameter_bits_per_sample = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_BITS_PER_SAMPLE,

    /// <summary>
    /// This error is returned when the stream contains a component count parameter outside the range [1,255] for SOF or
    /// [1,4] for SOS.
    /// </summary>
    invalid_parameter_component_count = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_COMPONENT_COUNT,

    /// <summary>
    /// This error is returned when the stream contains an interleave mode (ILV) parameter outside the range [0, 2]
    /// </summary>
    invalid_parameter_interleave_mode = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_INTERLEAVE_MODE,

    /// <summary>
    /// This error is returned when the stream contains a near-lossless (NEAR)
    /// parameter outside the range [0, min(255, MAXVAL/2)]
    /// </summary>
    invalid_parameter_near_lossless = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_NEAR_LOSSLESS,

    /// <summary>
    /// This error is returned when the stream contains an invalid JPEG-LS preset parameters segment.
    /// </summary>
    invalid_parameter_jpegls_preset_parameters = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_JPEGLS_PRESET_PARAMETERS,

    /// <summary>
    /// This error is returned when the stream contains an invalid color transformation segment or one that doesn't match
    /// with frame info.
    /// </summary>
    invalid_parameter_color_transformation = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_COLOR_TRANSFORMATION,

    /// <summary>
    /// This error is returned when the stream contains a mapping table with an invalid ID.
    /// </summary>
    invalid_parameter_mapping_table_id = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_MAPPING_TABLE_ID,

    /// <summary>
    /// This error is returned when the stream contains an invalid mapping table continuation.
    /// </summary>
    invalid_parameter_mapping_table_continuation = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_MAPPING_TABLE_CONTINUATION,

    // Logic errors:

    /// <summary>
    /// This error is returned when a method call is invalid for the current state.
    /// </summary>
    invalid_operation = impl::CHARLS_JPEGLS_ERRC_INVALID_OPERATION,

    /// <summary>
    /// This error is returned when one of the passed arguments is invalid.
    /// </summary>
    invalid_argument = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT,

    /// <summary>
    /// The argument for the width parameter is outside the range [1, 65535].
    /// </summary>
    invalid_argument_width = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_WIDTH,

    /// <summary>
    /// The argument for the height parameter is outside the range [1, 65535].
    /// </summary>
    invalid_argument_height = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_HEIGHT,

    /// <summary>
    /// The argument for the bit per sample parameter is outside the range [2, 16].
    /// </summary>
    invalid_argument_bits_per_sample = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_BITS_PER_SAMPLE,

    /// <summary>
    /// The argument for the component count parameter is outside the range [1, 255].
    /// </summary>
    invalid_argument_component_count = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_COMPONENT_COUNT,

    /// <summary>
    /// The argument for the interleave mode is not (None, Sample, Line) or invalid in combination with component count.
    /// </summary>
    invalid_argument_interleave_mode = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_INTERLEAVE_MODE,

    /// <summary>
    /// The argument for the near lossless parameter is outside the range [0, min(255, MAXVAL/2)].
    /// </summary>
    invalid_argument_near_lossless = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_NEAR_LOSSLESS,

    /// <summary>
    /// The argument for the JPEG-LS preset coding parameters is not valid, see ISO/IEC 14495-1,
    /// C.2.4.1.1, Table C.1 for the ranges of valid values.
    /// </summary>
    invalid_argument_jpegls_pc_parameters = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_JPEGLS_PC_PARAMETERS,

    /// <summary>
    /// The argument for the size parameter is outside the valid range.
    /// </summary>
    invalid_argument_size = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_SIZE,

    /// <summary>
    /// The argument for the color component is not (None, Hp1, Hp2, Hp3) or invalid in combination with component count.
    /// </summary>
    invalid_argument_color_transformation = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_COLOR_TRANSFORMATION,

    /// <summary>
    /// The stride argument does not match with the frame info and buffer size.
    /// </summary>
    invalid_argument_stride = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_STRIDE,

    /// <summary>
    /// The encoding options argument has an invalid value.
    /// </summary>
    invalid_argument_encoding_options = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_ENCODING_OPTIONS,
};


/// <summary>
/// Defines the interleave modes for multi-component (color) pixel data.
/// </summary>
enum class interleave_mode
{
    /// <summary>
    /// The data is encoded and stored as component for component: RRRGGGBBB.
    /// </summary>
    none = impl::CHARLS_INTERLEAVE_MODE_NONE,

    /// <summary>
    /// The interleave mode is by line. A full line of each component is encoded before moving to the next line.
    /// </summary>
    line = impl::CHARLS_INTERLEAVE_MODE_LINE,

    /// <summary>
    /// The data is encoded and stored by sample. For RGB color images this is the format like RGBRGBRGB.
    /// </summary>
    sample = impl::CHARLS_INTERLEAVE_MODE_SAMPLE,
};


/// <summary>
/// JPEG-LS defines 3 compressed data formats. (see Annex C).
/// </summary>
enum class compressed_data_format
{
    /// <summary>
    /// Not enough information has been decoded to determine the data format.
    /// </summary>
    unknown = impl::CHARLS_COMPRESSED_DATA_FORMAT_UNKNOWN,

    /// <summary>
    /// All data to decode the image is contained in the file. This is the typical format.
    /// </summary>
    interchange = impl::CHARLS_COMPRESSED_DATA_FORMAT_INTERCHANGE,

    /// <summary>
    /// The file has references to mapping tables that need to be provided by
    /// the application environment.
    /// </summary>
    abbreviated_image_data = impl::CHARLS_COMPRESSED_DATA_FORMAT_ABBREVIATED_IMAGE_DATA,

    /// <summary>
    /// The file only contains mapping tables, no image is present.
    /// </summary>
    abbreviated_table_specification = impl::CHARLS_COMPRESSED_DATA_FORMAT_ABBREVIATED_TABLE_SPECIFICATION
};


namespace encoding_options_private {

/// <summary>
/// Defines options that can be enabled during the encoding process.
/// These options can be combined.
/// </summary>
enum class encoding_options : unsigned
{
    /// <summary>
    /// No special encoding option is defined.
    /// </summary>
    none = impl::CHARLS_ENCODING_OPTIONS_NONE,

    /// <summary>
    /// Ensures that the generated encoded data has an even size by adding
    /// an extra 0xFF byte to the End Of Image (EOI) marker.
    /// DICOM requires that data is always even. This can be done by adding a zero padding byte
    /// after the encoded data or with this option.
    /// This option is not enabled by default.
    /// </summary>
    even_destination_size = impl::CHARLS_ENCODING_OPTIONS_EVEN_DESTINATION_SIZE,

    /// <summary>
    /// Add a comment (COM) segment with the content: "charls [version-number]" to the encoded data.
    /// Storing the used encoder version can be helpful for long term archival of images.
    /// This option is not enabled by default.
    /// </summary>
    include_version_number = impl::CHARLS_ENCODING_OPTIONS_INCLUDE_VERSION_NUMBER,

    /// <summary>
    /// Writes explicitly the default JPEG-LS preset coding parameters when the
    /// bits per sample is larger than 12 bits.
    /// The Java Advanced Imaging (JAI) JPEG-LS codec has a defect that causes it to use invalid
    /// preset coding parameters for these types of images.
    /// Most users of this codec are aware of this problem and have implemented a work-around.
    /// This option is not enabled by default.
    /// </summary>
    include_pc_parameters_jai = impl::CHARLS_ENCODING_OPTIONS_INCLUDE_PC_PARAMETERS_JAI
};

constexpr encoding_options operator|(const encoding_options lhs, const encoding_options rhs) noexcept
{
    using T = std::underlying_type_t<encoding_options>;

    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange) - warning cannot handle flags (known limitation).
    return static_cast<encoding_options>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

constexpr encoding_options& operator|=(encoding_options& lhs, const encoding_options rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

} // namespace encoding_options_private

using encoding_options = encoding_options_private::encoding_options;


/// <summary>
/// Defines color space transformations as defined and implemented by the JPEG-LS library of HP Labs.
/// These color space transformation decrease the correlation between the 3 color components, resulting in better encoding
/// ratio. These options are only implemented for backwards compatibility and NOT part of the JPEG-LS standard. The JPEG-LS
/// ISO/IEC 14495-1:1999 standard provides no capabilities to transport which color space transformation was used.
/// </summary>
enum class color_transformation
{
    /// <summary>
    /// No color space transformation has been applied.
    /// </summary>
    none = impl::CHARLS_COLOR_TRANSFORMATION_NONE,

    /// <summary>
    /// Defines the reversible lossless color transformation:
    /// G = G
    /// R = R - G
    /// B = B - G
    /// </summary>
    hp1 = impl::CHARLS_COLOR_TRANSFORMATION_HP1,

    /// <summary>
    /// Defines the reversible lossless color transformation:
    /// G = G
    /// B = B - (R + G) / 2
    /// R = R - G
    /// </summary>
    hp2 = impl::CHARLS_COLOR_TRANSFORMATION_HP2,

    /// <summary>
    /// Defines the reversible lossless color transformation of Y-Cb-Cr:
    /// R = R - G
    /// B = B - G
    /// G = G + (R + B) / 4
    /// </summary>
    hp3 = impl::CHARLS_COLOR_TRANSFORMATION_HP3,
};


/// <summary>
/// Defines the Application profile identifier options that can be used in a SPIFF header v2, as defined in ISO/IEC 10918-3,
/// F.1.2
/// </summary>
enum class spiff_profile_id : std::int32_t
{
    /// <summary>
    /// No profile identified.
    /// This is the only valid option for JPEG-LS encoded images.
    /// </summary>
    none = impl::CHARLS_SPIFF_PROFILE_ID_NONE,

    /// <summary>
    /// Continuous-tone base profile (JPEG)
    /// </summary>
    continuous_tone_base = impl::CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_BASE,

    /// <summary>
    /// Continuous-tone progressive profile
    /// </summary>
    continuous_tone_progressive = impl::CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_PROGRESSIVE,

    /// <summary>
    /// Bi-level facsimile profile (MH, MR, MMR, JBIG)
    /// </summary>
    bi_level_facsimile = impl::CHARLS_SPIFF_PROFILE_ID_BI_LEVEL_FACSIMILE,

    /// <summary>
    /// Continuous-tone facsimile profile (JPEG)
    /// </summary>
    continuous_tone_facsimile = impl::CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_FACSIMILE
};


/// <summary>
/// Defines the color space options that can be used in a SPIFF header v2, as defined in ISO/IEC 10918-3, F.2.1.1
/// </summary>
enum class spiff_color_space : std::int32_t
{
    /// <summary>
    /// Bi-level image. Each image sample is one bit: 0 = white and 1 = black.
    /// This option is not valid for JPEG-LS encoded images.
    /// </summary>
    bi_level_black = impl::CHARLS_SPIFF_COLOR_SPACE_BI_LEVEL_BLACK,

    /// <summary>
    /// The color space is based on recommendation ITU-R BT.709.
    /// </summary>
    ycbcr_itu_bt_709_video = impl::CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_709_VIDEO,

    /// <summary>
    /// Color space interpretation of the coded sample is none of the other options.
    /// </summary>
    none = impl::CHARLS_SPIFF_COLOR_SPACE_NONE,

    /// <summary>
    /// The color space is based on recommendation ITU-R BT.601-1. (RGB).
    /// </summary>
    ycbcr_itu_bt_601_1_rgb = impl::CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_601_1_RGB,

    /// <summary>
    /// The color space is based on recommendation ITU-R BT.601-1. (video).
    /// </summary>
    ycbcr_itu_bt_601_1_video = impl::CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_601_1_VIDEO,

    /// <summary>
    /// Grayscale – This is a single component sample with interpretation as grayscale value, 0 is minimum, 2bps -1 is
    /// maximum.
    /// </summary>
    grayscale = impl::CHARLS_SPIFF_COLOR_SPACE_GRAYSCALE,

    /// <summary>
    /// This is the color encoding method used in the Photo CD™ system.
    /// </summary>
    photo_ycc = impl::CHARLS_SPIFF_COLOR_SPACE_PHOTO_YCC,

    /// <summary>
    /// The encoded data consists of samples of (uncalibrated) R, G and B.
    /// </summary>
    rgb = impl::CHARLS_SPIFF_COLOR_SPACE_RGB,

    /// <summary>
    /// The encoded data consists of samples of Cyan, Magenta and Yellow samples.
    /// </summary>
    cmy = impl::CHARLS_SPIFF_COLOR_SPACE_CMY,

    /// <summary>
    /// The encoded data consists of samples of Cyan, Magenta, Yellow and Black samples.
    /// </summary>
    cmyk = impl::CHARLS_SPIFF_COLOR_SPACE_CMYK,

    /// <summary>
    /// Transformed CMYK type data (same as Adobe PostScript)
    /// </summary>
    ycck = impl::CHARLS_SPIFF_COLOR_SPACE_YCCK,

    /// <summary>
    /// The CIE 1976 (L* a* b*) color space.
    /// </summary>
    cie_lab = impl::CHARLS_SPIFF_COLOR_SPACE_CIE_LAB,

    /// <summary>
    /// Bi-level image. Each image sample is one bit: 1 = white and 0 = black.
    /// This option is not valid for JPEG-LS encoded images.
    /// </summary>
    bi_level_white = impl::CHARLS_SPIFF_COLOR_SPACE_BI_LEVEL_WHITE
};


/// <summary>
/// Defines the compression options that can be used in a SPIFF header v2, as defined in ISO/IEC 10918-3, F.2.1
/// </summary>
enum class spiff_compression_type : std::int32_t
{
    /// <summary>
    /// Picture data is stored in component interleaved format, encoded at BPS per sample.
    /// </summary>
    uncompressed = impl::CHARLS_SPIFF_COMPRESSION_TYPE_UNCOMPRESSED,

    /// <summary>
    /// Recommendation T.4, the basic algorithm commonly known as MH (Modified Huffman), only allowed for bi-level images.
    /// </summary>
    modified_huffman = impl::CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_HUFFMAN,

    /// <summary>
    /// Recommendation T.4, commonly known as MR (Modified READ), only allowed for bi-level images.
    /// </summary>
    modified_read = impl::CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_READ,

    /// <summary>
    /// Recommendation T .6, commonly known as MMR (Modified Modified READ), only allowed for bi-level images.
    /// </summary>
    modified_modified_read = impl::CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_MODIFIED_READ,

    /// <summary>
    /// ISO/IEC 11544, commonly known as JBIG, only allowed for bi-level images.
    /// </summary>
    jbig = impl::CHARLS_SPIFF_COMPRESSION_TYPE_JBIG,

    /// <summary>
    /// ISO/IEC 10918-1 or ISO/IEC 10918-3, commonly known as JPEG.
    /// </summary>
    jpeg = impl::CHARLS_SPIFF_COMPRESSION_TYPE_JPEG,

    /// <summary>
    /// ISO/IEC 14495-1 or ISO/IEC 14495-2, commonly known as JPEG-LS. (extension defined in ISO/IEC 14495-1).
    /// This is the only valid option for JPEG-LS encoded images.
    /// </summary>
    jpeg_ls = impl::CHARLS_SPIFF_COMPRESSION_TYPE_JPEG_LS
};


/// <summary>
/// Defines the resolution units for the VRES and HRES parameters, as defined in ISO/IEC 10918-3, F.2.1
/// </summary>
enum class spiff_resolution_units : std::int32_t
{
    /// <summary>
    /// VRES and HRES are to be interpreted as aspect ratio.
    /// </summary>
    /// <remark>
    /// If vertical or horizontal resolutions are not known, use this option and set VRES and HRES
    /// both to 1 to indicate that pixels in the image should be assumed to be square.
    /// </remark>
    aspect_ratio = impl::CHARLS_SPIFF_RESOLUTION_UNITS_ASPECT_RATIO,

    /// <summary>
    /// Units of dots/samples per inch
    /// </summary>
    dots_per_inch = impl::CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_INCH,

    /// <summary>
    /// Units of dots/samples per centimeter.
    /// </summary>
    dots_per_centimeter = impl::CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_CENTIMETER
};


/// <summary>
/// Official defined SPIFF tags defined in Table F.5 (ISO/IEC 10918-3)
/// </summary>
enum class spiff_entry_tag : std::uint32_t
{
    /// <summary>
    /// This entry describes the opto-electronic transfer characteristics of the source image.
    /// </summary>
    transfer_characteristics = impl::CHARLS_SPIFF_ENTRY_TAG_TRANSFER_CHARACTERISTICS,

    /// <summary>
    /// This entry specifies component registration, the spatial positioning of samples within components relative to the
    /// samples of other components.
    /// </summary>
    component_registration = impl::CHARLS_SPIFF_ENTRY_TAG_COMPONENT_REGISTRATION,

    /// <summary>
    /// This entry specifies the image orientation (rotation, flip).
    /// </summary>
    image_orientation = impl::CHARLS_SPIFF_ENTRY_TAG_IMAGE_ORIENTATION,

    /// <summary>
    /// This entry specifies a reference to a thumbnail.
    /// </summary>
    thumbnail = impl::CHARLS_SPIFF_ENTRY_TAG_THUMBNAIL,

    /// <summary>
    /// This entry describes in textual form a title for the image.
    /// </summary>
    image_title = impl::CHARLS_SPIFF_ENTRY_TAG_IMAGE_TITLE,

    /// <summary>
    /// This entry refers to data in textual form containing additional descriptive information about the image.
    /// </summary>
    image_description = impl::CHARLS_SPIFF_ENTRY_TAG_IMAGE_DESCRIPTION,

    /// <summary>
    /// This entry describes the date and time of the last modification of the image.
    /// </summary>
    time_stamp = impl::CHARLS_SPIFF_ENTRY_TAG_TIME_STAMP,

    /// <summary>
    /// This entry describes in textual form a version identifier which refers to the number of revisions of the image.
    /// </summary>
    version_identifier = impl::CHARLS_SPIFF_ENTRY_TAG_VERSION_IDENTIFIER,

    /// <summary>
    /// This entry describes in textual form the creator of the image.
    /// </summary>
    creator_identification = impl::CHARLS_SPIFF_ENTRY_TAG_CREATOR_IDENTIFICATION,

    /// <summary>
    /// The presence of this entry, indicates that the image’s owner has retained copyright protection and usage rights for
    /// the image.
    /// </summary>
    protection_indicator = impl::CHARLS_SPIFF_ENTRY_TAG_PROTECTION_INDICATOR,

    /// <summary>
    /// This entry describes in textual form copyright information for the image.
    /// </summary>
    copyright_information = impl::CHARLS_SPIFF_ENTRY_TAG_COPYRIGHT_INFORMATION,

    /// <summary>
    /// This entry describes in textual form contact information for use of the image.
    /// </summary>
    contact_information = impl::CHARLS_SPIFF_ENTRY_TAG_CONTACT_INFORMATION,

    /// <summary>
    /// This entry refers to data containing a list of offsets into the file.
    /// </summary>
    tile_index = impl::CHARLS_SPIFF_ENTRY_TAG_TILE_INDEX,

    /// <summary>
    /// This entry refers to data containing the scan list.
    /// </summary>
    scan_index = impl::CHARLS_SPIFF_ENTRY_TAG_SCAN_INDEX,

    /// <summary>
    /// This entry contains a 96-bit reference number intended to relate images stored in separate files.
    /// </summary>
    set_reference = impl::CHARLS_SPIFF_ENTRY_TAG_SET_REFERENCE
};

constexpr int mapping_table_missing{impl::CHARLS_MAPPING_TABLE_MISSING};

} // namespace charls

CHARLS_EXPORT
template<>
struct std::is_error_code_enum<charls::jpegls_errc> final : std::true_type
{
};

using charls_jpegls_errc = charls::jpegls_errc;
using charls_interleave_mode = charls::interleave_mode;
using charls_compressed_data_format = charls::compressed_data_format;
using charls_encoding_options = charls::encoding_options;
using charls_color_transformation = charls::color_transformation;

using charls_spiff_profile_id = charls::spiff_profile_id;
using charls_spiff_color_space = charls::spiff_color_space;
using charls_spiff_compression_type = charls::spiff_compression_type;
using charls_spiff_resolution_units = charls::spiff_resolution_units;
using charls_spiff_entry_tag = charls::spiff_entry_tag;

#else

typedef enum charls_jpegls_errc charls_jpegls_errc;
typedef enum charls_interleave_mode charls_interleave_mode;
typedef enum charls_compressed_data_format charls_compressed_data_format;
typedef enum charls_encoding_options charls_encoding_options;
typedef enum charls_color_transformation charls_color_transformation;

typedef int32_t charls_spiff_profile_id;
typedef int32_t charls_spiff_color_space;
typedef int32_t charls_spiff_compression_type;
typedef int32_t charls_spiff_resolution_units;

#endif


/// <summary>
/// Defines the information that can be stored in a SPIFF header as defined in ISO/IEC 10918-3, Annex F
/// </summary>
/// <remark>
/// The type I.8 is an unsigned 8-bit  integer, the type I.32 is an 32-bit  unsigned integer in the file header itself.
/// The type is indicated by the symbol “F.” are 4-byte parameters in “fixed point” notation.
/// The 16 most significant bits are essentially the same as a parameter of type I.16 and indicate the integer
/// part of this number.
/// The 16 least significant bits are essentially the same as an I.16 parameter and contain an unsigned integer that,
/// when divided by 65536, represents the fractional part of the fixed point number.
/// </remark>
struct charls_spiff_header CHARLS_FINAL
{
    charls_spiff_profile_id profile_id;   // P: Application profile, type I.8
    CHARLS_STD int32_t component_count;   // NC: Number of color components, range [1, 255], type I.8
    CHARLS_STD uint32_t height;           // HEIGHT: Number of lines in image, range [1, 4294967295], type I.32
    CHARLS_STD uint32_t width;            // WIDTH: Number of samples per line, range [1, 4294967295], type I.32
    charls_spiff_color_space color_space; // S: Color space used by image data, type is I.8
    CHARLS_STD int32_t bits_per_sample;   // BPS: Number of bits per sample, range (1, 2, 4, 8, 12, 16), type is I.8
    charls_spiff_compression_type compression_type; // C: Type of data compression used, type is I.8
    charls_spiff_resolution_units resolution_units; // R: Type of resolution units, type is I.8
    CHARLS_STD uint32_t vertical_resolution;   // VRES: Vertical resolution, range [1, 4294967295], type can be F or I.32
    CHARLS_STD uint32_t horizontal_resolution; // HRES: Horizontal resolution, range [1, 4294967295], type can be F or I.32
};


/// <summary>
/// Defines the information that can be stored in a JPEG-LS Frame marker segment that applies to all scans.
/// </summary>
/// <remark>
/// The JPEG-LS also allow to store subsampling information in a JPEG-LS Frame marker segment.
/// CharLS does not support JPEG-LS images that contain subsampled scans.
/// </remark>
struct charls_frame_info CHARLS_FINAL
{
    /// <summary>
    /// Width of the image, range [1, 4294967295].
    /// </summary>
    CHARLS_STD uint32_t width;

    /// <summary>
    /// Height of the image, range [1, 4294967295].
    /// </summary>
    CHARLS_STD uint32_t height;

    /// <summary>
    /// Number of bits per sample, range [2, 16]
    /// </summary>
    CHARLS_STD int32_t bits_per_sample;

    /// <summary>
    /// Number of components contained in the frame, range [1, 255]
    /// </summary>
    CHARLS_STD int32_t component_count;
};


/// <summary>
/// Defines the JPEG-LS preset coding parameters as defined in ISO/IEC 14495-1, C.2.4.1.1.
/// JPEG-LS defines a default set of parameters, but custom parameters can be used.
/// When used these parameters are written into the encoded bit stream as they are needed for the decoding process.
/// </summary>
struct charls_jpegls_pc_parameters CHARLS_FINAL
{
    /// <summary>
    /// Maximum possible value for any image sample in a scan.
    /// This must be greater than or equal to the actual maximum value for the components in a scan.
    /// </summary>
    CHARLS_STD int32_t maximum_sample_value;

    /// <summary>
    /// First quantization threshold value for the local gradients.
    /// </summary>
    CHARLS_STD int32_t threshold1;

    /// <summary>
    /// Second quantization threshold value for the local gradients.
    /// </summary>
    CHARLS_STD int32_t threshold2;

    /// <summary>
    /// Third quantization threshold value for the local gradients.
    /// </summary>
    CHARLS_STD int32_t threshold3;

    /// <summary>
    /// Value at which the counters A, B, and N are halved.
    /// </summary>
    CHARLS_STD int32_t reset_value;
};


/// <summary>
/// Defines the information that describes a mapping table.
/// </summary>
struct charls_mapping_table_info CHARLS_FINAL
{
    /// <summary>
    /// Identifier of the mapping table, range [1, 255].
    /// </summary>
    CHARLS_STD int32_t table_id;

    /// <summary>
    /// Width of a table entry in bytes, range [1, 255].
    /// </summary>
    CHARLS_STD int32_t entry_size;

    /// <summary>
    /// Size of the table in bytes, range [1, 16711680]
    /// </summary>
    CHARLS_STD uint32_t data_size;
};


#ifdef __cplusplus

/// <summary>
/// Function definition for a callback handler that will be called when a comment (COM) segment is found.
/// </summary>
/// <remarks>
/// </remarks>
/// <param name="data">Reference to the data of the COM segment.</param>
/// <param name="size">Size in bytes of the data of the COM segment.</param>
/// <param name="user_context">Free to use context information that can be set during the installation of the
/// handler.</param>
using charls_at_comment_handler = std::int32_t(CHARLS_API_CALLING_CONVENTION*)(const void* data, std::size_t size,
                                                                               void* user_context);

/// <summary>
/// Function definition for a callback handler that will be called when an application data (APPn) segment is found.
/// </summary>
/// <remarks>
/// </remarks>
/// <param name="application_data_id">Identifier of the APPn segment [0 - 15].</param>
/// <param name="data">Reference to the data of the APPn segment.</param>
/// <param name="size">Size in bytes of the data of the APPn segment.</param>
/// <param name="user_context">Free to use context information that can be set during the installation of the
/// handler.</param>
using charls_at_application_data_handler = std::int32_t(CHARLS_API_CALLING_CONVENTION*)(std::int32_t application_data_id,
                                                                                        const void* data, std::size_t size,
                                                                                        void* user_context);

CHARLS_EXPORT
namespace charls {

using spiff_header = charls_spiff_header;
using frame_info = charls_frame_info;
using jpegls_pc_parameters = charls_jpegls_pc_parameters;
using mapping_table_info = charls_mapping_table_info;
using at_comment_handler = charls_at_comment_handler;
using at_application_data_handler = charls_at_application_data_handler;

static_assert(sizeof(spiff_header) == 40, "size of struct is incorrect, check padding settings");
static_assert(sizeof(frame_info) == 16, "size of struct is incorrect, check padding settings");
static_assert(sizeof(jpegls_pc_parameters) == 20, "size of struct is incorrect, check padding settings");
static_assert(sizeof(mapping_table_info) == 12, "size of struct is incorrect, check padding settings");

} // namespace charls

#else

typedef int32_t(CHARLS_API_CALLING_CONVENTION* charls_at_comment_handler)(const void* data, size_t size, void* user_context);
typedef int32_t(CHARLS_API_CALLING_CONVENTION* charls_at_application_data_handler)(int32_t application_data_id,
                                                                                   const void* data, size_t size,
                                                                                   void* user_context);

typedef struct charls_spiff_header charls_spiff_header;
typedef struct charls_frame_info charls_frame_info;
typedef struct charls_jpegls_pc_parameters charls_jpegls_pc_parameters;
typedef struct charls_mapping_table_info charls_mapping_table_info;

#endif
