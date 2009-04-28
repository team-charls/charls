// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 
#ifndef CHARLS_COLORTRANSFORM
#define CHARLS_COLORTRANSFORM

struct TransformNoneImpl
{
	inlinehint static Triplet<BYTE> Apply(int v1, int v2, int v3)
	{ return Triplet<BYTE>(v1, v2, v3); }
};

struct TransformNone : public TransformNoneImpl
{
	typedef struct TransformNoneImpl INVERSE;
};


struct TransformHp1ToRgb
{
	inlinehint static Triplet<BYTE> Apply(int v1, int v2, int v3)
	{ return Triplet<BYTE>(v1 + v2 - 0x80, v2, v3 + v2 - 0x80); }
};

struct TransformHp1
{
	typedef struct TransformHp1ToRgb INVERSE;
	inlinehint static Triplet<BYTE> Apply(int R, int G, int B)
	{
		Triplet<BYTE> hp1;
		hp1.v2 = BYTE(G);
		hp1.v1 = BYTE(R - G + 0x80);
		hp1.v3 = BYTE(B - G + 0x80);
		return hp1;
	}
};


struct TransformHp2
{
	typedef struct TransformHp2ToRgb INVERSE;
	inlinehint static Triplet<BYTE> Apply(int R, int G, int B)
	{
		//Triplet<BYTE> hp2;
		//hp2.v1 = BYTE(R - G + 0x80);
		//hp2.v2 = BYTE(G);
		//hp2.v3 = BYTE(B - ((R+G )>>1) - 0x80);
		//return hp2;
		return Triplet<BYTE>(R - G + 0x80, G, B - ((R+G )>>1) - 0x80);
	}
};

struct TransformHp2ToRgb
{
	inlinehint static Triplet<BYTE> Apply(int v1, int v2, int v3)
	{
		Triplet<BYTE> rgb;
		rgb.R  = BYTE(v1 + v2 - 0x80);          // new R
		rgb.G  = BYTE(v2);                     // new G				
		rgb.B  = BYTE(v3 + ((rgb.R + rgb.G) >> 1) - 0x80); // new B
		return rgb;
	}
};



struct TransformHp3
{
	typedef struct TransformHp3ToRgb INVERSE;
	inlinehint static Triplet<BYTE> Apply(int R, int G, int B)
	{
		Triplet<BYTE> hp3;		
		hp3.v2 = BYTE(B - G + 0x80);
		hp3.v3 = BYTE(R - G + 0x80);
		hp3.v1 = BYTE(G + ((hp3.v2 + hp3.v3)>>2)) - 0x40;
		return hp3;
	}
};


struct TransformHp3ToRgb
{
	inlinehint static Triplet<BYTE> Apply(int v1, int v2, int v3)
	{
		int G = v1 - ((v3 + v2)>>2) + 0x40;
		Triplet<BYTE> rgb;
		rgb.R  = BYTE(v3 + G - 0x80); // new R
		rgb.G  = BYTE(G);             // new G				
		rgb.B  = BYTE(v2 + G - 0x80); // new B
		return rgb;
	}
};


template<class TRANSFORM> 
void TransformLine(Triplet<BYTE>* pDest, const Triplet<BYTE>* pSrc, int pixelCount, const TRANSFORM&)
{	
	for (int i = 0; i < pixelCount; ++i)
	{
		pDest[i] = TRANSFORM::Apply(pSrc[i].v1, pSrc[i].v2, pSrc[i].v3);
	}
};


template<class TRANSFORM> 
void TransformLineToTriplet(const BYTE* ptypeInput, LONG pixelStrideIn, BYTE* pbyteBuffer, LONG pixelStride, const TRANSFORM&)
{
	int cpixel = MIN(pixelStride, pixelStrideIn);
	Triplet<BYTE>* ptypeBuffer = (Triplet<BYTE>*)pbyteBuffer;

	for (int x = 0; x < cpixel; ++x)
	{
		ptypeBuffer[x] = TRANSFORM::Apply(ptypeInput[x], ptypeInput[x + pixelStrideIn], ptypeInput[x + 2*pixelStrideIn]);
	}
}

