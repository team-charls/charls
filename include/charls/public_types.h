// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#ifdef __cplusplus

#include <cstdint>
#include <cstddef>
#include <system_error>


// GCC 5.0 cannot handle [[deprecated]] on enum values
#if defined(__GNUC__) && !defined(__clang__)
#define CHARLS_ENUM_DEPRECATED
#else
#define CHARLS_ENUM_DEPRECATED [[deprecated]]
#endif


namespace charls
{
    /// <summary>
    /// Defines the result values that are returned by the CharLS API functions.
    /// </summary>
    enum class jpegls_errc
    {
        success = 0,                             // The operation completed without errors.
        invalid_argument = 1,                    // This error is returned when one of the arguments is invalid and no specific reason is available.
        parameter_value_not_supported = 2,       // This error is returned when the JPEG stream contains a parameter value that is not supported by this implementation.
        destination_buffer_too_small = 3,        // The destination buffer is too small to hold all the output.
        source_buffer_too_small = 4,             // The source buffer is too small, more input data was expected.
        invalid_encoded_data = 5,                // This error is returned when the encoded bit stream contains a general structural problem.
        too_much_encoded_data = 6,               // Too much compressed data.The decoding process is ready but the input buffer still contains encoded data.
        bit_depth_for_transform_not_supported = 8, // The bit depth for transformation is not supported.
        color_transform_not_supported = 9,       // The color transform is not supported.
        encoding_not_supported = 10,             // This error is returned when an encoded frame is found that is not encoded with the JPEG-LS algorithm.
        unknown_jpeg_marker_found = 11,          // This error is returned when an unknown JPEG marker code is found in the encoded bit stream.
        jpeg_marker_start_byte_not_found = 12,   // This error is returned when the algorithm expect a 0xFF code (indicates start of a JPEG marker) but none was found.
        not_enough_memory = 13,                  // This error is returned when the implementation could not allocate memory for its internal buffers.
        unexpected_failure = 14,                 // This error is returned when the implementation encountered a failure it didn't expect. No guarantees can be given for the state after this error.
        start_of_image_marker_not_found = 15,    // This error is returned when the first JPEG marker is not the SOI marker.
        start_of_frame_marker_not_found = 16,    // This error is returned when the SOF JPEG marker is not found before the SOS marker.
        invalid_marker_segment_size = 17,        // This error is returned when the segment size of a marker segment is invalid.
        duplicate_start_of_image_marker = 18,    // This error is returned when the stream contains more then one SOI marker.
        duplicate_start_of_frame_marker = 19,    // This error is returned when the stream contains more then one SOF marker.
        unexpected_end_of_image_marker = 20,     // This error is returned when the stream contains an unexpected EOI marker.
        invalid_jpegls_preset_parameter_type = 21, // This error is returned when the stream contains an invalid type parameter in the JPEG-LS segment.
        jpegls_preset_extended_parameter_type_not_supported = 22, // This error is returned when the stream contains an unsupported type parameter in the JPEG-LS segment.
        invalid_argument_width = 100,            // The argument for the width parameter is outside the range [1, 65535].
        invalid_argument_height = 101,           // The argument for the height parameter is outside the range [1, 65535].
        invalid_argument_component_count = 102,  // The argument for the component count parameter is outside the range [1, 255].
        invalid_argument_bits_per_sample = 103,  // The argument for the bit per sample parameter is outside the range [2, 16].
        invalid_argument_interleave_mode = 104,  // The argument for the interleave mode is not (None, Sample, Line) or invalid in combination with component count.
        invalid_argument_destination = 105,      // The destination buffer or stream is not set.
        invalid_argument_source = 106,           // The source buffer or stream is not set.
        invalid_argument_thumbnail = 107,        // The arguments for the thumbnail and the dimensions don't match.
        invalid_parameter_width = 200,           // This error is returned when the stream contains a width parameter defined more then once or in an incompatible way.
        invalid_parameter_height = 201,          // This error is returned when the stream contains a height parameter defined more then once in an incompatible way.
        invalid_parameter_component_count = 202, // This error is returned when the stream contains a component count parameter outside the range [1,255]
        invalid_parameter_bits_per_sample = 203, // This error is returned when the stream contains a bits per sample (sample precision) parameter outside the range [2,16]
        invalid_parameter_interleave_mode = 204, // This error is returned when the stream contains an interleave mode (ILV) parameter outside the range [0, 2]

