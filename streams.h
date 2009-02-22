// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once

#include <vector>
#include "util.h"

struct ScanInfo
{
	ScanInfo()   :
		cbit(0),
		nnear(0),
		ccomp(0),
		ilv(ILV_NONE),
		size(0,0)
	{
	}
   	int cbit;
	int nnear;
	int ccomp;
	interleavemode ilv;
	Size size;
};

class JpegSegment;


//
// JLSOutputStream: minimal implementation to write JPEG header streams
//
class JLSOutputStream
{
	friend class JpegMarkerSegment;
	friend class JpegImageDataSegment;

public:
	JLSOutputStream();
	virtual ~JLSOutputStream();

	void Init(int ccomp, int ccol, int cline, int cbpp);
	void AddScan(const void* pbyteComp, Size size, int cbit, int ccomp, interleavemode ilv, int nearval);

	int GetBytesWritten()
		{ return _cbyteOffset; }

	int GetLength()
		{ return _cbyteLength - _cbyteOffset; }

	int Write(BYTE* pdata, int cbyteLength, const void* pbyteExpected);

private:
	BYTE* GetPos() const
		{ return _pdata + _cbyteOffset; }

	void WriteByte(BYTE val)
	{ _pdata[_cbyteOffset++] = val; }

	void WriteBytes(const std::vector<BYTE>& rgbyte)
	{
		for (UINT i = 0; i < rgbyte.size(); ++i)
		{
			WriteByte(rgbyte[i]);
		}		
	}

	void WriteWord(USHORT val)
	{
		WriteByte(BYTE(val / 0x100));
		WriteByte(BYTE(val % 0x100));
	}


    void Seek(int cbyte)
		{ _cbyteOffset += cbyte; }

private:
	BYTE* _pdata;
	int _cbyteOffset;
	int _cbyteLength;
	int _icompLast;
	std::vector<JpegSegment*> _rgsegment;
};

struct Presets
{
public:
	Presets() :
			MAXVAL(0), 
			T1(0),
			T2(0),
			T3(0),
			RESET(0)
	{		
	}
	int MAXVAL;
	int T1;
	int T2;
	int T3;
	int RESET;
};


//
// JLSInputStream: minimal implementation to read JPEG header streams
//
class JLSInputStream
{
public:
	JLSInputStream(const BYTE* pdata, int cbyteLength);

	int GetBytesRead()
		{ return _cbyteOffset; }

	const ScanInfo& GetMetadata() const
		{ return _info; } 

	bool Read(void* pvoid);
	int ReadHeader();
	
	void EnableCompare(bool bCompare)
		{ _bCompare = bCompare;	}
private:
	bool ReadPixels(void* pvoid);
	void ReadScan(void*);	
	void ReadStartOfScan();
	void ReadPresetParameters();
	void ReadComment();
	void ReadStartOfFrame();
	int ReadByte();
	int ReadWord();

private:
	const BYTE* _pdata;
	int _cbyteOffset;
	int _cbyteLength;
	bool _bCompare;
	Presets _presets;
	ScanInfo _info;
};




