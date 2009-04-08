// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


struct TransformRgbToHp1
{
	static Triplet Apply(int R, int G, int B)
	{
		Triplet hp1;
		hp1.v2 = BYTE(G);
		hp1.v1 = BYTE(R - G + 0x80);
		hp1.v3 = BYTE(B - G + 0x80);
		return hp1;
	}
};

struct TransformRgbToHp2
{
	static Triplet Apply(int R, int G, int B)
	{
		Triplet hp1;
		hp1.v2 = BYTE(G);
		hp1.v1 = BYTE(R - G + 0x80);
		hp1.v3 = BYTE(B - ((R+G )>>1) - 0x80);
		return hp1;
	}
};


struct TransformRgbToHp3
{
	static Triplet Apply(int R, int G, int B)
	{
		Triplet hp1;		
		hp1.v2 = BYTE(B - G + 0x80);
		hp1.v3 = BYTE(R - G + 0x80);

		hp1.v1 = BYTE(G + ((hp1.v2 + hp1.v3 )>>2) - 0x40);
		return hp1;
	}
};

template<class TRANSFORM> 
void TransformLine(Triplet* pDest, Triplet* pSrc, int pixelCount, const TRANSFORM&)
{	
	for (int i = 0; i < pixelCount; ++i)
	{
		pDest[i] = TRANSFORM::Apply(pSrc[i]);
	}
}


template<class TRANSFORM> 
void TransformLine(BYTE* ptypeInput, LONG cpixel, BYTE* ptypeBuffer, LONG pixelStride, const TRANSFORM&)
{
	for (int x = 0; x < cpixel; ++x)
	{
		Triplet colorTranformed = TRANSFORM::Apply(ptypeInput[x], ptypeInput[x + cpixel], ptypeInput[x + 2*cpixel]);
		ptypeBuffer[x] = colorTranformed.v1;
		ptypeBuffer[x + pixelStride] = colorTranformed.v2;
		ptypeBuffer[x + 2 *pixelStride] = colorTranformed.v3;
	}
}

#ifndef CHARLS_ENCODERSTRATEGY
#define CHARLS_ENCODERSTRATEGY

#include "decoderstrategy.h"

class EncoderStrategy
{

public:
	explicit EncoderStrategy(const JlsParamaters& info) :
		 _qdecoder(0),
		 valcurrent(0),
		 bitpos(0),
		 _bFFWritten(false),
		 _cbyteWritten(0),
		 _info(info)
	{};

	virtual ~EncoderStrategy() 
		 {}

	LONG PeekByte();
	
	

	void OnLineBegin(Triplet* ptypeInput, int iline, LONG cpixel, Triplet* ptypeBuffer, LONG /*pixelStride*/)
	{
		memcpy(ptypeBuffer, ptypeInput + cpixel*iline, cpixel * sizeof(Triplet));
	}

	void OnLineBegin(USHORT* ptypeInput, int iline, LONG cpixel, USHORT* ptypeBuffer, LONG /*pixelStride*/)
	{
		memcpy(ptypeBuffer, ptypeInput + cpixel*iline, cpixel * sizeof(USHORT));
	}
	
	void OnLineBegin(BYTE* ptypeInput, int iline, LONG cpixel, BYTE* ptypeBuffer, LONG pixelStride)
	{
		if (_info.colorTransform == 0)
		{
			for (int i = 0; i < _info.components; ++i)
			{
				memcpy(ptypeBuffer + pixelStride * i, ptypeInput + cpixel * (iline * _info.components + i), 
					cpixel * sizeof(BYTE));
			}
			return;
		}

		ptypeInput += iline * _info.components * cpixel;
		
		switch(_info.colorTransform)
		{
			case COLORXFORM_HP1 : return TransformLine(ptypeInput, cpixel, ptypeBuffer, pixelStride, TransformRgbToHp1());
			case COLORXFORM_HP2 : return TransformLine(ptypeInput, cpixel, ptypeBuffer, pixelStride, TransformRgbToHp2());
			case COLORXFORM_HP3 : return TransformLine(ptypeInput, cpixel, ptypeBuffer, pixelStride, TransformRgbToHp3());
		}
	}

	void OnLineEnd(void* /*ptypeCur*/, void* /*ptypeLine*/, LONG /*cpixel*/) {};

    virtual void SetPresets(const JlsCustomParameters& presets) = 0;
		
	virtual size_t EncodeScan(const void* pvoid, const Size& size, LONG ccomp, void* pvoidOut, size_t cbyte, void* pvoidCompare) = 0;

protected:

	void Init(BYTE* pbyteCompressed, size_t cbyte)
	{
		bitpos = 32;
		valcurrent = 0;
		_pbyteCompressed = pbyteCompressed;
   		_cbyteCompressed = cbyte;
	}


	void AppendToBitStream(LONG value, LONG length)
	{	
		ASSERT(length < 32 && length >= 0);

		ASSERT((_qdecoder == NULL) || (length == 0 && value == 0) ||( _qdecoder->ReadLongValue(length) == value));

#ifdef _DEBUG
		if (length < 32)
		{
			int mask = (1 << (length)) - 1;
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

	size_t GetLength() 
	{ 
		return _cbyteWritten - (bitpos -32)/8; 
	};


	inlinehint void AppendOnesToBitStream(LONG length)
	{
		AppendToBitStream((1 << length) - 1, length);	
	}


	DecoderStrategy* _qdecoder; 

protected:
	JlsParamaters _info;

private:

	unsigned int valcurrent;
	LONG bitpos;
	size_t _cbyteCompressed;
	
	// encoding
	BYTE* _pbyteCompressed;
	bool _bFFWritten;
	size_t _cbyteWritten;

};

#endif
