//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#ifndef CHARLS_JPEGIMAGEDATASEGMENT
#define CHARLS_JPEGIMAGEDATASEGMENT

#include "jpegsegment.h"
#include "jpegstreamwriter.h"

class JpegImageDataSegment : public JpegSegment
{
public:
    JpegImageDataSegment(ByteStreamInfo rawStream, const JlsParameters& info, int componentCount) :
        _componentCount(componentCount),
        _rawStreamInfo(rawStream),
        _info(info)
    {
    }

    void Serialize(JpegStreamWriter& streamWriter) override;

private:
    int _componentCount;
    ByteStreamInfo _rawStreamInfo;
    JlsParameters _info;
};

#endif
