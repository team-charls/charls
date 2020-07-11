// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "jpeg_stream_reader.h"

#include "constants.h"
#include "decoder_strategy.h"
#include "encoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_marker_code.h"
#include "jpegls_preset_parameters_type.h"
#include "util.h"

#include <algorithm>
#include <iomanip>
#include <memory>

using charls::impl::throw_jpegls_error;
using std::find;
using std::unique_ptr;
using std::vector;

namespace charls {

jpeg_stream_reader::jpeg_stream_reader(byte_stream_info byte_stream_info) noexcept :
    byteStream_{byte_stream_info}
{
}


void jpeg_stream_reader::read(byte_stream_info source, uint32_t stride)
{
    ASSERT(state_ == state::bit_stream_section);

    check_parameter_coherent();

    if (rect_.Width <= 0)
    {
        rect_.Width = static_cast<int32_t>(frame_info_.width);
        rect_.Height = static_cast<int32_t>(frame_info_.height);
    }

    if (stride == 0)
    {
        const uint32_t width = rect_.Width != 0 ? static_cast<uint32_t>(rect_.Width) : frame_info_.width;
        const uint32_t component_count = parameters_.interleave_mode == interleave_mode::none ? 1U : static_cast<uint32_t>(frame_info_.component_count);
        stride = component_count * width * ((static_cast<uint32_t>(frame_info_.bits_per_sample) + 7U) / 8U);
    }

    const int64_t bytesPerPlane = static_cast<int64_t>(rect_.Width) * rect_.Height * ((frame_info_.bits_per_sample + 7) / 8);

    if (source.rawData && static_cast<int64_t>(source.count) < bytesPerPlane * frame_info_.component_count)
        throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

    int componentIndex{};
    while (componentIndex < frame_info_.component_count)
    {
        if (state_ == state::scan_section)
        {
            read_next_start_of_scan();
        }

        unique_ptr<decoder_strategy> codec = jls_codec_factory<decoder_strategy>().create_codec(frame_info_, parameters_, preset_coding_parameters_);
        unique_ptr<process_line> processLine(codec->create_process(source, stride));
        codec->decode_scan(move(processLine), rect_, byteStream_);
        skip_bytes(source, static_cast<size_t>(bytesPerPlane));
        state_ = state::scan_section;

        if (parameters_.interleave_mode != interleave_mode::none)
            return;

        componentIndex++;
    }
}


void jpeg_stream_reader::read_bytes(std::vector<char>& destination, const int byte_count)
{
    for (int i = 0; i < byte_count; ++i)
    {
        destination.push_back(static_cast<char>(read_byte()));
    }
}


void jpeg_stream_reader::read_header(spiff_header* header, bool* spiff_header_found)
{
    ASSERT(state_ != state::scan_section);

    if (state_ == state::before_start_of_image)
    {
        if (read_next_marker_code() != JpegMarkerCode::StartOfImage)
            throw_jpegls_error(jpegls_errc::start_of_image_marker_not_found);

        state_ = state::header_section;
    }

    for (;;)
    {
        const JpegMarkerCode marker_code = read_next_marker_code();
        validate_marker_code(marker_code);

        if (marker_code == JpegMarkerCode::StartOfScan)
        {
            if (!is_maximum_sample_value_valid())
                throw_jpegls_error(jpegls_errc::invalid_parameter_jpegls_pc_parameters);

            state_ = state::scan_section;
            return;
        }

        const int32_t segmentSize = read_segment_size();
        int bytesRead;
        switch (state_)
        {
        case state::spiff_header_section:
            bytesRead = read_spiff_directory_entry(marker_code, segmentSize - 2) + 2;
            break;

        default:
            bytesRead = read_marker_segment(marker_code, segmentSize - 2, header, spiff_header_found) + 2;
            break;
        }

        const int paddingToRead = segmentSize - bytesRead;
        if (paddingToRead < 0)
            throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

        for (int i = 0; i < paddingToRead; ++i)
        {
            read_byte();
        }

        if (state_ == state::header_section && spiff_header_found && *spiff_header_found)
        {
            state_ = state::spiff_header_section;
            return;
        }
    }
}


void jpeg_stream_reader::read_next_start_of_scan()
{
    ASSERT(state_ == state::scan_section);

    for (;;)
    {
        const JpegMarkerCode markerCode = read_next_marker_code();
        validate_marker_code(markerCode);

        if (markerCode == JpegMarkerCode::StartOfScan)
        {
            read_start_of_scan();
            return;
        }

        const int32_t segmentSize = read_segment_size();
        const int bytesRead = read_marker_segment(markerCode, segmentSize - 2) + 2;

        const int paddingToRead = segmentSize - bytesRead;
        if (paddingToRead < 0)
            throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

        for (int i = 0; i < paddingToRead; ++i)
        {
            read_byte();
        }
    }
}


JpegMarkerCode jpeg_stream_reader::read_next_marker_code()
{
    auto byte = read_byte();
    if (byte != JpegMarkerStartByte)
        throw_jpegls_error(jpegls_errc::jpeg_marker_start_byte_not_found);

    // Read all preceding 0xFF fill values until a non 0xFF value has been found. (see T.81, B.1.1.2)
    do
    {
        byte = read_byte();
    } while (byte == JpegMarkerStartByte);

    return static_cast<JpegMarkerCode>(byte);
}


void jpeg_stream_reader::validate_marker_code(const JpegMarkerCode marker_code) const
{
    // ISO/IEC 14495-1, C.1.1. defines the following markers as valid for a JPEG-LS byte stream:
    // SOF55, LSE, SOI, EOI, SOS, DNL, DRI, RSTm, APPn and COM.
    // All other markers shall not be present.
    switch (marker_code)
    {
    case JpegMarkerCode::StartOfScan:
        if (state_ != state::scan_section)
            throw_jpegls_error(jpegls_errc::unexpected_marker_found);

        return;

    case JpegMarkerCode::StartOfFrameJpegLS:
        if (state_ == state::scan_section)
            throw_jpegls_error(jpegls_errc::duplicate_start_of_frame_marker);

        return;

    case JpegMarkerCode::JpegLSPresetParameters:
    case JpegMarkerCode::Comment:
    case JpegMarkerCode::ApplicationData0:
    case JpegMarkerCode::ApplicationData1:
    case JpegMarkerCode::ApplicationData2:
    case JpegMarkerCode::ApplicationData3:
    case JpegMarkerCode::ApplicationData4:
    case JpegMarkerCode::ApplicationData5:
    case JpegMarkerCode::ApplicationData6:
    case JpegMarkerCode::ApplicationData7:
    case JpegMarkerCode::ApplicationData8:
    case JpegMarkerCode::ApplicationData9:
    case JpegMarkerCode::ApplicationData10:
    case JpegMarkerCode::ApplicationData11:
    case JpegMarkerCode::ApplicationData12:
    case JpegMarkerCode::ApplicationData13:
    case JpegMarkerCode::ApplicationData14:
    case JpegMarkerCode::ApplicationData15:
        return;

    // Check explicit for one of the other common JPEG encodings.
    case JpegMarkerCode::StartOfFrameBaselineJpeg:
    case JpegMarkerCode::StartOfFrameExtendedSequential:
    case JpegMarkerCode::StartOfFrameProgressive:
    case JpegMarkerCode::StartOfFrameLossless:
    case JpegMarkerCode::StartOfFrameDifferentialSequential:
    case JpegMarkerCode::StartOfFrameDifferentialProgressive:
    case JpegMarkerCode::StartOfFrameDifferentialLossless:
    case JpegMarkerCode::StartOfFrameExtendedArithmetic:
    case JpegMarkerCode::StartOfFrameProgressiveArithmetic:
    case JpegMarkerCode::StartOfFrameLosslessArithmetic:
    case JpegMarkerCode::StartOfFrameJpegLSExtended:
        throw_jpegls_error(jpegls_errc::encoding_not_supported);

    case JpegMarkerCode::StartOfImage:
        throw_jpegls_error(jpegls_errc::duplicate_start_of_image_marker);

    case JpegMarkerCode::EndOfImage:
        throw_jpegls_error(jpegls_errc::unexpected_end_of_image_marker);
    }

    throw_jpegls_error(jpegls_errc::unknown_jpeg_marker_found);
}


int jpeg_stream_reader::read_marker_segment(const JpegMarkerCode marker_code,
                                            const int32_t segment_size,
                                            spiff_header* header,
                                            bool* spiff_header_found)
{
    switch (marker_code)
    {
    case JpegMarkerCode::StartOfFrameJpegLS:
        return read_start_of_frame_segment(segment_size);

    case JpegMarkerCode::Comment:
        return read_comment();

    case JpegMarkerCode::JpegLSPresetParameters:
        return read_preset_parameters_segment(segment_size);

    case JpegMarkerCode::ApplicationData0:
    case JpegMarkerCode::ApplicationData1:
    case JpegMarkerCode::ApplicationData2:
    case JpegMarkerCode::ApplicationData3:
    case JpegMarkerCode::ApplicationData4:
    case JpegMarkerCode::ApplicationData5:
    case JpegMarkerCode::ApplicationData6:
    case JpegMarkerCode::ApplicationData7:
    case JpegMarkerCode::ApplicationData9:
    case JpegMarkerCode::ApplicationData10:
    case JpegMarkerCode::ApplicationData11:
    case JpegMarkerCode::ApplicationData12:
    case JpegMarkerCode::ApplicationData13:
    case JpegMarkerCode::ApplicationData14:
    case JpegMarkerCode::ApplicationData15:
        return 0;

    case JpegMarkerCode::ApplicationData8:
        return try_read_application_data8_segment(segment_size, header, spiff_header_found);

    // Other tags not supported (among which DNL DRI)
    default:
        ASSERT(false);
        return 0;
    }
}

int jpeg_stream_reader::read_spiff_directory_entry(const JpegMarkerCode marker_code, const int32_t segment_size)
{
    if (marker_code != JpegMarkerCode::ApplicationData8)
        throw_jpegls_error(jpegls_errc::missing_end_of_spiff_directory);

    if (segment_size < 4)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    const uint32_t spiffDirectoryType = read_uint32();
    if (spiffDirectoryType == spiff_end_of_directory_entry_type)
    {
        state_ = state::image_section;
    }

    return 4;
}

int jpeg_stream_reader::read_start_of_frame_segment(const int32_t segment_size)
{
    // A JPEG-LS Start of Frame (SOF) segment is documented in ISO/IEC 14495-1, C.2.2
    // This section references ISO/IEC 10918-1, B.2.2, which defines the normal JPEG SOF,
    // with some modifications.

    if (segment_size < 6)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    frame_info_.bits_per_sample = read_byte();
    if (frame_info_.bits_per_sample < MinimumBitsPerSample || frame_info_.bits_per_sample > MaximumBitsPerSample)
        throw_jpegls_error(jpegls_errc::invalid_parameter_bits_per_sample);

    frame_info_.height = read_uint16();
    if (frame_info_.height < 1)
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    frame_info_.width = read_uint16();
    if (frame_info_.width < 1)
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    frame_info_.component_count = read_byte();
    if (frame_info_.component_count < 1)
        throw_jpegls_error(jpegls_errc::invalid_parameter_component_count);

    if (segment_size != 6 + (frame_info_.component_count * 3))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    for (auto i = 0; i < frame_info_.component_count; ++i)
    {
        // Component specification parameters
        add_component(read_byte());                                   // Ci = Component identifier
        const uint8_t horizontalVerticalSamplingFactor = read_byte(); // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
        if (horizontalVerticalSamplingFactor != 0x11)
            throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

        skip_byte(); // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
    }

    state_ = state::scan_section;

    return segment_size;
}


int jpeg_stream_reader::read_comment() noexcept
{
    return 0;
}


int jpeg_stream_reader::read_preset_parameters_segment(const int32_t segment_size)
{
    if (segment_size < 1)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    const auto type = static_cast<JpegLSPresetParametersType>(read_byte());

    switch (type)
    {
    case JpegLSPresetParametersType::PresetCodingParameters: {
        constexpr int32_t CodingParameterSegmentSize = 11;
        if (segment_size != CodingParameterSegmentSize)
            throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

        preset_coding_parameters_.maximum_sample_value = read_uint16();
        preset_coding_parameters_.threshold1 = read_uint16();
        preset_coding_parameters_.threshold2 = read_uint16();
        preset_coding_parameters_.threshold3 = read_uint16();
        preset_coding_parameters_.reset_value = read_uint16();

        return CodingParameterSegmentSize;
    }

    case JpegLSPresetParametersType::MappingTableSpecification:
    case JpegLSPresetParametersType::MappingTableContinuation:
    case JpegLSPresetParametersType::ExtendedWidthAndHeight:
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    case JpegLSPresetParametersType::CodingMethodSpecification:
    case JpegLSPresetParametersType::NearLosslessErrorReSpecification:
    case JpegLSPresetParametersType::VisuallyOrientedQuantizationSpecification:
    case JpegLSPresetParametersType::ExtendedPredictionSpecification:
    case JpegLSPresetParametersType::StartOfFixedLengthCoding:
    case JpegLSPresetParametersType::EndOfFixedLengthCoding:
    case JpegLSPresetParametersType::ExtendedPresetCodingParameters:
    case JpegLSPresetParametersType::InverseColorTransformSpecification:
        throw_jpegls_error(jpegls_errc::jpegls_preset_extended_parameter_type_not_supported);
    }

    throw_jpegls_error(jpegls_errc::invalid_jpegls_preset_parameter_type);
}


void jpeg_stream_reader::read_start_of_scan()
{
    const int32_t segmentSize = read_segment_size();
    if (segmentSize < 6)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    const int componentCountInScan = read_byte();
    if (componentCountInScan != 1 && componentCountInScan != frame_info_.component_count)
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    if (segmentSize < 6 + (2 * componentCountInScan))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    for (int i = 0; i < componentCountInScan; ++i)
    {
        read_byte(); // Read Scan component selector
        read_byte(); // Read Mapping table selector
    }

    parameters_.near_lossless = read_byte(); // Read NEAR parameter
    if (parameters_.near_lossless > MaximumNearLossless(static_cast<int>(maximum_sample_value())))
        throw_jpegls_error(jpegls_errc::invalid_parameter_near_lossless);

    const auto mode = static_cast<interleave_mode>(read_byte()); // Read ILV parameter
    if (!(mode == interleave_mode::none || mode == interleave_mode::line || mode == interleave_mode::sample))
        throw_jpegls_error(jpegls_errc::invalid_parameter_interleave_mode);
    parameters_.interleave_mode = mode;

    if ((read_byte() & 0xFU) != 0) // Read Ah (no meaning) and Al (point transform).
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    state_ = state::bit_stream_section;
}


uint8_t jpeg_stream_reader::read_byte()
{
    if (byteStream_.rawStream)
        return static_cast<uint8_t>(byteStream_.rawStream->sbumpc());

    if (byteStream_.count == 0)
        throw_jpegls_error(jpegls_errc::source_buffer_too_small);

    const uint8_t value = byteStream_.rawData[0];
    skip_bytes(byteStream_, 1);
    return value;
}


void jpeg_stream_reader::skip_byte()
{
    static_cast<void>(read_byte());
}


uint16_t jpeg_stream_reader::read_uint16()
{
    const uint16_t value = read_byte() * 256;
    return value + read_byte();
}

uint32_t jpeg_stream_reader::read_uint32()
{
    uint32_t value = read_uint16();
    value = value << 16U;
    value += read_uint16();

    return value;
}

int32_t jpeg_stream_reader::read_segment_size()
{
    const int32_t segmentSize = read_uint16();
    if (segmentSize < 2)
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);

