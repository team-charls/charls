#ifndef JLS_INTERFACE
#define JLS_INTERFACE

enum JLS_ERROR
{
	OK = 0,
	InvalidJlsParameters,
	ParameterValueNotSupported,
	UncompressedBufferTooSmall,
	CompressedBufferTooSmall,
	InvalidCompressedData
};

enum interleavemode
{
	ILV_NONE = 0,
	ILV_LINE = 1,
	ILV_SAMPLE = 2,
};

struct JlsParamaters
{
	int width;
	int height;
	int bitspersample;
	int components;
	int allowedlossyerror;
	interleavemode ilv;
};

#ifndef CHARLS_IMEXPORT
#define CHARLS_IMEXPORT __declspec(dllimport) 
#endif
extern "C"
{
  CHARLS_IMEXPORT	JLS_ERROR JpegLsEncode(void* pdataCompressed, int cbyteBuffer, int* pcbyteWritten, const void* pdataUncompressed, int cbyteUncompressed, const JlsParamaters* pparams);
  CHARLS_IMEXPORT	JLS_ERROR JpegLsDecode(void* pdataUncompressed, int cbyteUncompressed, const void* pdataCompressed, int cbyteCompressed);
  CHARLS_IMEXPORT	JLS_ERROR JpegLsReadHeader(const void* pdataUncompressed, int cbyteUncompressed, JlsParamaters* pparams);
  CHARLS_IMEXPORT	JLS_ERROR JpegLsCanEncode(JlsParamaters*);
  CHARLS_IMEXPORT   JLS_ERROR JpegLsCanDecode(JlsParamaters*);
}
#endif