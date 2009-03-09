// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once

#include <vector>
#include "util.h"


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

	void Init(Size size, int cbpp, int ccomp);
	void AddScan(const void* pbyteComp, const JlsParamaters* pparams);
	void AddLSE(const JlsCustomParameters* pcustom);
	int GetBytesWritten()
		{ return _cbyteOffset; }

	int GetLength()
		{ return _cbyteLength - _cbyteOffset; }

	int Write(BYTE* pdata, int cbyteLength);
	
	void EnableCompare(bool bCompare) 
	{ _bCompare = bCompare; };
private:
	BYTE* GetPos() const
		{ return _pdata + _cbyteOffset; }

	void WriteByte(BYTE val)
	{ 
		ASSERT(!_bCompare || _pdata[_cbyteOffset] == val);
		
		_pdata[_cbyteOffset++] = val; 
	}

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

	bool _bCompare;

private:
	BYTE* _pdata;
	int _cbyteOffset;
	int _cbyteLength;
	int _icompLast;
	std::vector<JpegSegment*> _segments;
};



struct Presets : public JlsCustomParameters
{
public:
	Presets()			
	{		
		MAXVAL = 0;
		T1 = 0;
		T2 = 0;
		T3 = 0;
		RESET = 0;
	};
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

	const JlsParamaters& GetMetadata() const
		{ return _info; } 

	const JlsCustomParameters& GetCustomPreset() const
	{ return _info.custom; } 

	void Read(void* pvoid, int cbyteAvailable);
	void ReadHeader();
	
	void EnableCompare(bool bCompare)
		{ _bCompare = bCompare;	}
private:
	void ReadPixels(void* pvoid, int cbyteAvailable);
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
	JlsParamaters _info;
};




