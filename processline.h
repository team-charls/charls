// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 
#ifndef CHARLS_PROCESSLINE
#define CHARLS_PROCESSLINE

#include "colortransform.h"


class ProcessLine
{
public:
	virtual ~ProcessLine() {}
	virtual void NewLineDecoded(const void* pSrc, int pixelCount, int byteStride) = 0;
	virtual void NewLineRequested(void* pSrc, int pixelCount, int byteStride) = 0;
};



class PostProcesSingleComponent : public ProcessLine
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



template<class TRANSFORM> 
class ProcessTransformed : public ProcessLine
{
	ProcessTransformed(const ProcessTransformed&) {}
public:
	ProcessTransformed(void* pbyteOutput, const JlsParamaters& info) :
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




#endif
