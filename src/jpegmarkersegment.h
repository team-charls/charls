//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#ifndef CHARLS_JPEGMARKERSEGMENT
#define CHARLS_JPEGMARKERSEGMENT

#include "jpegsegment.h"
#include "jpegstreamwriter.h"
#include <vector>
#include <cstdint>


enum class JpegMarkerCode : uint8_t;


class JpegMarkerSegment : public JpegSegment
{
public:
    /// <summary>
    /// Creates a JPEG-LS Start Of Frame (SOF-55) segment.
    /// </summary>
    /// <param name="width">The width of the frame.</param>
    /// <param name="height">The height of the frame.</param>
    /// <param name="bitsPerSample">The bits per sample.</param>
    /// <param name="componentCount">The component count.</param>
    static JpegMarkerSegment* CreateStartOfFrameSegment(int width, int height, int bitsPerSample, int componentCount);

    /// <summary>
    /// Creates a JPEG File Interchange (APP1 + jfif) segment.
    /// </summary>
    /// <param name="jfif">Parameters to write into the JFIF segment.</param>
    static JpegMarkerSegment* CreateJpegFileInterchangeFormatSegment(const JfifParameters& jfif);

    /// <summary>
    /// Creates a JPEG-LS extended parameters () segment.
    /// </summary>
    /// <param name="jfif">Parameters to write into the JFIF segment.</param>
    static JpegMarkerSegment* CreateJpegLSExtendedParametersSegment(const JlsCustomParameters& pcustom);

    /// <summary>
    /// Creates a color transformation () segment.
    /// </summary>
    /// <param name="jfif">Parameters to write into the JFIF segment.</param>
    static JpegMarkerSegment* CreateColorTransformSegment(charls::ColorTransformation transformation);

    /// <summary>
    /// Creates a JPEG-LS Start Of Scan (SOS) segment.
    /// </summary>
    /// <param name="componentIndex">The component index of the scan segment or the start index if component count > 1.</param>
    /// <param name="componentCount">The number of components in the scan segment. Can only be > 1 when the components are interleaved.</param>
    /// <param name="allowedLossyError">The allowed lossy error. 0 means lossless</param>
    /// <param name="interleaveMode">The interleave mode of the components.</param>
    static JpegMarkerSegment* CreateStartOfScanSegment(int componentIndex, int componentCount, int allowedLossyError, charls::InterleaveMode interleaveMode);

    JpegMarkerSegment(JpegMarkerCode marker, std::vector<uint8_t>&& content) :
        _marker(marker),
        _content(content)
    {
    }

    virtual void Serialize(JpegStreamWriter& streamWriter) override
    {
        streamWriter.WriteByte(0xFF);
        streamWriter.WriteByte(static_cast<uint8_t>(_marker));
        streamWriter.WriteWord(static_cast<uint16_t>(_content.size() + 2));
        streamWriter.WriteBytes(_content);
    }

private:
    JpegMarkerCode _marker;
    std::vector<uint8_t> _content;
};

#endif
