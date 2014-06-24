//
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
//
#ifndef CHARLS_JPEGMARKER
#define CHARLS_JPEGMARKER

#include <memory>
#include <vector>
#include "util.h"


//
// JpegStreamReader: minimal implementation to read a JPEG byte stream.
//
class JpegStreamReader
{
public:
	JpegStreamReader(ByteStreamInfo byteStreamInfo);

	const JlsParameters& GetMetadata() const
	{
		return _info;
	}

	const JlsCustomParameters& GetCustomPreset() const
	{
		return _info.custom;
	}

	void Read(ByteStreamInfo info);
	void ReadHeader();

	void EnableCompare(bool bCompare)
	{
		_bCompare = bCompare;
	}

	void SetInfo(JlsParameters* info)
	{
		_info = *info;
	}

	void SetRect(const JlsRect& rect)
	{
		_rect = rect;
	}

	void ReadStartOfScan(bool firstComponent);
	BYTE ReadByte();

private:
	void ReadScan(ByteStreamInfo rawPixels);
	int ReadPresetParameters();
	int ReadComment();
	int ReadStartOfFrame();
	int ReadWord();
	void ReadNBytes(std::vector<char>& dst, int byteCount);
	int ReadMarker(BYTE marker);

	// JFIF
	void ReadJfif();
	// Color Transform Application Markers & Code Stream (HP extension)
	int ReadColorSpace();
	int ReadColorXForm();

private:
	ByteStreamInfo _byteStream;
	bool _bCompare;
	JlsParameters _info;
	JlsRect _rect;
};


#endif