    return segmentSize;
}

int jpeg_stream_reader::try_read_application_data8_segment(const int32_t segment_size,
                                                           spiff_header* header,
                                                           bool* spiff_header_found)
{
    if (spiff_header_found)
    {
        ASSERT(header);
        *spiff_header_found = false;
    }

    if (segment_size == 5)
        return try_read_hp_color_transform_segment();

    if (header && spiff_header_found && segment_size >= 30)
        return try_read_spiff_header_segment(*header, *spiff_header_found);

    return 0;
}


int jpeg_stream_reader::try_read_hp_color_transform_segment()
{
    vector<char> sourceTag;
    read_bytes(sourceTag, 4);
    if (strncmp(sourceTag.data(), "mrfx", 4) != 0) // mrfx = xfrm (in big endian) = colorXFoRM
        return 4;

    const auto colorTransformation = read_byte();
    switch (colorTransformation)
    {
    case static_cast<uint8_t>(color_transformation::none):
    case static_cast<uint8_t>(color_transformation::hp1):
    case static_cast<uint8_t>(color_transformation::hp2):
    case static_cast<uint8_t>(color_transformation::hp3):
        parameters_.transformation = static_cast<color_transformation>(colorTransformation);
        return 5;

    case 4: // RgbAsYuvLossy (The standard lossy RGB to YCbCr transform used in JPEG.)
    case 5: // Matrix (transformation is controlled using a matrix that is also stored in the segment.
        throw_jpegls_error(jpegls_errc::color_transform_not_supported);

    default:
        throw_jpegls_error(jpegls_errc::invalid_encoded_data);
    }
}