        // Legacy enum names, will be removed in a future release.
        OK CHARLS_ENUM_DEPRECATED = success,
        InvalidJlsParameters CHARLS_ENUM_DEPRECATED = invalid_argument,
        ParameterValueNotSupported CHARLS_ENUM_DEPRECATED = invalid_encoded_data,
        UncompressedBufferTooSmall CHARLS_ENUM_DEPRECATED = destination_buffer_too_small,
        CompressedBufferTooSmall CHARLS_ENUM_DEPRECATED = source_buffer_too_small,
        InvalidCompressedData CHARLS_ENUM_DEPRECATED = invalid_encoded_data,
        TooMuchCompressedData CHARLS_ENUM_DEPRECATED = too_much_encoded_data,
        UnsupportedColorTransform CHARLS_ENUM_DEPRECATED = color_transform_not_supported,
        UnsupportedEncoding CHARLS_ENUM_DEPRECATED = encoding_not_supported,
        UnknownJpegMarker CHARLS_ENUM_DEPRECATED = unknown_jpeg_marker_found,
        MissingJpegMarkerStart CHARLS_ENUM_DEPRECATED = jpeg_marker_start_byte_not_found,
        UnexpectedFailure CHARLS_ENUM_DEPRECATED = unexpected_failure
    };

    [[deprecated]]
    typedef jpegls_errc ApiResult;

    /// <summary>
    /// Defines the interleave mode for multi-component (color) pixel data.
    /// </summary>
    enum class InterleaveMode
    {
        /// <summary>
        /// The data is encoded and stored as component for component: RRRGGGBBB.
        /// </summary>
        None   = 0,

        /// <summary>
        /// The interleave mode is by line. A full line of each component is encoded before moving to the next line.
        /// </summary>
        Line   = 1,

        /// <summary>
        /// The data is encoded and stored by sample. For color images this is the format like RGBRGBRGB.
        /// </summary>
        Sample = 2
    };

    /// <summary>
    /// Defines color space transformations as defined and implemented by the JPEG-LS library of HP Labs.
    /// These color space transformation decrease the correlation between the 3 color components, resulting in better encoding ratio.
    /// These options are only implemented for backwards compatibility and NOT part of the JPEG-LS standard.
    /// The JPEG-LS ISO/IEC 14495-1:1999 standard provides no capabilities to transport which color space transformation was used.
    /// </summary>
    enum class ColorTransformation
    {
        /// <summary>
        /// No color space transformation has been applied.
        /// </summary>
        None = 0,

        /// <summary>
        /// Defines the reversible lossless color transformation:
        /// G = G
        /// R = R - G
        /// B = B - G
        /// </summary>
        HP1 = 1,

        /// <summary>
        /// Defines the reversible lossless color transformation:
        /// G = G
        /// B = B - (R + G) / 2
        /// R = R - G
        /// </summary>
        HP2 = 2,

        /// <summary>
        /// Defines the reversible lossless color transformation of Y-Cb-Cr):
        /// R = R - G
        /// B = B - G
        /// G = G + (R + B) / 4
        /// </summary>
        HP3 = 3,
    };
}

namespace std {

template<> struct is_error_code_enum<charls::jpegls_errc> : true_type {};

} // namespace std


using CharlsApiResultType = charls::jpegls_errc;
using CharlsInterleaveModeType = charls::InterleaveMode;
using CharlsColorTransformationType = charls::ColorTransformation;

#else

#include <stdint.h>

