//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#include "jpegstreamreader.h"
#include "util.h"
#include "jpegstreamwriter.h"
#include "jpegimagedatasegment.h"
#include "jpegmarkercode.h"
#include "decoderstrategy.h"
#include "encoderstrategy.h"
#include "jlscodecfactory.h"
#include "constants.h"
#include <memory>
#include <iomanip>
#include <algorithm>

using namespace charls;

extern template class JlsCodecFactory<EncoderStrategy>;
extern template class JlsCodecFactory<DecoderStrategy>;

namespace {


// JFIF\0
uint8_t jfifID[] = { 'J', 'F', 'I', 'F', '\0' };


/// <summary>Clamping function as defined by ISO/IEC 14495-1, Figure C.3</summary>
int32_t clamp(int32_t i, int32_t j, int32_t maximumSampleValue) noexcept
{
    if (i > maximumSampleValue || i < j)
        return j;

    return i;
}

 
ApiResult CheckParameterCoherent(const JlsParameters& params) noexcept
{
    if (params.bitsPerSample < 2 || params.bitsPerSample > 16)
        return ApiResult::ParameterValueNotSupported;

    if (params.interleaveMode < InterleaveMode::None || params.interleaveMode > InterleaveMode::Sample)
        return ApiResult::InvalidCompressedData;

    switch (params.components)
    {
        case 4: return params.interleaveMode == InterleaveMode::Sample ? ApiResult::ParameterValueNotSupported : ApiResult::OK;
        case 3: return ApiResult::OK;
        case 0: return ApiResult::InvalidJlsParameters;

        default: return params.interleaveMode != InterleaveMode::None ? ApiResult::ParameterValueNotSupported : ApiResult::OK;
    }
}

} // namespace


JpegLSPresetCodingParameters ComputeDefault(int32_t maximumSampleValue, int32_t allowedLossyError) noexcept
{
    const int32_t factor = (std::min(maximumSampleValue, 4095) + 128) / 256;
    const int threshold1 = clamp(factor * (DefaultThreshold1 - 2) + 2 + 3 * allowedLossyError, allowedLossyError + 1, maximumSampleValue);
    const int threshold2 = clamp(factor * (DefaultThreshold2 - 3) + 3 + 5 * allowedLossyError, threshold1, maximumSampleValue); //-V537

    return
    {
        maximumSampleValue,
        threshold1,
        threshold2,
        clamp(factor * (DefaultThreshold3 - 4) + 4 + 7 * allowedLossyError, threshold2, maximumSampleValue),
        DefaultResetValue
    };
}


void JpegImageDataSegment::Serialize(JpegStreamWriter& streamWriter)
{
    JlsParameters info = _params;
    info.components = _componentCount;
    auto codec = JlsCodecFactory<EncoderStrategy>().CreateCodec(info, _params.custom);
    std::unique_ptr<ProcessLine> processLine(codec->CreateProcess(_rawStreamInfo));
    ByteStreamInfo compressedData = streamWriter.OutputStream();
    const size_t cbyteWritten = codec->EncodeScan(move(processLine), compressedData);
    streamWriter.Seek(cbyteWritten);
}


JpegStreamReader::JpegStreamReader(ByteStreamInfo byteStreamInfo) noexcept :
    _byteStream(byteStreamInfo),
    _params(),
    _rect()
{
}


void JpegStreamReader::Read(ByteStreamInfo rawPixels)
{
    ReadHeader();

    const auto result = CheckParameterCoherent(_params);
    if (result != ApiResult::OK)
        throw charls_error(result);

    if (_rect.Width <= 0)
    {
        _rect.Width = _params.width;
        _rect.Height = _params.height;
    }

    const int64_t bytesPerPlane = static_cast<int64_t>(_rect.Width) * _rect.Height * ((_params.bitsPerSample + 7)/8);

    if (rawPixels.rawData && static_cast<int64_t>(rawPixels.count) < bytesPerPlane * _params.components)
        throw charls_error(ApiResult::UncompressedBufferTooSmall);

    int componentIndex = 0;

    while (componentIndex < _params.components)
    {
        ReadStartOfScan(componentIndex == 0);

        std::unique_ptr<DecoderStrategy> qcodec = JlsCodecFactory<DecoderStrategy>().CreateCodec(_params, _params.custom);
        std::unique_ptr<ProcessLine> processLine(qcodec->CreateProcess(rawPixels));
        qcodec->DecodeScan(move(processLine), _rect, _byteStream);
        SkipBytes(rawPixels, static_cast<size_t>(bytesPerPlane));

        if (_params.interleaveMode != InterleaveMode::None)
            return;

        componentIndex += 1;
    }
}


