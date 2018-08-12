//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#ifndef CHARLS_JPEG_IMAGE_DATA_SEGMENT
#define CHARLS_JPEG_IMAGE_DATA_SEGMENT

#include "jpegsegment.h"
#include "publictypes.h"

class JpegImageDataSegment : public JpegSegment
{
public:
    JpegImageDataSegment(ByteStreamInfo rawStream, const JlsParameters& params, int componentCount) noexcept
        : _componentCount(componentCount),
          _rawStreamInfo(rawStream),
          _params(params)
    {
    }

    void Serialize(JpegStreamWriter& streamWriter) override;

private:
    int _componentCount;
    ByteStreamInfo _rawStreamInfo;
    JlsParameters _params;
};

#endif
