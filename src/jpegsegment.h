// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

class JpegStreamWriter;

// Purpose: base class for segments that can be written to JPEG streams.
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
