// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 





#ifndef CHARLS_ENCODERSTRATEGY
#define CHARLS_ENCODERSTRATEGY

#include "decoderstrategy.h"

class EncoderStrategy
{

public:
	EncoderStrategy() :
		 _qdecoder(0),
		 valcurrent(0),
		 bitpos(0),
		 _bFFWritten(false),
		 _cbyteWritten(0)
	{};

		 enum { IsDecoding = 0 };

	virtual ~EncoderStrategy() 
		 {}

	LONG PeekByte();
	

	template <class T>
	void OnLineBegin(T* ptypeCur, T* ptypeLine, LONG cpixel)
	{
		memcpy(ptypeCur, ptypeLine, cpixel * sizeof(T));
	}

	void OnLineEnd(void* /*ptypeCur*/, void* /*ptypeLine*/, LONG /*cpixel*/) {};

    virtual void SetPresets(const JlsCustomParameters& presets) = 0;
		
	virtual ULONG EncodeScan(const void* pvoid, const Size& size, LONG ccomp, void* pvoidOut, ULONG cbyte, void* pvoidCompare) = 0;

protected:

	void Init(BYTE* pbyteCompressed, ULONG cbyte)
	{
		bitpos = 32;
		valcurrent = 0;
		_pbyteCompressed = pbyteCompressed;
   		_cbyteCompressed = cbyte;
	}


	void AppendToBitStream(ULONG value, ULONG length)
	{	
		ASSERT(length < 32 && length >= 0);

		ASSERT((_qdecoder == NULL) || (length == 0 && value == 0) ||( _qdecoder->ReadLongValue(length) == value));

#ifdef _DEBUG
		if (length < 32)
		{
			ULONG mask = (1 << (length)) - 1;
			ASSERT((value | mask) == mask);
		}
#endif

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

	inline bool hasbit(ULONG i, LONG ibit)
	{
		return (i & (1 << ibit)) != 0;
	}

	void Flush()
	{
		for (LONG i = 0; i < 4; ++i)
		{
			if (bitpos >= 32)
				break;

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

	ULONG GetLength() 
	{ 
		return _cbyteWritten - (bitpos -32)/8; 
	};


	inlinehint void AppendOnesToBitStream(ULONG length)
	{
		AppendToBitStream((1 << length) - 1, length);	
	}


	DecoderStrategy* _qdecoder; 

private:

	ULONG valcurrent;
	LONG bitpos;
	ULONG _cbyteCompressed;
	
	// encoding
	BYTE* _pbyteCompressed;
	bool _bFFWritten;
	ULONG _cbyteWritten;


};

#endif
