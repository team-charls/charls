// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

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

using std::vector;
using std::find;
using namespace charls;

namespace
{
// JFIF\0
uint8_t jfifID[] = {'J', 'F', 'I', 'F', '\0'};


void CheckParameterCoherent(const JlsParameters& params)
{
    switch (params.components)
    {
    case 4:
    case 3:
        break;
    default:
        if (params.interleaveMode != InterleaveMode::None)
            throw jpegls_error{jpegls_errc::parameter_value_not_supported};

        break;
    }
}

} // namespace

namespace charls
{
JpegStreamReader::JpegStreamReader(ByteStreamInfo byteStreamInfo) noexcept
    : byteStream_{byteStreamInfo}
{
}


void JpegStreamReader::Read(ByteStreamInfo rawPixels)
{
    ReadHeader();
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
        ReadStartOfScan(componentIndex == 0);

        std::unique_ptr<DecoderStrategy> codec = JlsCodecFactory<DecoderStrategy>().CreateCodec(params_, params_.custom);
        std::unique_ptr<ProcessLine> processLine(codec->CreateProcess(rawPixels));
        codec->DecodeScan(move(processLine), rect_, byteStream_);
        SkipBytes(rawPixels, static_cast<size_t>(bytesPerPlane));

        if (params_.interleaveMode != InterleaveMode::None)
            return;

        componentIndex += 1;
    }
}


void JpegStreamReader::ReadNBytes(std::vector<char>& destination, int byteCount)
{
    for (int i = 0; i < byteCount; ++i)
    {
        destination.push_back(static_cast<char>(ReadByte()));
    }
}


void JpegStreamReader::ReadHeader()
{
    if (ReadNextMarkerCode() != JpegMarkerCode::StartOfImage)
        throw jpegls_error{jpegls_errc::start_of_image_marker_not_found};

    for (;;)
    {
        const JpegMarkerCode markerCode = ReadNextMarkerCode();
        ValidateMarkerCode(markerCode);

        if (markerCode == JpegMarkerCode::StartOfScan)
            return;

        const int32_t segmentSize = ReadSegmentSize();
        const int bytesRead = ReadMarkerSegment(markerCode, segmentSize - 2) + 2;
        const int paddingToRead = segmentSize - bytesRead;
        if (paddingToRead < 0)
            throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

        for (int i = 0; i < paddingToRead; ++i)
        {
            ReadByte();
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


void JpegStreamReader::ValidateMarkerCode(JpegMarkerCode markerCode) const
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


int JpegStreamReader::ReadMarkerSegment(JpegMarkerCode markerCode, int32_t segmentSize)
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
        return TryReadHPColorTransformSegment(segmentSize);

    // Other tags not supported (among which DNL DRI)
    default:
        ASSERT(false);
        return 0;
    }
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

        params_.custom.MaximumSampleValue = ReadUInt16();
        params_.custom.Threshold1 = ReadUInt16();
        params_.custom.Threshold2 = ReadUInt16();
        params_.custom.Threshold3 = ReadUInt16();
        params_.custom.ResetValue = ReadUInt16();
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
            throw jpegls_error{jpegls_errc::invalid_encoded_data}; // TODO: throw more specific error code.
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

    params_.allowedLossyError = ReadByte();                           // Read NEAR parameter
    params_.interleaveMode = static_cast<InterleaveMode>(ReadByte()); // Read ILV parameter
    if (!(params_.interleaveMode == InterleaveMode::None || params_.interleaveMode == InterleaveMode::Line || params_.interleaveMode == InterleaveMode::Sample))
        throw jpegls_error{jpegls_errc::invalid_parameter_interleave_mode};

    if ((ReadByte() & 0xF) != 0) // Read Ah (no meaning) and Al (point transform).
        throw jpegls_error{jpegls_errc::parameter_value_not_supported};

    if (params_.stride == 0)
    {
        const int width = rect_.Width != 0 ? rect_.Width : params_.width;
        const int components = params_.interleaveMode == InterleaveMode::None ? 1 : params_.components;
        params_.stride = components * width * ((params_.bitsPerSample + 7) / 8);
    }
}


void JpegStreamReader::ReadJfif()
{
    for (int i = 0; i < static_cast<int>(sizeof(jfifID)); i++)
    {
        if (jfifID[i] != ReadByte())
            return;
    }
    params_.jfif.version = ReadUInt16();

    // DPI or DPcm
    params_.jfif.units = ReadByte();
    params_.jfif.Xdensity = ReadUInt16();
    params_.jfif.Ydensity = ReadUInt16();

    // thumbnail
    params_.jfif.Xthumbnail = ReadByte();
    params_.jfif.Ythumbnail = ReadByte();
    if (params_.jfif.Xthumbnail > 0 && params_.jfif.thumbnail)
    {
        std::vector<char> tempBuffer(static_cast<char*>(params_.jfif.thumbnail),
                                     static_cast<char*>(params_.jfif.thumbnail) + static_cast<size_t>(3) * params_.jfif.Xthumbnail * params_.jfif.Ythumbnail);
        ReadNBytes(tempBuffer, 3 * params_.jfif.Xthumbnail * params_.jfif.Ythumbnail);
    }
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


int32_t JpegStreamReader::ReadSegmentSize()
{
    const int32_t segmentSize = ReadUInt16();
    if (segmentSize < 2)
        throw jpegls_error{jpegls_errc::invalid_marker_segment_size};

    return segmentSize;
}


int JpegStreamReader::TryReadHPColorTransformSegment(int32_t segmentSize)
{
    if (segmentSize < 5)
        return 0;

    vector<char> sourceTag;
    ReadNBytes(sourceTag, 4);
    if (strncmp(sourceTag.data(), "mrfx", 4) != 0)
        return 4;

    const auto colorTransformation = ReadByte();
    switch (colorTransformation)
    {
    case static_cast<uint8_t>(ColorTransformation::None):
    case static_cast<uint8_t>(ColorTransformation::HP1):
    case static_cast<uint8_t>(ColorTransformation::HP2):
    case static_cast<uint8_t>(ColorTransformation::HP3):
        params_.colorTransformation = static_cast<ColorTransformation>(colorTransformation);
        return 5;

    case 4: // RgbAsYuvLossy (The standard lossy RGB to YCbCr transform used in JPEG.)
    case 5: // Matrix (transformation is controlled using a matrix that is also stored in the segment.
        throw jpegls_error{jpegls_errc::color_transform_not_supported};

    default:
        throw jpegls_error{jpegls_errc::invalid_encoded_data};
    }
}


void JpegStreamReader::AddComponent(uint8_t componentId)
{
    if (find(componentIds_.cbegin(), componentIds_.cend(), componentId) != componentIds_.cend())
        throw jpegls_error{jpegls_errc::duplicate_component_id_in_sof_segment};

    componentIds_.push_back(componentId);
}


} // namespace charls
