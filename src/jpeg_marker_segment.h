// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include "jpeg_segment.h"
#include "jpeg_stream_writer.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace charls {

enum class JpegMarkerCode : uint8_t;

class JpegMarkerSegment final : public JpegSegment
{
public:
    /// <summary>
    /// Creates a JPEG-LS Start Of Frame (SOF-55) segment.
    /// </summary>
    /// <param name="width">The width of the frame.</param>
    /// <param name="height">The height of the frame.</param>
    /// <param name="bitsPerSample">The bits per sample.</param>
    /// <param name="componentCount">The component count.</param>
    static std::unique_ptr<JpegMarkerSegment> CreateStartOfFrameSegment(int width, int height, int bitsPerSample, int componentCount);

    /// <summary>
    /// Creates a JPEG File Interchange (APP1 + jfif) segment.
    /// </summary>
    /// <param name="params">Parameters to write into the JFIF segment.</param>
    static std::unique_ptr<JpegMarkerSegment> CreateJpegFileInterchangeFormatSegment(const JfifParameters& params);

    /// <summary>
    /// Creates a JPEG-LS preset parameters (LSE) segment.
    /// </summary>
    /// <param name="params">Parameters to write into the JPEG-LS preset segment.</param>
    static std::unique_ptr<JpegMarkerSegment> CreateJpegLSPresetParametersSegment(const JpegLSPresetCodingParameters& params);

    /// <summary>
    /// Creates a color transformation (APP8) segment.
    /// </summary>
    /// <param name="transformation">Parameters to write into the JFIF segment.</param>
    static std::unique_ptr<JpegMarkerSegment> CreateColorTransformSegment(charls::ColorTransformation transformation);

    /// <summary>
    /// Creates a JPEG-LS Start Of Scan (SOS) segment.
    /// </summary>
    /// <param name="componentIndex">The component index of the scan segment or the start index if component count > 1.</param>
    /// <param name="componentCount">The number of components in the scan segment. Can only be > 1 when the components are interleaved.</param>
    /// <param name="allowedLossyError">The allowed lossy error. 0 means lossless.</param>
    /// <param name="interleaveMode">The interleave mode of the components.</param>
    static std::unique_ptr<JpegMarkerSegment> CreateStartOfScanSegment(int componentIndex, int componentCount, int allowedLossyError, charls::InterleaveMode interleaveMode);

    JpegMarkerSegment(JpegMarkerCode markerCode, std::vector<uint8_t>&& content) :
        markerCode_(markerCode),
        content_(content)
    {
    }

    void Serialize(JpegStreamWriter& streamWriter) override
    {
        streamWriter.WriteByte(0xFF);
        streamWriter.WriteByte(static_cast<uint8_t>(markerCode_));
        streamWriter.WriteWord(static_cast<uint16_t>(content_.size() + 2));
        streamWriter.WriteBytes(content_);
    }

private:
    JpegMarkerCode markerCode_;
    std::vector<uint8_t> content_;
};

} // namespace charls
