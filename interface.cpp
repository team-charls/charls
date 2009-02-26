
#include "stdafx.h"
#include "interface.h"
#include "header.h"

JLS_ERROR CheckInput(const void* pdataCompressed, int cbyteCompressed, const void* pdataUncompressed, int cbyteUncompressed, const JlsParamaters* pparams)
{
	if (pparams == NULL)
		return InvalidJlsParameters;

	if (cbyteCompressed == 0)
		return InvalidJlsParameters;

	if (pdataCompressed == NULL)
		return InvalidJlsParameters;

	if (pdataUncompressed == NULL)
		return InvalidJlsParameters;

	if (pparams->bitspersample < 8 || pparams->bitspersample > 16)
		return ParameterValueNotSupported;

	if (pparams->width < 1 || pparams->width > 65535)
		return ParameterValueNotSupported;

	if (pparams->height < 1 || pparams->height > 65535)
		return ParameterValueNotSupported;

	if (cbyteUncompressed < (pparams->width * pparams->height * pparams->components * ((pparams->bitspersample + 7)/8)))
		return InvalidJlsParameters;

	switch (pparams->components)
	{
		case 3: return (pparams->bitspersample != 8) ? ParameterValueNotSupported : OK;
		case 1: return OK;
		case 0: return InvalidJlsParameters;
		default: return ParameterValueNotSupported;
	}
}


JLS_ERROR JpegLsEncode(void* pdataCompressed, int cbyteBuffer, int* pcbyteWritten, const void* pdataUncompressed, int cbyteUncompressed, const JlsParamaters* pparams)
{

	JLS_ERROR parameterError = CheckInput(pdataCompressed, cbyteBuffer, pdataUncompressed, cbyteUncompressed, pparams);

	if (pcbyteWritten == NULL)
		return parameterError;

	if (parameterError != OK)
		return parameterError;

	Size size = Size(pparams->width, pparams->height);
	int cbit = pparams->bitspersample;

	JLSOutputStream stream;
	
	stream.Init(size, pparams->bitspersample, pparams->components);
	
	if (pparams->ilv == ILV_NONE)
	{
		int cbyteComp = size.cx*size.cy*((cbit +7)/8);
		for (int icomp = 0; icomp < pparams->components; ++icomp)
		{
			const BYTE* pbyteComp = static_cast<const BYTE*>(pdataUncompressed) + icomp*cbyteComp;
			stream.AddScan(pbyteComp, size, cbit, 1, ILV_NONE, pparams->allowedlossyerror);
		}
	}
	else 
	{
		stream.AddScan(pdataUncompressed, size, cbit, pparams->components, pparams->ilv, pparams->allowedlossyerror);
	}

	stream.Write((BYTE*)pdataCompressed, cbyteBuffer, NULL);
	
	*pcbyteWritten = stream.GetBytesWritten();	
	return OK;
}

//JLS_ERROR JpegLsDecode(void* pdataUncompressed, int cbyteUncompressed, const void* pdataCompressed, int cbyteCompressed)
//{
//	return ParameterValueNotSupported;
//}


JLS_ERROR JpegLsReadHeader(const void* pdataCompressed, int cbyteCompressed, JlsParamaters* pparams)
{
	JLSInputStream reader((BYTE*)pdataCompressed, cbyteCompressed);
	reader.ReadHeader();	
	ScanInfo info = reader.GetMetadata();
	pparams->width = info.size.cx;
	pparams->height = info.size.cy;
	pparams->components= info.ccomp;
	pparams->bitspersample = info.cbit;
	pparams->allowedlossyerror = info.nnear;
	pparams->ilv = info.ilv;

	return OK;
}