template<typename EnumType, EnumType Low, EnumType High>
EnumType enum_cast(uint8_t value)
{
    if (value < static_cast<uint8_t>(Low))
        throw_jpegls_error(jpegls_errc::invalid_encoded_data);

    if (value > static_cast<uint8_t>(High))
        throw_jpegls_error(jpegls_errc::invalid_encoded_data);

    return static_cast<EnumType>(value);
}

int jpeg_stream_reader::try_read_spiff_header_segment(OUT_ spiff_header& header, OUT_ bool& spiff_header_found)
{
    vector<char> sourceTag;
    read_bytes(sourceTag, 6);
    if (strncmp(sourceTag.data(), "SPIFF\0", 6) != 0)
    {
        header = {};
        spiff_header_found = false;
        return 6;
    }

    const auto high_version = read_byte();
    if (high_version > spiff_major_revision_number)
    {
        header = {};
        spiff_header_found = false;
        return 7; // Treat unknown versions as if the SPIFF header doesn't exists.
    }

    skip_byte(); // low version

    header.profile_id = static_cast<spiff_profile_id>(read_byte());
    header.component_count = read_byte();
    header.height = read_uint32();
    header.width = read_uint32();
    header.color_space = static_cast<spiff_color_space>(read_byte());
    header.bits_per_sample = read_byte();
    header.compression_type = static_cast<spiff_compression_type>(read_byte());
    header.resolution_units = static_cast<spiff_resolution_units>(read_byte());
    header.vertical_resolution = read_uint32();
    header.horizontal_resolution = read_uint32();

    spiff_header_found = true;
    return 30;
}