// This API return code table is a copy of the C++ table. For additional info see the C++ table.
// 2 tables are defined to prevent global namespace pollution.
enum CharlsApiResult
{
    CHARLS_API_RESULT_SUCCESS                               = 0,
    CHARLS_API_RESULT_INVALID_ARGUMENT                      = 1,
    CHARLS_API_RESULT_PARAMETER_VALUE_NOT_SUPPORTED         = 2,
    CHARLS_API_RESULT_DESTINATION_BUFFER_TOO_SMALL          = 3,
    CHARLS_API_RESULT_SOURCE_BUFFER_TOO_SMALL               = 4,
    CHARLS_API_RESULT_INVALID_ENCODED_DATA                  = 5,
    CHARLS_API_RESULT_TOO_MUCH_ENCODED_DATA                 = 6,
    CHARLS_API_RESULT_IMAGE_TYPE_NOT_SUPPORTED              = 7,
    CHARLS_API_RESULT_BIT_DEPTH_FOR_TRANSFORM_NOT_SUPPORTED = 8,
    CHARLS_API_RESULT_COLOR_TRANSFORM_NOT_SUPPORTED         = 9,
    CHARLS_API_RESULT_ENCODING_NOT_SUPPORTED                = 10,
    CHARLS_API_RESULT_UNKNOWN_JPEG_MARKER_FOUND             = 11,
    CHARLS_API_RESULT_JPEG_MARKER_START_BYTE_NOT_FOUND      = 12,
    CHARLS_API_RESULT_NOT_ENOUGH_MEMORY                     = 13,
    CHARLS_API_RESULT_UNEXPECTED_FAILURE                    = 14,
    CHARLS_API_RESULT_START_OF_IMAGE_MARKER_NOT_FOUND       = 15,
    CHARLS_API_RESULT_START_OF_FRAME_MARKER_NOT_FOUND       = 16,
    CHARLS_API_RESULT_INVALID_MARKER_SEGMENT_SIZE           = 17,
    CHARLS_API_RESULT_DUPLICATE_START_OF_IMAGE_MARKER       = 18,
    CHARLS_API_RESULT_DUPLICATE_START_OF_FRAME_MARKER       = 19,
    CHARLS_API_RESULT_UNEXPECTED_END_OF_IMAGE_MARKER        = 20,
    CHARLS_API_RESULT_INVALID_JPEGLS_PRESET_PARAMETER_TYPE  = 21,
    CHARLS_API_RESULT_JPEGLS_PRESET_EXTENDED_PARAMETER_TYPE_NOT_SUPPORTED = 22,
    CHARLS_API_RESULT_INVALID_ARGUMENT_WIDTH                = 100,
    CHARLS_API_RESULT_INVALID_ARGUMENT_HEIGHT               = 101,
    CHARLS_API_RESULT_INVALID_ARGUMENT_COMPONENT_COUNT      = 102,
    CHARLS_API_RESULT_INVALID_ARGUMENT_BITS_PER_SAMPLE      = 103,
    CHARLS_API_RESULT_INVALID_ARGUMENT_INTERLEAVE_MODE      = 104,
    CHARLS_API_RESULT_INVALID_ARGUMENT_DESTINATION          = 105,
    CHARLS_API_RESULT_INVALID_ARGUMENT_SOURCE               = 106,
    CHARLS_API_RESULT_INVALID_ARGUMENT_THUMBNAIL            = 107,
    CHARLS_API_RESULT_INVALID_PARAMETER_WIDTH               = 200,
    CHARLS_API_RESULT_INVALID_PARAMETER_HEIGHT              = 201,
    CHARLS_API_RESULT_INVALID_PARAMETER_COMPONENT_COUNT     = 202,
    CHARLS_API_RESULT_INVALID_PARAMETER_BITS_PER_SAMPLE     = 203,
    CHARLS_API_RESULT_INVALID_PARAMETER_INTERLEAVE_MODE     = 204,
};

enum CharlsInterleaveMode
{
    CHARLS_IM_NONE   = 0,
    CHARLS_IM_LINE   = 1,
    CHARLS_IM_SAMPLE = 2
};

enum CharlsColorTransformation
{
    CHARLS_COLOR_TRANSFORMATION_NONE = 0,
    CHARLS_COLOR_TRANSFORMATION_HP1 = 1,
    CHARLS_COLOR_TRANSFORMATION_HP2 = 2,
    CHARLS_COLOR_TRANSFORMATION_HP3 = 3,
};

typedef enum CharlsApiResult CharlsApiResultType;
typedef enum CharlsInterleaveMode CharlsInterleaveModeType;
typedef enum CharlsColorTransformation CharlsColorTransformationType;


#endif


/// <summary>
/// Defines the JPEG-LS preset coding parameters as defined in ISO/IEC 14495-1, C.2.4.1.1.
/// JPEG-LS defines a default set of parameters, but custom parameters can be used.
/// When used these parameters are written into the encoded bit stream as they are needed for the decoding process.
/// </summary>
struct JpegLSPresetCodingParameters
{
    /// <summary>
    /// Maximum possible value for any image sample in a scan.
    /// This must be greater than or equal to the actual maximum value for the components in a scan.
    /// </summary>
    int32_t MaximumSampleValue;

    /// <summary>
    /// First quantization threshold value for the local gradients.
    /// </summary>
    int32_t Threshold1;

    /// <summary>
    /// Second quantization threshold value for the local gradients.
    /// </summary>
    int32_t Threshold2;

    /// <summary>
    /// Third quantization threshold value for the local gradients.
    /// </summary>
    int32_t Threshold3;

    /// <summary>
    /// Value at which the counters A, B, and N are halved.
    /// </summary>
    int32_t ResetValue;
};


