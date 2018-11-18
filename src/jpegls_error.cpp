// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include <charls/jpegls_error.h>

namespace charls {

class jpegls_category_t : public std::error_category
{
public:
    const char* name() const noexcept override
    {
        return "charls::jpegls";
    }

    std::string message(int error_value) const override
    {
        return charls_get_error_message(error_value);
    }
};

}

using namespace charls;

const void* CHARLS_API_CALLING_CONVENTION charls_jpegls_category()
{
    static jpegls_category_t instance;
    return &instance;
}

const char* CHARLS_API_CALLING_CONVENTION charls_get_error_message(int32_t error_value)
{
    switch (static_cast<jpegls_errc>(error_value))
    {
    case jpegls_errc::success:
        return "";

    case jpegls_errc::invalid_argument:
        return "Invalid argument";

    case jpegls_errc::invalid_argument_width:
        return "The width argument is outside the supported range [1, 65535]";

    case jpegls_errc::invalid_argument_height:
        return "The height argument is outside the supported range [1, 65535]";

    case jpegls_errc::invalid_argument_component_count:
        return "The component count argument is outside the range [1, 255]";

    case jpegls_errc::invalid_argument_bits_per_sample:
        return "The bit per sample argument is outside the range [2, 16]";

    case jpegls_errc::invalid_argument_interleave_mode:
        return "The interleave mode is not None, Sample, Line) or invalid in combination with component count";

    case jpegls_errc::invalid_argument_destination:
        return "The destination buffer or stream is not set";

    case jpegls_errc::invalid_argument_source:
        return "The source buffer or stream is not set";

    case jpegls_errc::invalid_argument_thumbnail:
        return "The arguments for the thumbnail and the dimensions don't match";

    case jpegls_errc::start_of_image_marker_not_found:
        return "Invalid JPEG-LS stream, first JPEG marker is not a Start Of Image (SOI) marker";

    case jpegls_errc::start_of_frame_marker_not_found:
        return "Invalid JPEG-LS stream, Start Of Frame (SOF) marker not found before the SOS marker";

    case jpegls_errc::invalid_marker_segment_size:
        return "Invalid JPEG-LS stream, segment size of a marker segment is invalid";

    case jpegls_errc::duplicate_start_of_image_marker:
        return "Invalid JPEG-LS stream, more then one Start Of Image (SOI) marker";

    case jpegls_errc::duplicate_start_of_frame_marker:
        return "Invalid JPEG-LS stream, more then one Start Of Frame (SOF) marker";

    case jpegls_errc::unexpected_end_of_image_marker:
        return "Invalid JPEG-LS stream, unexpected End Of Image (EOI) marker";

    case jpegls_errc::invalid_jpegls_preset_parameter_type:
        return "Invalid JPEG-LS stream, JPEG-LS preset parameters segment contains an invalid type";

    case jpegls_errc::jpegls_preset_extended_parameter_type_not_supported:
        return "Unsupported JPEG-LS stream, JPEG-LS preset parameters segment contains an JPEG-LS Extended (ISO/IEC 14495-2) type";

    case jpegls_errc::invalid_parameter_bits_per_sample:
        return "Invalid JPEG-LS stream, The bit per sample (sample precision) parameter is not in the range [2, 16]";

    case jpegls_errc::parameter_value_not_supported:
        return "The JPEG-LS stream is encoded with a parameter value that is not supported by the CharLS decoder";

    case jpegls_errc::destination_buffer_too_small:
        return "The destination buffer is too small to hold all the output";

    case jpegls_errc::source_buffer_too_small:
        return "The source buffer is too small, more input data was expected";

    case jpegls_errc::invalid_encoded_data:
        return "Invalid JPEG-LS stream, the encoded bit stream contains a general structural problem";

    case jpegls_errc::too_much_encoded_data:
        return "Invalid JPEG-LS stream, the decoding process is ready but the source buffer still contains encoded data";

    case jpegls_errc::bit_depth_for_transform_not_supported:
        return "The bit depth for the transformation is not supported";

    case jpegls_errc::color_transform_not_supported:
        return "The color transform is not supported";

    case jpegls_errc::encoding_not_supported:
        return "Invalid JPEG-LS stream, the JPEG stream is not encoded with the JPEG-LS algorithm";

    case jpegls_errc::unknown_jpeg_marker_found:
        return "Invalid JPEG-LS stream, an unknown JPEG marker code was found";

    case jpegls_errc::jpeg_marker_start_byte_not_found:
        return "Invalid JPEG-LS stream, the leading start byte 0xFF for a JPEG marker was not found";

    case jpegls_errc::not_enough_memory:
        return "No memory could be allocated for an internal buffer";

    case jpegls_errc::unexpected_failure:
        return "An unexpected internal failure occured";

    case jpegls_errc::invalid_parameter_width:
        return "Invalid JPEG-LS stream, the width (Number of samples per line) is already defined";

    case jpegls_errc::invalid_parameter_height:
        return "Invalid JPEG-LS stream, the height (Number of lines) is already defined";

    case jpegls_errc::invalid_parameter_component_count:
        return "Invalid JPEG-LS stream, component count in the SOF segment is outside the range [1, 255]";

    case jpegls_errc::invalid_parameter_interleave_mode:
        return "Invalid JPEG-LS stream, interleave mode is outside the range [0, 2] or conflicts with component count";
    }

    return nullptr;
}
