// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "jpeg_stream_writer.h"
#include "jpeg_image_data_segment.h"
#include "jpeg_marker_code.h"
#include "jpeg_marker_segment.h"
#include "jpeg_stream_reader.h"
#include <vector>

using std::make_unique;

namespace
{

bool IsDefault(const JpegLSPresetCodingParameters& custom) noexcept
{
    if (custom.MaximumSampleValue != 0)
        return false;

    if (custom.Threshold1 != 0)
        return false;

    if (custom.Threshold2 != 0)
        return false;

    if (custom.Threshold3 != 0)
        return false;

    if (custom.ResetValue != 0)
        return false;

    return true;
}

}

namespace charls
{

JpegStreamWriter::JpegStreamWriter() noexcept
    : data_(),
      byteOffset_(0),
      lastComponentIndex_(0)
{
}


void JpegStreamWriter::AddColorTransform(ColorTransformation transformation)
{
    AddSegment(JpegMarkerSegment::CreateColorTransformSegment(transformation));
}


size_t JpegStreamWriter::Write(const ByteStreamInfo& info)
{
    data_ = info;

    WriteMarker(JpegMarkerCode::StartOfImage);

    for (size_t i = 0; i < segments_.size(); ++i)
    {
        segments_[i]->Serialize(*this);
    }

    WriteMarker(JpegMarkerCode::EndOfImage);

    return byteOffset_;
}


void JpegStreamWriter::AddScan(const ByteStreamInfo& info, const JlsParameters& params)
{
    if (!IsDefault(params.custom))
    {
        AddSegment(JpegMarkerSegment::CreateJpegLSPresetParametersSegment(params.custom));
    }
    else if (params.bitsPerSample > 12)
    {
        const JpegLSPresetCodingParameters preset = ComputeDefault((1 << params.bitsPerSample) - 1, params.allowedLossyError);
        AddSegment(JpegMarkerSegment::CreateJpegLSPresetParametersSegment(preset));
    }

    // Note: it is a common practice to start to count components by index 1.
    lastComponentIndex_ += 1;
    const int componentCount = params.interleaveMode == InterleaveMode::None ? 1 : params.components;
    AddSegment(JpegMarkerSegment::CreateStartOfScanSegment(lastComponentIndex_, componentCount, params.allowedLossyError, params.interleaveMode));

    AddSegment(make_unique<JpegImageDataSegment>(info, params, componentCount));
}

}