struct JlsRect
{
    int32_t X;
    int32_t Y;
    int32_t Width;
    int32_t Height;
};


/// <summary>
/// Defines the parameters for the JPEG File Interchange Format.
/// The format is defined in the JPEG File Interchange Format v1.02 document by Eric Hamilton.
/// </summary>
/// <remarks>
/// The JPEG File Interchange Format is the de facto standard JPEG interchange format.
/// </remarks>
struct JfifParameters
{
    /// <summary>
    /// Version of the JPEG File Interchange Format.
    /// Should be set to zero to not write a JFIF header or to 1.02, encoded as: (1 * 256) + 2.
    /// </summary>
    int32_t version;

    /// <summary>
    /// Defines the units for the X and Y densities.
    /// 0: no units, X and Y specify the pixel aspect ratio.
    /// 1: X and Y are dots per inch.
    /// 2: X and Y are dots per cm.
    /// </summary>
    int32_t units;

    /// <summary>
    /// Horizontal pixel density
    /// </summary>
    int32_t Xdensity;

    /// <summary>
    /// Vertical pixel density
    /// </summary>
    int32_t Ydensity;

    /// <summary>
    /// Thumbnail horizontal pixel count.
    /// </summary>
    int32_t Xthumbnail;

    /// <summary>
    /// Thumbnail vertical pixel count.
    /// </summary>
    int32_t Ythumbnail;

    /// <summary>
    /// Reference to a buffer with thumbnail pixels of size Xthumbnail * Ythumbnail * 3(RGB).
    /// This parameter is only used when creating JPEG-LS encoded images.
    /// </summary>
    void* thumbnail;
};


struct JlsParameters
{
    /// <summary>
    /// Width of the image in pixels.
    /// This parameter is called "Number of samples per line" in the JPEG-LS standard.
    /// </summary>
    int32_t width;

    /// <summary>
    /// Height of the image in pixels.
    /// This parameter is called "Number of lines" in the JPEG-LS standard.
    /// </summary>
    int32_t height;

    /// <summary>
    /// The number of valid bits per sample to encode.
    /// Valid range 2 - 16. When greater than 8, pixels are assumed to stored as two bytes per sample, otherwise one byte per sample is assumed.
    /// This parameter is called "Sample precision" in the JPEG-LS standard, often also called "Bit Depth".
    /// </summary>
    int32_t bitsPerSample;

    /// <summary>
    /// The stride is the number of bytes from one row of pixels in memory to the next row of pixels in memory.
    /// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
    /// </summary>
    int32_t stride;

    /// <summary>
    /// The number of components.
    /// Typical 1 for monochrome images and 3 for color images or 4 if an alpha channel is present.
    /// Up to 255 components are supported by the JPEG-LS standard.
    /// </summary>
    int32_t components;

    /// <summary>
    /// Defines the allowed lossy error. Value 0 defines lossless.
    /// </summary>
    int32_t allowedLossyError;

    /// <summary>
    /// Determines the order of the color components in the compressed stream.
    /// </summary>
    CharlsInterleaveModeType interleaveMode;

    /// <summary>
    /// Color transformation used in the compressed stream. The color transformations are all lossless and
    /// are an HP proprietary extension of the standard. Do not use the color transformations unless
    /// you know the decoder is capable of decoding it. Color transform typically improve compression ratios only
    /// for synthetic images (non - photo-realistic computer generated images).
    /// </summary>
    CharlsColorTransformationType colorTransformation;

    /// <summary>
    /// If set to true RGB images will be decoded to BGR. BGR is the standard ordering in MS Windows bitmaps.
    /// </summary>
    char outputBgr;

    struct JpegLSPresetCodingParameters custom;
    struct JfifParameters jfif;
};


#ifdef __cplusplus

#include <iostream>


//
// ByteStreamInfo & FromByteArray helper function
//
// ByteStreamInfo describes the stream: either set rawStream to a valid stream, or rawData/count, not both.
// it's possible to decode to memory streams, but using rawData will always be faster.
struct ByteStreamInfo
{
    std::basic_streambuf<char>* rawStream;
    uint8_t* rawData;
    std::size_t count;
};


inline ByteStreamInfo FromByteArray(void* bytes, std::size_t count) noexcept
{
    return { nullptr, static_cast<uint8_t*>(bytes), count };
}


inline ByteStreamInfo FromByteArrayConst(const void* bytes, std::size_t count) noexcept
{
    return FromByteArray(const_cast<void*>(bytes), count);
}


#endif