void JpegStreamReader::ReadNBytes(std::vector<char>& dst, int byteCount)
{
    for (int i = 0; i < byteCount; ++i)
    {
        dst.push_back(static_cast<char>(ReadByte()));
    }
}


void JpegStreamReader::ReadHeader()
{
    if (ReadNextMarkerCode() != JpegMarkerCode::StartOfImage)
        throw charls_error(ApiResult::InvalidCompressedData,
            "Invalid JPEG stream, first marker code is not SOI");

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
            throw charls_error(ApiResult::InvalidCompressedData);

        for (int i = 0; i < paddingToRead; ++i)
        {
            ReadByte();
        }
    }
}


JpegMarkerCode JpegStreamReader::ReadNextMarkerCode()
{
    auto byte = ReadByte();
    if (byte != 0xFF)
    {
        std::ostringstream message;
        message << std::setfill('0');
        message << "Expected JPEG Marker start byte 0xFF but the byte value was 0x" << std::hex << std::uppercase
                << std::setw(2) << static_cast<unsigned int>(byte);
        throw charls_error(ApiResult::MissingJpegMarkerStart, message.str());
    }

    // Read all preceding 0xFF fill values until a non 0xFF value has been found. (see T.81, B.1.1.2)
    do
    {
        byte = ReadByte();
    } while (byte == 0xFF);

    return static_cast<JpegMarkerCode>(byte);
}


void JpegStreamReader::ValidateMarkerCode(JpegMarkerCode markerCode) const
{
    // ISO/IEC 14495-1, ITU-T Recommendation T.87, C.1.1. defines the following markers valid for a JPEG-LS byte stream:
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
            break;

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
            {
                std::ostringstream message;
                message << "JPEG encoding with marker " << static_cast<unsigned int>(markerCode) << " is not supported.";
                throw charls_error(ApiResult::UnsupportedEncoding, message.str());
            }

        case JpegMarkerCode::StartOfImage:
        case JpegMarkerCode::EndOfImage:
            {
                std::ostringstream message;
                message << "Invalid JPEG stream, marker " << static_cast<unsigned int>(markerCode) << " invalid in current state.";
                throw charls_error(ApiResult::InvalidCompressedData, message.str());
            }

        default:
            {
                std::ostringstream message;
                message << "Unknown JPEG marker " << static_cast<unsigned int>(markerCode) << " encountered.";
                throw charls_error(ApiResult::UnknownJpegMarker, message.str());
            }
    }
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
            return ReadPresetParameters();

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


int JpegStreamReader::ReadPresetParameters()
{
    const int type = ReadByte();

    switch (type)
    {
    case 0x1:
        {
            _params.custom.MaximumSampleValue = ReadUInt16();
            _params.custom.Threshold1 = ReadUInt16();
            _params.custom.Threshold2 = ReadUInt16();
            _params.custom.Threshold3 = ReadUInt16();
            _params.custom.ResetValue = ReadUInt16();
            return 11;
        }

    case 0x2: // mapping table specification
    case 0x3: // mapping table continuation
    case 0x4: // X and Y parameters greater than 16 bits are defined.
        {
            std::ostringstream message;
            message << "JPEG-LS preset parameters with type " << static_cast<unsigned int>(type) << " is not supported.";
            throw charls_error(ApiResult::UnsupportedEncoding, message.str());
        }

    case 0x5: // JPEG-LS Extended (ISO/IEC 14495-2): Coding method specification
    case 0x6: // JPEG-LS Extended (ISO/IEC 14495-2): NEAR value re-specification
    case 0x7: // JPEG-LS Extended (ISO/IEC 14495-2): Visually oriented quantization specification
    case 0x8: // JPEG-LS Extended (ISO/IEC 14495-2): Extended prediction specification
    case 0x9: // JPEG-LS Extended (ISO/IEC 14495-2): Specification of the start of fixed length coding
    case 0xA: // JPEG-LS Extended (ISO/IEC 14495-2): Specification of the end of fixed length coding
    case 0xC: // JPEG-LS Extended (ISO/IEC 14495-2): JPEG-LS preset coding parameters
    case 0xD: // JPEG-LS Extended (ISO/IEC 14495-2): Inverse color transform specification
        {
            std::ostringstream message;
            message << "JPEG-LS Extended (ISO/IEC 14495-2) preset parameter with type " << static_cast<unsigned int>(type) << " is not supported.";
            throw charls_error(ApiResult::UnsupportedEncoding, message.str());
        }

    default:
        {
            std::ostringstream message;
            message << "JPEG-LS preset parameters with invalid type " << static_cast<unsigned int>(type) << " encountered.";
            throw charls_error(ApiResult::InvalidJlsParameters, message.str());
        }
    }
}


