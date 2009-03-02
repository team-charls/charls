// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#include "stdafx.h"
#include "header.h"
#include "streams.h"
#include "decoderstrategy.h"
#include "encoderstrategy.h"


bool IsDefault(const JlsCustomParameters* pcustom)
{
	if (pcustom->MAXVAL != 0)
		return false;

	if (pcustom->T1 != 0)
		return false;

	if (pcustom->T2 != 0)
		return false;

	if (pcustom->T3 != 0)
		return false;

	if (pcustom->RESET != 0)
		return false;

	return true;
}

//
// JpegMarkerSegment
//
class JpegMarkerSegment : public JpegSegment
{
public:
	JpegMarkerSegment(BYTE marker, std::vector<BYTE> vecbyte)
	{
		_marker = marker;
		std::swap(_vecbyte, vecbyte);
	}

	virtual void Write(JLSOutputStream* pstream)
	{
		pstream->WriteByte(0xFF);
		pstream->WriteByte(_marker);
		pstream->WriteWord(USHORT(_vecbyte.size() + 2));
		pstream->WriteBytes(_vecbyte);		
	}

	BYTE _marker;
	std::vector<BYTE> _vecbyte;
};


//
// push_back()
//
void push_back(std::vector<BYTE>& vec, USHORT value)
{
	vec.push_back(BYTE(value / 0x100));
	vec.push_back(BYTE(value % 0x100));
}				   


//
// CreateMarkerStartOfFrame()
//
JpegSegment* CreateMarkerStartOfFrame(Size size, int cbpp, int ccomp)
{
	std::vector<BYTE> vec;
	vec.push_back(static_cast<BYTE>(cbpp));
	push_back(vec, static_cast<USHORT>(size.cy));
	push_back(vec, static_cast<USHORT>(size.cx));
	
	// components
	vec.push_back(static_cast<BYTE>(ccomp));
	for (BYTE icomp = 0; icomp < ccomp; icomp++)
	{
		// rescaling
		vec.push_back(icomp + 1);
		vec.push_back(0x11); 
		//"Tq1" reserved, 0
		vec.push_back(0);		
	}

	return new JpegMarkerSegment(JPEG_SOF, vec);
};




//
// ctor()
//
JLSOutputStream::JLSOutputStream() :
	_pdata(NULL),
	_cbyteOffset(0),
	_cbyteLength(0),
	_icompLast(0),
	_bCompare(false)
{
}



//
// dtor()
//
JLSOutputStream::~JLSOutputStream()
{
	for (UINT i = 0; i < _rgsegment.size(); ++i)
	{
		delete _rgsegment[i];
	}
	_rgsegment.empty();
}




//
// Init()
//
void JLSOutputStream::Init(Size size, int cbpp, int ccomp)
{
		_rgsegment.push_back(CreateMarkerStartOfFrame(size, cbpp, ccomp));
}



//
// Write()
//
int JLSOutputStream::Write(BYTE* pdata, int cbyteLength)
{
	_pdata = pdata;
	_cbyteLength = cbyteLength;

	WriteByte(0xFF);
	WriteByte(JPEG_SOI);
	

	for (UINT i = 0; i < _rgsegment.size(); ++i)
	{
		_rgsegment[i]->Write(this);
	}

	//_bCompare = false;

	WriteByte(0xFF);
	WriteByte(JPEG_EOI);

	return _cbyteOffset;
}



JLSInputStream::JLSInputStream(const BYTE* pdata, int cbyteLength) :
		_pdata(pdata),
		_cbyteOffset(0),
		_cbyteLength(cbyteLength),
		_bCompare(false)
	{
	}


//
// Read()
//
bool JLSInputStream::Read(void* pvoid, int cbyteAvailable)
{
	if (!ReadHeader())
		return false;

	return ReadPixels(pvoid, cbyteAvailable);
}


//
// ReadPixels()
//
bool JLSInputStream::ReadPixels(void* pvoid, int cbyteAvailable)
{
	int cbytePlane = _info.size.cx * _info.size.cy * ((_info.cbit + 7)/8);

	if (cbyteAvailable < cbytePlane * _info.ccomp)
		return false;

 	// line interleave not supported yet
	if (_info.ilv == ILV_LINE)
		return false; 
	
	if (_info.ilv == ILV_NONE)
	{
		BYTE* pbyte = (BYTE*)pvoid;
		for (int icomp = 0; icomp < _info.ccomp; ++icomp)
		{
			ReadScan(pbyte);			
			pbyte += cbytePlane; 
		}	
	}
	else
	{
		ReadScan(pvoid);			
	}
	return true;
	
}


//
// ReadHeader()
//
int JLSInputStream::ReadHeader()
{
	if (ReadByte() != 0xFF)
		return 0;

	if (ReadByte() != JPEG_SOI)
		return 0;
	
	for (;;)
	{
		if (ReadByte() != 0xFF)
			return 0;

		BYTE marker = (BYTE)ReadByte();

		int cbyteStart = _cbyteOffset;
		int cbyteMarker = ReadWord();

		switch (marker)
		{
			case JPEG_SOS: ReadStartOfScan();  break;
			case JPEG_SOF: ReadStartOfFrame(); break;
			case JPEG_COM: ReadComment();	   break;
			case JPEG_LSE: ReadPresetParameters();	break;

			// Other tags not supported (among which DNL DRI)
			default:
			return 0;
		}

		if (marker == JPEG_SOS)
		{				
			_cbyteOffset = cbyteStart - 2;
			return _cbyteOffset;
		}
		_cbyteOffset = cbyteStart + cbyteMarker;
	}
}


