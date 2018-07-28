//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use.
//

#ifndef CHARLS_JPEG_SEGMENT
#define CHARLS_JPEG_SEGMENT

class JpegStreamWriter;

//
// Purpose: base class for segments that can be written to JPEG streams.
//
class JpegSegment
{
public:
    virtual ~JpegSegment() = default;
    virtual void Serialize(JpegStreamWriter& streamWriter) = 0;

    JpegSegment(const JpegSegment&) = delete;
    JpegSegment(JpegSegment&&) = delete;
    JpegSegment& operator=(const JpegSegment&) = delete;
    JpegSegment& operator=(JpegSegment&&) = delete;

protected:
    JpegSegment() = default;
};

#endif
