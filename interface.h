#include "stdafx.h"

#ifndef JLS_INTERFACE
#define JLS_INTERFACE


enum JLS_ERROR
{
	OK = 0,
	InvalidJlsParameters,
	ParameterValueNotSupported,
	UncompressedBufferTooSmall,
	CompressedBufferTooSmall,
	InvalidCompressedData,
	ImageTypeNotSupported
};

class JlsException
{
public:
	JlsException(JLS_ERROR error) : _error(error)
		{ }

	JLS_ERROR _error;
};

enum interleavemode
{
	ILV_NONE = 0,
	ILV_LINE = 1,
	ILV_SAMPLE = 2,
};

struct JlsCustomParameters
{
	int MAXVAL;
	int T1;
	int T2;
	int T3;
	int RESET;
};

struct JlsParamaters
{
	int width;
	int height;
	int bitspersample;
	int components;
	int allowedlossyerror;
	interleavemode ilv;
	JlsCustomParameters custom;
};


#if defined(WIN32)
#ifndef CHARLS_IMEXPORT
#define CHARLS_IMEXPORT __declspec(dllimport) 
#pragma comment (lib,"charls.lib")
#endif
#else
#ifndef CHARLS_IMEXPORT
#define CHARLS_IMEXPORT
#endif
#endif /* WIN32 */


extern "C"
{
  CHARLS_IMEXPORT JLS_ERROR JpegLsEncode(void* pdataCompressed, int cbyteBuffer, size_t* pcbyteWritten, const void* pdataUncompressed, int cbyteUncompressed, const JlsParamaters* pparams);
  CHARLS_IMEXPORT JLS_ERROR JpegLsDecode(void* pdataUncompressed, int cbyteUncompressed, const void* pdataCompressed, int cbyteCompressed);
  CHARLS_IMEXPORT JLS_ERROR JpegLsReadHeader(const void* pdataUncompressed, int cbyteUncompressed, JlsParamaters* pparams);
  CHARLS_IMEXPORT JLS_ERROR JpegLsVerifyEncode(const void* pdataUncompressed, int cbyteUncompressed, const void* pdataCompressed, int cbyteCompressed);
//CHARLS_IMEXPORT	JLS_ERROR JpegLsCanEncode(JlsParamaters*);
  //CHARLS_IMEXPORT   JLS_ERROR JpegLsCanDecode(JlsParamaters*);
}


#endif