void JpegStreamReader::ReadStartOfScan(bool firstComponent)
{
    if (!firstComponent)
    {
        const JpegMarkerCode markerCode = ReadNextMarkerCode();
        if (markerCode != JpegMarkerCode::StartOfScan)
            throw charls_error(ApiResult::InvalidCompressedData);// TODO: throw more specific error code.
    }

    const int32_t segmentSize = ReadSegmentSize();
    if (segmentSize < 6)
        throw charls_error(ApiResult::InvalidCompressedData,
                           "Invalid segment size, SOS segment size needs to be at least 6");

    const int componentCountInScan = ReadByte();
    if (componentCountInScan != 1 && componentCountInScan != _params.components)
        throw charls_error(ApiResult::ParameterValueNotSupported);

    if (segmentSize < 6 + (2 * componentCountInScan))
        throw charls_error(ApiResult::InvalidCompressedData,
                           "Invalid segment size, SOS segment size needs to be at least 6 + 2 * Ns");

    for (int i = 0; i < componentCountInScan; ++i)
    {
        ReadByte(); // Read Scan component selector
        ReadByte(); // Read Mapping table selector
    }

    _params.allowedLossyError = ReadByte(); // Read NEAR parameter
    _params.interleaveMode = static_cast<InterleaveMode>(ReadByte()); // Read ILV parameter
    if (!(_params.interleaveMode == InterleaveMode::None || _params.interleaveMode == InterleaveMode::Line || _params.interleaveMode == InterleaveMode::Sample))
        throw charls_error(ApiResult::InvalidCompressedData);// TODO: throw more specific error code.
    if (ReadByte() != 0) // Read Ah and Al.
        throw charls_error(ApiResult::InvalidCompressedData);// TODO: throw more specific error code.

    if(_params.stride == 0)
    {
        const int width = _rect.Width != 0 ? _rect.Width : _params.width;
        const int components = _params.interleaveMode == InterleaveMode::None ? 1 : _params.components;
        _params.stride = components * width * ((_params.bitsPerSample + 7) / 8);
    }
}


int JpegStreamReader::ReadComment() noexcept
{
    return 0;
}


void JpegStreamReader::ReadJfif()
{
    for(int i = 0; i < static_cast<int>(sizeof(jfifID)); i++)
    {
        if(jfifID[i] != ReadByte())
            return;
    }
    _params.jfif.version   = ReadUInt16();

    // DPI or DPcm
    _params.jfif.units = ReadByte();
    _params.jfif.Xdensity = ReadUInt16();
    _params.jfif.Ydensity = ReadUInt16();

    // thumbnail
    _params.jfif.Xthumbnail = ReadByte();
    _params.jfif.Ythumbnail = ReadByte();
    if(_params.jfif.Xthumbnail > 0 && _params.jfif.thumbnail)
    {
        std::vector<char> tempbuff(static_cast<char*>(_params.jfif.thumbnail),
            static_cast<char*>(_params.jfif.thumbnail) + static_cast<size_t>(3) * _params.jfif.Xthumbnail * _params.jfif.Ythumbnail);
        ReadNBytes(tempbuff, 3*_params.jfif.Xthumbnail*_params.jfif.Ythumbnail);
    }
}


int JpegStreamReader::ReadStartOfFrameSegment(int32_t segmentSize)
{
    if (segmentSize < 6)
        throw charls_error(ApiResult::InvalidCompressedData,
                           "Invalid segment size, SOF_55 segment size needs to be at least 6");

    _params.bitsPerSample = ReadByte();
    _params.height = ReadUInt16();
    _params.width = ReadUInt16();
    _params.components= ReadByte();

    // Note: component specific parameters are currently not verified.

    return 6;
}


uint8_t JpegStreamReader::ReadByte()
{
    if (_byteStream.rawStream)
        return static_cast<uint8_t>(_byteStream.rawStream->sbumpc());

    if (_byteStream.count == 0)
        throw charls_error(ApiResult::CompressedBufferTooSmall);

    const uint8_t value = _byteStream.rawData[0];
    SkipBytes(_byteStream, 1);
    return value;
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
        throw charls_error(ApiResult::InvalidCompressedData,
                           "Invalid segment size, segment size needs to be at least 2");

    return segmentSize;
}


int JpegStreamReader::TryReadHPColorTransformSegment(int32_t segmentSize)
{
    if (segmentSize < 5)
        return 0;

    std::vector<char> sourceTag;
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
            _params.colorTransformation = static_cast<ColorTransformation>(colorTransformation);
            return 5;

        case 4: // RgbAsYuvLossy (The standard lossy RGB to YCbCr transform used in JPEG.)
        case 5: // Matrix (transformation is controlled using a matrix that is also stored in the segment.
            throw charls_error(ApiResult::ImageTypeNotSupported);

        default:
            throw charls_error(ApiResult::InvalidCompressedData);
    }
}
