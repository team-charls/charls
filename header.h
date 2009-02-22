// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once
#include "streams.h"

#define JPEG_SOI  0xD8
#define JPEG_EOI  0xD9
#define JPEG_SOS  0xDA

#define JPEG_SOF  0xF7
#define JPEG_LSE  0xF8
#define JPEG_DNL  0xDC
#define JPEG_DRI  0xDD
#define JPEG_RSTm  0xD0
#define JPEG_COM  0xFE

class JLSOutputStream;


template<class STRATEGY>
class JlsCodecFactory 
{
public:	
	STRATEGY* GetCodec(const ScanInfo& info, const Presets&);
private:
	STRATEGY* GetCodecImpl(const ScanInfo& info);
};


//
// JpegSegment
//
class JpegSegment
{
protected:
	JpegSegment() {}
public:
	virtual ~JpegSegment() {}
	virtual void Write(JLSOutputStream* pheader) = 0;
};

