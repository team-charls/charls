// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once

#include "decoderstrategy.h"

class EncoderStrategy
{

public:
	EncoderStrategy() :
		 valcurrent(0),
		 bitpos(0),
		 _qdecoder(0),
		 _bFFWritten(false),
		 _cbyteWritten(0)
	{};


	virtual ~EncoderStrategy() 
		 {}

    virtual void SetPresets(const Presets& presets) = 0;
		
	virtual int EncodeScan(const void* pvoid, const Size& size, void* pvoidOut, int cbyte, void* pvoidCompare) = 0;

protected:

void Init(BYTE* pbyteCompressed, int cbyte)
{
	bitpos = 32;
	valcurrent = 0;
	_pbyteCompressed = pbyteCompressed;
   	_cbyteCompressed = cbyte;
}


inlinehint void AppendToBitStream(UINT value, UINT length)
{	
	ASSERT(length < 32 && length >= 0);

	ASSERT((_qdecoder == NULL) || (length == 0 && value == 0) ||( _qdecoder->ReadLongValue(length) == value));

	if (length < 32)
	{
		UINT mask = (1 << (length)) - 1;
		ASSERT((value | mask) == mask);
	}

	bitpos -= length;
	if (bitpos >= 0)
	{
		valcurrent = valcurrent | (value << bitpos);
		return;
	}
	valcurrent |= value >> -bitpos;

	Flush();
        
	ASSERT(bitpos >=0);
	valcurrent |= value << bitpos;	

}

inline bool hasbit(UINT i, int ibit)
{
	return (i & (1 << ibit)) != 0;
}

void Flush()
{
	for (int i = 0; i < 4; ++i)
	{
		if (_bFFWritten)
		{
			// insert highmost bit
			*_pbyteCompressed = BYTE(valcurrent >> 25);
			valcurrent = valcurrent << 7;			
			bitpos += 7;	
			_bFFWritten = false;
		}
		else
		{
			*_pbyteCompressed = BYTE(valcurrent >> 24);
			valcurrent = valcurrent << 8;			
			bitpos += 8;			
			_bFFWritten = *_pbyteCompressed == 0xFF;			
		}
		
		_pbyteCompressed++;
		_cbyteCompressed--;
		_cbyteWritten++;

	}
	
}

int GetLength() 
{ 
	return _cbyteWritten - (bitpos -32)/8; 
};


inlinehint void AppendOnesToBitStream(UINT length)
{
	AppendToBitStream((1 << length) - 1, length);	
}

	DecoderStrategy* _qdecoder; 

private:

	UINT valcurrent;
	// encoding
	int _cbyteCompressed;
	
	// encoding
	int bitpos;
	BYTE* _pbyteCompressed;
	bool _bFFWritten;
	int _cbyteWritten;
};
