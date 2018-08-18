//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use.
//
#ifndef CHARLS_JPEG_STREAM_READER
#define CHARLS_JPEG_STREAM_READER

#include "publictypes.h"
#include <cstdint>
#include <vector>


enum class JpegMarkerCode : uint8_t;
struct JlsParameters;
class JpegCustomParameters;


JpegLSPresetCodingParameters ComputeDefault(int32_t maximumSampleValue, int32_t allowedLossyError) noexcept;


//
// JpegStreamReader: minimal implementation to read a JPEG byte stream.
//
class JpegStreamReader final
{
public:
    explicit JpegStreamReader(ByteStreamInfo byteStreamInfo) noexcept;

    const JlsParameters& GetMetadata() const noexcept
    {
        return _params;
    }

    const JpegLSPresetCodingParameters& GetCustomPreset() const noexcept
    {
        return _params.custom;
    }

    void Read(ByteStreamInfo rawPixels);
    void ReadHeader();

    void SetInfo(const JlsParameters& params) noexcept
    {
        _params = params;
    }

    void SetRect(const JlsRect& rect) noexcept
    {
        _rect = rect;
    }

    void ReadStartOfScan(bool firstComponent);
    uint8_t ReadByte();

private:
    int ReadUInt16();
    int32_t ReadSegmentSize();
    void ReadNBytes(std::vector<char>& dst, int byteCount);
    JpegMarkerCode ReadNextMarkerCode();
    void ValidateMarkerCode(JpegMarkerCode markerCode) const;

    int ReadMarkerSegment(JpegMarkerCode markerCode, int32_t segmentSize);
    int ReadPresetParameters();
    static int ReadComment() noexcept;
    int ReadStartOfFrameSegment(int32_t segmentSize);
    void ReadJfif();
    int TryReadHPColorTransformSegment(int32_t segmentSize);

    ByteStreamInfo _byteStream;
    JlsParameters _params;
    JlsRect _rect;
};


#endif
