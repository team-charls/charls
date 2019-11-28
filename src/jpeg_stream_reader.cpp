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

using std::find;
using std::vector;
using std::unique_ptr;
using namespace charls;

namespace {

void CheckParameterCoherent(const JlsParameters& params)
{
    switch (params.components)
    {
    case 4:
    case 3:
        break;
    default:
        if (params.interleaveMode != interleave_mode::none)
            throw jpegls_error{jpegls_errc::parameter_value_not_supported};

        break;
    }
}

} // namespace

namespace charls {

JpegStreamReader::JpegStreamReader(ByteStreamInfo byteStreamInfo) noexcept :
    byteStream_{byteStreamInfo}
{
}


void JpegStreamReader::Read(ByteStreamInfo rawPixels)
{
    ASSERT(state_ == state::bit_stream_section);

    CheckParameterCoherent(params_);

    if (rect_.Width <= 0)
    {
        rect_.Width = params_.width;
        rect_.Height = params_.height;
    }

    const int64_t bytesPerPlane = static_cast<int64_t>(rect_.Width) * rect_.Height * ((params_.bitsPerSample + 7) / 8);

    if (rawPixels.rawData && static_cast<int64_t>(rawPixels.count) < bytesPerPlane * params_.components)
        throw jpegls_error{jpegls_errc::destination_buffer_too_small};

    int componentIndex{};
    while (componentIndex < params_.components)
    {
        if (state_ == state::scan_section)
        {
            ReadStartOfScan(componentIndex == 0);
        }

        unique_ptr<DecoderStrategy> codec = JlsCodecFactory<DecoderStrategy>().CreateCodec(params_, preset_coding_parameters_);
        unique_ptr<ProcessLine> processLine(codec->CreateProcess(rawPixels));
        codec->DecodeScan(move(processLine), rect_, byteStream_);
        SkipBytes(rawPixels, static_cast<size_t>(bytesPerPlane));
        state_ = state::scan_section;

        if (params_.interleaveMode != interleave_mode::none)
            return;

        componentIndex++;
    }
}


void JpegStreamReader::ReadNBytes(std::vector<char>& destination, int byteCount)
{
    for (int i = 0; i < byteCount; ++i)
    {
        destination.push_back(static_cast<char>(ReadByte()));
    }
}


void JpegStreamReader::ReadHeader(spiff_header* header, bool* spiff_header_found)
{
    ASSERT(state_ != state::scan_section);

    if (state_ == state::before_start_of_image)
    {
        if (ReadNextMarkerCode() != JpegMarkerCode::StartOfImage)
            throw jpegls_error{jpegls_errc::start_of_image_marker_not_found};

        state_ = state::header_section;
    }

    for (;;)
    {
        const JpegMarkerCode markerCode = ReadNextMarkerCode();
        ValidateMarkerCode(markerCode);

        if (markerCode == JpegMarkerCode::StartOfScan)
        {
            state_ = state::scan_section;
            return;
        }

        const int32_t segmentSize = ReadSegmentSize();
        int bytesRead;
        switch (state_)
        {
        case state::spiff_header_section:
            bytesRead = ReadSpiffDirectoryEntry(markerCode, segmentSize - 2) + 2;
            break;

        default:
            bytesRead = ReadMarkerSegment(markerCode, segmentSize - 2, header, spiff_header_found) + 2;
            break;
        }

        const int paddingToRead = segmentSize - bytesRead;
        if (paddingToRead < 0)
            throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

        for (int i = 0; i < paddingToRead; ++i)
        {
            ReadByte();
        }

        if (state_ == state::header_section && spiff_header_found && *spiff_header_found)
        {
            state_ = state::spiff_header_section;
            return;
        }
    }
}


JpegMarkerCode JpegStreamReader::ReadNextMarkerCode()
{
    auto byte = ReadByte();
    if (byte != JpegMarkerStartByte)
        throw jpegls_error{jpegls_errc::jpeg_marker_start_byte_not_found};

    // Read all preceding 0xFF fill values until a non 0xFF value has been found. (see T.81, B.1.1.2)
    do
    {
        byte = ReadByte();
    } while (byte == JpegMarkerStartByte);

    return static_cast<JpegMarkerCode>(byte);
}


void JpegStreamReader::ValidateMarkerCode(JpegMarkerCode markerCode)
{
    // ISO/IEC 14495-1, C.1.1. defines the following markers as valid for a JPEG-LS byte stream:
    // SOF55, LSE, SOI, EOI, SOS, DNL, DRI, RSTm, APPn and COM.
    // All other markers shall not be present.
    switch (markerCode)
    {
    case JpegMarkerCode::StartOfFrameJpegLS:
    case JpegMarkerCode::JpegLSPresetParameters:
    case JpegMarkerCode::StartOfScan:
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
        throw jpegls_error{jpegls_errc::encoding_not_supported};

    case JpegMarkerCode::StartOfImage:
        throw jpegls_error{jpegls_errc::duplicate_start_of_image_marker};

    case JpegMarkerCode::EndOfImage:
        throw jpegls_error{jpegls_errc::unexpected_end_of_image_marker};
    }

    throw jpegls_error{jpegls_errc::unknown_jpeg_marker_found};
}


int JpegStreamReader::ReadMarkerSegment(JpegMarkerCode markerCode, int32_t segmentSize, spiff_header* header, bool* spiff_header_found)
{
    switch (markerCode)
    {
    case JpegMarkerCode::StartOfFrameJpegLS:
        return ReadStartOfFrameSegment(segmentSize);

    case JpegMarkerCode::Comment:
        return ReadComment();

    case JpegMarkerCode::JpegLSPresetParameters:
        return ReadPresetParametersSegment(segmentSize);

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
        return TryReadApplicationData8Segment(segmentSize, header, spiff_header_found);

    // Other tags not supported (among which DNL DRI)
    default:
        ASSERT(false);
        return 0;
    }
}

int JpegStreamReader::ReadSpiffDirectoryEntry(JpegMarkerCode markerCode, int32_t segmentSize)
{
    if (markerCode != JpegMarkerCode::ApplicationData8)
        throw jpegls_error{jpegls_errc::missing_end_of_spiff_directory};

    if (segmentSize < 4)
        throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

    const uint32_t spiffDirectoryType = ReadUInt32();
    if (spiffDirectoryType == spiff_end_of_directory_entry_type)
    {
        state_ = state::image_section;
    }

    return 4;
}

int JpegStreamReader::ReadStartOfFrameSegment(int32_t segmentSize)
{
    // A JPEG-LS Start of Frame (SOF) segment is documented in ISO/IEC 14495-1, C.2.2
    // This section references ISO/IEC 10918-1, B.2.2, which defines the normal JPEG SOF,
    // with some modifications.

    if (segmentSize < 6)
        throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

    params_.bitsPerSample = ReadByte();
    if (params_.bitsPerSample < MinimumBitsPerSample || params_.bitsPerSample > MaximumBitsPerSample)
        throw jpegls_error{jpegls_errc::invalid_parameter_bits_per_sample};

    params_.height = ReadUInt16();
    if (params_.height < 1)
        throw jpegls_error{jpegls_errc::parameter_value_not_supported};

    params_.width = ReadUInt16();
    if (params_.width < 1)
        throw jpegls_error{jpegls_errc::parameter_value_not_supported};

    params_.components = ReadByte();
    if (params_.components < 1)
        throw jpegls_error{jpegls_errc::invalid_parameter_component_count};

    if (segmentSize != 6 + (params_.components * 3))
        throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

    for (auto i = 0; i < params_.components; ++i)
    {
        // Component specification parameters
        AddComponent(ReadByte());                                    // Ci = Component identifier
        const uint8_t horizontalVerticalSamplingFactor = ReadByte(); // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
        if (horizontalVerticalSamplingFactor != 0x11)
            throw jpegls_error{jpegls_errc::parameter_value_not_supported};

        SkipByte(); // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
    }

    return segmentSize;
}


int JpegStreamReader::ReadComment() noexcept
{
    return 0;
}


int JpegStreamReader::ReadPresetParametersSegment(int32_t segmentSize)
{
    if (segmentSize < 1)
        throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

    const auto type = static_cast<JpegLSPresetParametersType>(ReadByte());

    switch (type)
    {
    case JpegLSPresetParametersType::PresetCodingParameters:
    {
        constexpr int32_t CodingParameterSegmentSize = 11;
        if (segmentSize != CodingParameterSegmentSize)
            throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

        preset_coding_parameters_.maximum_sample_value = ReadUInt16();
        preset_coding_parameters_.threshold1 = ReadUInt16();
        preset_coding_parameters_.threshold2 = ReadUInt16();
        preset_coding_parameters_.threshold3 = ReadUInt16();
        preset_coding_parameters_.reset_value = ReadUInt16();

        return CodingParameterSegmentSize;
    }

    case JpegLSPresetParametersType::MappingTableSpecification:
    case JpegLSPresetParametersType::MappingTableContinuation:
    case JpegLSPresetParametersType::ExtendedWidthAndHeight:
        throw jpegls_error{jpegls_errc::parameter_value_not_supported};

    case JpegLSPresetParametersType::CodingMethodSpecification:
    case JpegLSPresetParametersType::NearLosslessErrorReSpecification:
    case JpegLSPresetParametersType::VisuallyOrientedQuantizationSpecification:
    case JpegLSPresetParametersType::ExtendedPredictionSpecification:
    case JpegLSPresetParametersType::StartOfFixedLengthCoding:
    case JpegLSPresetParametersType::EndOfFixedLengthCoding:
    case JpegLSPresetParametersType::ExtendedPresetCodingParameters:
    case JpegLSPresetParametersType::InverseColorTransformSpecification:
        throw jpegls_error{jpegls_errc::jpegls_preset_extended_parameter_type_not_supported};
    }

    throw jpegls_error{jpegls_errc::invalid_jpegls_preset_parameter_type};
}


void JpegStreamReader::ReadStartOfScan(bool firstComponent)
{
    if (!firstComponent)
    {
        const JpegMarkerCode markerCode = ReadNextMarkerCode();
        if (markerCode != JpegMarkerCode::StartOfScan)
            throw jpegls_error{jpegls_errc::invalid_encoded_data};
    }

    const int32_t segmentSize = ReadSegmentSize();
    if (segmentSize < 6)
        throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

    const int componentCountInScan = ReadByte();
    if (componentCountInScan != 1 && componentCountInScan != params_.components)
        throw jpegls_error{jpegls_errc::parameter_value_not_supported};

    if (segmentSize < 6 + (2 * componentCountInScan))
        throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

    for (int i = 0; i < componentCountInScan; ++i)
    {
        ReadByte(); // Read Scan component selector
        ReadByte(); // Read Mapping table selector
    }

    params_.allowedLossyError = ReadByte();                            // Read NEAR parameter
    params_.interleaveMode = static_cast<interleave_mode>(ReadByte()); // Read ILV parameter
    if (!(params_.interleaveMode == interleave_mode::none || params_.interleaveMode == interleave_mode::line || params_.interleaveMode == interleave_mode::sample))
        throw jpegls_error{jpegls_errc::invalid_parameter_interleave_mode};

    if ((ReadByte() & 0xF) != 0) // Read Ah (no meaning) and Al (point transform).
        throw jpegls_error{jpegls_errc::parameter_value_not_supported};

    if (params_.stride == 0)
    {
        const int width = rect_.Width != 0 ? rect_.Width : params_.width;
        const int components = params_.interleaveMode == interleave_mode::none ? 1 : params_.components;
        params_.stride = components * width * ((params_.bitsPerSample + 7) / 8);
    }

    state_ = state::bit_stream_section;
}


uint8_t JpegStreamReader::ReadByte()
{
    if (byteStream_.rawStream)
        return static_cast<uint8_t>(byteStream_.rawStream->sbumpc());

    if (byteStream_.count == 0)
        throw jpegls_error{jpegls_errc::source_buffer_too_small};

    const uint8_t value = byteStream_.rawData[0];
    SkipBytes(byteStream_, 1);
    return value;
}


void JpegStreamReader::SkipByte()
{
    static_cast<void>(ReadByte());
}


int JpegStreamReader::ReadUInt16()
{
    const int i = ReadByte() * 256;
    return i + ReadByte();
}

uint32_t JpegStreamReader::ReadUInt32()
{
    uint32_t value = ReadUInt16();
    value = value << 16;
    value += ReadUInt16();

    return value;
}

int32_t JpegStreamReader::ReadSegmentSize()
{
    const int32_t segmentSize = ReadUInt16();
    if (segmentSize < 2)
        throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

    return segmentSize;
}

int JpegStreamReader::TryReadApplicationData8Segment(int32_t segmentSize, spiff_header* header, bool* spiff_header_found)
{
    if (spiff_header_found)
    {
        ASSERT(header);
        *spiff_header_found = false;
    }

    if (segmentSize == 5)
        return TryReadHPColorTransformSegment();

    if (header && spiff_header_found && segmentSize >= 30)
        return TryReadSpiffHeaderSegment(header, *spiff_header_found);

    return 0;
}


int JpegStreamReader::TryReadHPColorTransformSegment()
{
    vector<char> sourceTag;
    ReadNBytes(sourceTag, 4);
    if (strncmp(sourceTag.data(), "mrfx", 4) != 0) // mrfx = xfrm (in big endian) = colorXFoRM
        return 4;

    const auto colorTransformation = ReadByte();
    switch (colorTransformation)
    {
    case static_cast<uint8_t>(color_transformation::none):
    case static_cast<uint8_t>(color_transformation::hp1):
    case static_cast<uint8_t>(color_transformation::hp2):
    case static_cast<uint8_t>(color_transformation::hp3):
        params_.colorTransformation = static_cast<color_transformation>(colorTransformation);
        return 5;

    case 4: // RgbAsYuvLossy (The standard lossy RGB to YCbCr transform used in JPEG.)
    case 5: // Matrix (transformation is controlled using a matrix that is also stored in the segment.
        throw jpegls_error{jpegls_errc::color_transform_not_supported};

    default:
        throw jpegls_error{jpegls_errc::invalid_encoded_data};
    }
}

template<typename EnumType, EnumType Low, EnumType High>
EnumType enum_cast(uint8_t value)
{
    if (value < static_cast<uint8_t>(Low))
        throw jpegls_error{jpegls_errc::invalid_encoded_data};

    if (value > static_cast<uint8_t>(High))
        throw jpegls_error{jpegls_errc::invalid_encoded_data};

    return static_cast<EnumType>(value);
}

int JpegStreamReader::TryReadSpiffHeaderSegment(spiff_header* header, bool& spiff_header_found)
{
    vector<char> sourceTag;
    ReadNBytes(sourceTag, 6);
    if (strncmp(sourceTag.data(), "SPIFF", 6) != 0)
        return 6;

    const auto high_version = ReadByte();
    if (high_version > spiff_major_revision_number)
        return 7; // Treat unknown versions as if the SPIFF header doesn't exists.

    SkipByte(); // low version

    header->profile_id = static_cast<spiff_profile_id>(ReadByte());
    header->component_count = ReadByte();
    header->height = ReadUInt32();
    header->width = ReadUInt32();
    header->color_space = static_cast<spiff_color_space>(ReadByte());
    header->bits_per_sample = ReadByte();
    header->compression_type = static_cast<spiff_compression_type>(ReadByte());
    header->resolution_units = static_cast<spiff_resolution_units>(ReadByte());
    header->vertical_resolution = ReadUInt32();
    header->horizontal_resolution = ReadUInt32();

    spiff_header_found = true;
    return 30;
}


void JpegStreamReader::AddComponent(uint8_t componentId)
{
    if (find(componentIds_.cbegin(), componentIds_.cend(), componentId) != componentIds_.cend())
        throw jpegls_error{jpegls_errc::duplicate_component_id_in_sof_segment};

    componentIds_.push_back(componentId);
}


} // namespace charls