template<class TRANSFORM> 
void TransformTripletToLine(const BYTE* pbyteInput, LONG pixelStrideIn, BYTE* ptypeBuffer, LONG pixelStride, const TRANSFORM&)
{
	int cpixel = MIN(pixelStride, pixelStrideIn);
	const Triplet<BYTE>* ptypeBufferIn = (Triplet<BYTE>*)pbyteInput;

	for (int x = 0; x < cpixel; ++x)
	{
		Triplet<BYTE> color = ptypeBufferIn[x];
		Triplet<BYTE> colorTranformed = TRANSFORM::Apply(color.v1, color.v2, color.v3);

		ptypeBuffer[x] = colorTranformed.v1;
		ptypeBuffer[x + pixelStride] = colorTranformed.v2;
		ptypeBuffer[x + 2 *pixelStride] = colorTranformed.v3;
	}
}


class PostProcessLine
{
public:
	virtual ~PostProcessLine() {}
	virtual void NewLineDecoded(const void* pSrc, int pixelCount, int byteStride) = 0;
	virtual void NewLineRequested(void* pSrc, int pixelCount, int byteStride) = 0;
};


template<class TRANSFORM> 
class PostProcessTransformed : public PostProcessLine
{
	PostProcessTransformed(const PostProcessTransformed&) {}
public:
	PostProcessTransformed(void* pbyteOutput, const JlsParamaters& info) :
		_pbyteOutput((BYTE*)pbyteOutput),
		_info(info)
	{
		ASSERT(_info.components == 3);
	}

	void NewLineRequested(void* pDst, int pixelCount, int byteStride)
	{
		if (_info.ilv == ILV_SAMPLE)
		{
			TransformLine((Triplet<BYTE>*)pDst, (const Triplet<BYTE>*)_pbyteOutput, pixelCount, TRANSFORM());
		}
		else
		{
			TransformTripletToLine((const BYTE*)_pbyteOutput, pixelCount, (BYTE*)pDst, byteStride, TRANSFORM());
		}
		_pbyteOutput += 3*pixelCount;
	}

	void NewLineDecoded(const void* pSrc, int pixelCount, int byteStride)
	{
		if (_info.ilv == ILV_SAMPLE)
		{
			TransformLine((Triplet<BYTE>*)_pbyteOutput, (const Triplet<BYTE>*)pSrc, pixelCount, TRANSFORM::INVERSE());
		}
		else
		{
			TransformLineToTriplet((const BYTE*)pSrc, byteStride, _pbyteOutput, pixelCount, TRANSFORM::INVERSE());
		}
		_pbyteOutput += 3* pixelCount;
	}

private:
	BYTE* _pbyteOutput;
	const JlsParamaters& _info;	
};


class PostProcesSingleComponent : public PostProcessLine
{
public:
	PostProcesSingleComponent(void* pbyteOutput, const JlsParamaters& info, int bytesPerPixel) :
		_pbyteOutput((BYTE*)pbyteOutput), 
		_bytesPerPixel(bytesPerPixel),
		_info(info)
	{
	}

	void NewLineRequested(void* pDst, int pixelCount, int byteStride)
	{
		::memcpy(pDst, _pbyteOutput, pixelCount * _bytesPerPixel);
		_pbyteOutput += pixelCount * _bytesPerPixel;
	}

	void NewLineDecoded(const void* pSrc, int pixelCount, int byteStride)
	{
		::memcpy(_pbyteOutput, pSrc, pixelCount * _bytesPerPixel);
		_pbyteOutput += pixelCount * _bytesPerPixel;
	}

private:
	BYTE* _pbyteOutput;
	int _bytesPerPixel;
	const JlsParamaters& _info;	
};


#endif