void jpeg_stream_reader::add_component(const uint8_t component_id)
{
    if (find(componentIds_.cbegin(), componentIds_.cend(), component_id) != componentIds_.cend())
        throw_jpegls_error(jpegls_errc::duplicate_component_id_in_sof_segment);

    componentIds_.push_back(component_id);
}


void jpeg_stream_reader::check_parameter_coherent() const
{
    switch (frame_info_.component_count)
    {
    case 4:
    case 3:
        break;
    default:
        if (parameters_.interleave_mode != interleave_mode::none)
            throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

        break;
    }
}


bool jpeg_stream_reader::is_maximum_sample_value_valid() const noexcept
{
    return preset_coding_parameters_.maximum_sample_value == 0 ||
           static_cast<uint32_t>(preset_coding_parameters_.maximum_sample_value) <= calculate_maximum_sample_value(frame_info_.bits_per_sample);
}


uint32_t jpeg_stream_reader::maximum_sample_value() const noexcept
{
    ASSERT(is_maximum_sample_value_valid());

    if (preset_coding_parameters_.maximum_sample_value != 0)
        return static_cast<uint32_t>(preset_coding_parameters_.maximum_sample_value);

    return calculate_maximum_sample_value(frame_info_.bits_per_sample);
}


} // namespace charls
