//
// (C) CharLS Team 2014, all rights reserved. See the accompanying "License.txt" for licensed use. 
//

#pragma once

#include "util.h"
#include "jpegsegment.h"
#include "jpegstreamwriter.h"
#include <vector>


class JpegMarkerSegment : public JpegSegment
{
public:
	JpegMarkerSegment(BYTE marker, std::vector<BYTE>&& content) :
		_marker(marker),
		_content(content)
	{
	}

	virtual void Serialize(JpegStreamWriter& streamWriter)
	{
		streamWriter.WriteByte(0xFF);
		streamWriter.WriteByte(_marker);
		streamWriter.WriteWord(USHORT(_content.size() + 2));
		streamWriter.WriteBytes(_content);
	}

	static JpegMarkerSegment* CreateStartOfFrameMarker(Size size, LONG bitsPerSample, LONG componentCount);
	static JpegMarkerSegment* CreateJpegFileInterchangeFormatMarker(const JfifParameters& jfif);
	static JpegMarkerSegment* CreateJpegLSExtendedParametersMarker(const JlsCustomParameters& pcustom);
	static JpegMarkerSegment* CreateColorTransformMarker(int i);
	static JpegMarkerSegment* CreateStartOfScanMarker(const JlsParameters* pparams, LONG icomponent);

private:
	BYTE _marker;
	std::vector<BYTE> _content;
};