JpegMarkerSegment* EncodeStartOfScan(const JlsParamaters* pparams, int icomponent)
{
	BYTE itable		= 0;
	
	std::vector<BYTE> rgbyte;

	if (icomponent < 0)
	{
		rgbyte.push_back((BYTE)pparams->components);
		for (int icomponent = 0; icomponent < pparams->components; ++icomponent )
		{
			rgbyte.push_back(BYTE(icomponent + 1));
			rgbyte.push_back(itable);
		}
	}
	else
	{
		rgbyte.push_back(1);
		rgbyte.push_back((BYTE)icomponent);
		rgbyte.push_back(itable);	
	}

	rgbyte.push_back(BYTE(pparams->allowedlossyerror));
	rgbyte.push_back(BYTE(pparams->ilv));
	rgbyte.push_back(0); // transform

	return new JpegMarkerSegment(JPEG_SOS, rgbyte);
}



JpegMarkerSegment* CreateLSE(const JlsCustomParameters* pcustom)
{
	std::vector<BYTE> rgbyte;

	rgbyte.push_back(1);
	push_back(rgbyte, (USHORT)pcustom->MAXVAL);
	push_back(rgbyte, (USHORT)pcustom->T1);
	push_back(rgbyte, (USHORT)pcustom->T2);
	push_back(rgbyte, (USHORT)pcustom->T3);
	push_back(rgbyte, (USHORT)pcustom->RESET);
	
	return new JpegMarkerSegment(JPEG_LSE, rgbyte);
}

//
// ReadPresetParameters()
//
void JLSInputStream::ReadPresetParameters()
{
	int type = ReadByte();


	switch (type)
	{
	case 1:
		{
			_presets.MAXVAL = ReadWord();
			_presets.T1 = ReadWord();
			_presets.T2 = ReadWord();
			_presets.T3 = ReadWord();
			_presets.RESET = ReadWord();
			return;
		}
	}

	
}


//
// ReadStartOfScan()
//
void JLSInputStream::ReadStartOfScan()
{
	int ccomp = ReadByte();
	for (int i = 0; i < ccomp; ++i)
	{
		ReadByte();
		ReadByte();
	}
	_info.nnear = ReadByte();
	_info.ilv = interleavemode(ReadByte());
}


//
// ReadComment()
//
void JLSInputStream::ReadComment()
{}


//
// ReadStartOfFrame()
//
void JLSInputStream::ReadStartOfFrame()
{
	_info.cbit = ReadByte();
	int cline = ReadWord();
	int ccol = ReadWord();
	_info.size = Size(ccol, cline);
	_info.ccomp = ReadByte();
	
}


//
// ReadByte()
//
int JLSInputStream::ReadByte()
{ return _pdata[_cbyteOffset++]; }


//
// ReadWord()
//
int JLSInputStream::ReadWord()
{
	int i = ReadByte() * 256;
	return i + ReadByte();
}


void JLSInputStream::ReadScan(void* pvout) 
{
	std::auto_ptr<DecoderStrategy> qcodec(JlsCodecFactory<DecoderStrategy>().GetCodec(_info, _presets));
	_cbyteOffset += qcodec->DecodeScan(pvout, _info.size, _pdata + _cbyteOffset, _cbyteLength - _cbyteOffset, _bCompare); 
};


class JpegImageDataSegment: public JpegSegment
{
public:
	JpegImageDataSegment(const void* pvoidRaw, Size size, int cbit, int icompStart, int ccompScan, interleavemode ilv, int accuracy, const JlsCustomParameters& presets)  :
		_cbit(cbit), 
		_nnear(accuracy),
		_size(size),
		_ccompScan(ccompScan),
		_ilv(ilv),
		_icompStart(icompStart),
		_pvoidRaw(pvoidRaw),
		_presets(presets)
	{
	}


	void Write(JLSOutputStream* pstream)
	{		

		ScanInfo info;
		info.cbit = _cbit;
		info.ccomp = _ccompScan;
		info.ilv = _ilv;
		info.nnear = _nnear;
		
		std::auto_ptr<EncoderStrategy> qcodec(JlsCodecFactory<EncoderStrategy>().GetCodec(info, _presets));
		int cbyteWritten = qcodec->EncodeScan((BYTE*)_pvoidRaw, _size, pstream->GetPos(), pstream->GetLength(), pstream->_bCompare ? pstream->GetPos() : NULL); 
		pstream->Seek(cbyteWritten);
	}



	const void* _pvoidRaw;
	Size _size;
	int _cbit;
	int _ccompScan;
	interleavemode _ilv;
	int _icompStart;
	int _nnear;

	JlsCustomParameters _presets;
};



void JLSOutputStream::AddScan(const void* pbyteComp, const JlsParamaters* pparams)
{
	if (!IsDefault(&pparams->custom))
	{
		_rgsegment.push_back(CreateLSE(&pparams->custom));		
	}

	_icompLast += 1;
	_rgsegment.push_back(EncodeStartOfScan(pparams,pparams->ilv == ILV_NONE ? _icompLast : -1));

	Size size = Size(pparams->width, pparams->height);
	int ccomp = pparams->ilv == ILV_NONE ? 1 : pparams->components;

	
	_rgsegment.push_back(new JpegImageDataSegment(pbyteComp, size, pparams->bitspersample, _icompLast, ccomp, pparams->ilv, pparams->allowedlossyerror, pparams->custom));
}
