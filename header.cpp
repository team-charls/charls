// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#include "stdafx.h"
#include "header.h"
#include "streams.h"
#include "decoderstrategy.h"
#include "encoderstrategy.h"

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

	virtual void Write(JLSOutputStream* pheader)
	{
		pheader->WriteByte(0xFF);
		pheader->WriteByte(_marker);
		pheader->WriteWord(USHORT(_vecbyte.size() + 2));
		pheader->WriteBytes(_vecbyte);		
	}

	BYTE _marker;
	std::vector<BYTE> _vecbyte;
};


//
// AppendWord()
//
void AppendWord(std::vector<BYTE>& vec, USHORT value)
{
	vec.push_back(BYTE(value / 0x100));
	vec.push_back(BYTE(value % 0x100));
}				   


//
// CreateMarkerStartOfFrame()
//
JpegSegment* CreateMarkerStartOfFrame(int ccol, int cline, int cbpp, int ccomp)
{
	std::vector<BYTE> vec;
	vec.push_back(static_cast<BYTE>(cbpp));
	AppendWord(vec, static_cast<USHORT>(cline));
	AppendWord(vec, static_cast<USHORT>(ccol));
	
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
	_icompLast(0)
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
void JLSOutputStream::Init(int ccomp, int ccol, int cline, int cbpp)
{
		_rgsegment.push_back(CreateMarkerStartOfFrame(ccomp, ccol, cline, cbpp));
}
//
// Write()
//
int JLSOutputStream::Write(BYTE* pdata, int cbyteLength, const void* /*pbyteExpected*/)
{
	_pdata = pdata;
	_cbyteLength = cbyteLength;

	WriteByte(0xFF);
	WriteByte(JPEG_SOI);
	
//	pbyteExpected += 2;

	for (UINT i = 0; i < _rgsegment.size(); ++i)
	{
		_rgsegment[i]->Write(this);
	}

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
bool JLSInputStream::Read(void* pvoid)
{
	if (!ReadHeader())
		return false;

	return ReadPixels(pvoid);
}


//
// ReadPixels()
//
bool JLSInputStream::ReadPixels(void* pvoid)
{

 	// line interleave not supported yet
	if (_info.ilv == ILV_LINE)
		return false; 
	
	if (_info.ilv == ILV_NONE)
	{
		BYTE* pbyte = (BYTE*)pvoid;
		for (int icomp = 0; icomp < _info.ccomp; ++icomp)
		{
			ReadScan(pbyte);			
			pbyte += _info.size.cx * _info.size.cy * ((_info.cbit + 7)/8); 
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
	_info.size = Size(cline, ccol);
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
	JpegImageDataSegment(const void* pvoidRaw, Size size, int cbit, int icompStart, int ccompScan, interleavemode ilv, int accuracy)  :
		_cbit(cbit), 
		_nnear(accuracy),
		_size(size),
		_ccompScan(ccompScan),
		_ilv(ilv),
		_icompStart(icompStart),
		_pvoidRaw(pvoidRaw)
	{
	}


	void Write(JLSOutputStream* pstream)
	{		
		EncodeScanHeader(pstream);

		ScanInfo info;
		info.cbit = _cbit;
		info.ccomp = _ccompScan;
		info.ilv = _ilv;
		info.nnear = _nnear;
		
		Presets presets;

		std::auto_ptr<EncoderStrategy> qcodec(JlsCodecFactory<EncoderStrategy>().GetCodec(info, presets));
		int cbyteWritten = qcodec->EncodeScan((BYTE*)_pvoidRaw, _size, pstream->GetPos(), pstream->GetLength(), NULL); 
		pstream->Seek(cbyteWritten);
	}


void EncodeScanHeader(JLSOutputStream* pstream) const
{
	BYTE ccomponent = BYTE(_ccompScan);
	BYTE itable		= 0;
	USHORT cbyteHeader = 0x06 + 2 * ccomponent;
	pstream->WriteByte(0xFF);
	pstream->WriteByte(JPEG_SOS);
	pstream->WriteWord(cbyteHeader);
	pstream->WriteByte(ccomponent);

	for (int icomponent = 0; icomponent < ccomponent; ++icomponent )
	{
		pstream->WriteByte(BYTE(icomponent + _icompStart));
		pstream->WriteByte(itable);
	}

	pstream->WriteByte(BYTE(_nnear)); // NEAR
	pstream->WriteByte(BYTE(_ilv));
	pstream->WriteByte(0); // transform
}

	const void* _pvoidRaw;
	Size _size;
	int _cbit;
	int _ccompScan;
	interleavemode _ilv;
	int _icompStart;
	int _nnear;
};



void JLSOutputStream::AddScan(const void* pbyteComp, Size size, int cbit, int ccomp, interleavemode ilv, int nearval)
{
	_icompLast += 1;
	_rgsegment.push_back(new JpegImageDataSegment(pbyteComp, size, cbit, _icompLast, ccomp, ilv, nearval));
}
