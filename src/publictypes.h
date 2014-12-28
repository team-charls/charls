/* 
  (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
*/ 
#ifndef CHARLS_PUBLICTYPES
#define CHARLS_PUBLICTYPES

#include "config.h"

#ifdef __cplusplus
#include <iostream>
#include <cstddef>
#include <cstdint>
#endif

enum JLS_ERROR
{
    OK                              = 0,  // The operation completed without errors.
    InvalidJlsParameters            = 1,  // One of the JLS parameters is invalid.
    ParameterValueNotSupported      = 2,  // The parameter value not supported.
    UncompressedBufferTooSmall      = 3,  // The uncompressed buffer is too small to hold all the output.
    CompressedBufferTooSmall        = 4,  // The compressed buffer too small, more input data was expected.
    InvalidCompressedData           = 5,  // This error is returned when the encoded bit stream contains a general structural problem.
    TooMuchCompressedData           = 6,  // Too much compressed data.The decoding proccess is ready but the input buffer still contains encoded data.
    ImageTypeNotSupported           = 7,  // This error is returned when the bit stream is encoded with an option that is not supported by this implementation.
    UnsupportedBitDepthForTransform = 8,  // The bit depth for transformation is not supported.
    UnsupportedColorTransform       = 9,  // The color transformation is not supported.
    UnsupportedEncoding             = 10, // This error is returned when an encoded frame is found that is not encoded with the JPEG-LS algorithm.
    UnknownJpegMarker               = 11, // This error is returned when an unknown JPEG marker code is detected in the encoded bit stream.
    MissingJpegMarkerStart          = 12, // This error is returned when the algorithm expect a 0xFF code (indicates start of a JPEG marker) but none was found.
    UnspecifiedFailure              = 13, // This error is returned when the implementation detected a failure, but no specific error is available.
    UnexpectedFailure               = 14, // This error is returned when the implementation encountered a failure it didn't expect. No guarantees can be given for the state after this error.
};


enum interleavemode
{
    ILV_NONE = 0,
    ILV_LINE = 1,
    ILV_SAMPLE = 2
};


struct JlsCustomParameters
{
    int MAXVAL;
    int T1;
    int T2;
    int T3;
    int RESET;
};


struct JlsRect
{
    int X, Y;
    int Width, Height;
};


struct JfifParameters
{
    int   Ver;
    char  units;
    int   XDensity;
    int   YDensity;
    short Xthumb;
    short Ythumb;
    void* pdataThumbnail; /* user must set buffer which size is Xthumb*Ythumb*3(RGB) before JpegLsDecode() */
};


struct JlsParameters
{
    int width;
    int height;
    int bitspersample;
    int bytesperline;	/* for [source (at encoding)][decoded (at decoding)] pixel image in user buffer */
    int components;
    int allowedlossyerror;
    enum interleavemode ilv;
    int colorTransform;
    char outputBgr;
    struct JlsCustomParameters custom;
    struct JfifParameters jfif;
};


enum JPEGLS_ColorXForm
{
    // default (RGB)
    COLORXFORM_NONE = 0,

    // Color transforms as defined by HP
    // Not part of the JPEG-LS standard in any way, provided for compatibility with existing streams.
    COLORXFORM_HP1,
    COLORXFORM_HP2,
    COLORXFORM_HP3,

    // Defined by HP but not supported by CharLS
    COLORXFORM_RGB_AS_YUV_LOSSY,
    COLORXFORM_MATRIX,
    XFORM_BIGENDIAN = 1 << 29,
    XFORM_LITTLEENDIAN = 1 << 30
};


#ifdef __cplusplus

// 
// ByteStreamInfo & FromByteArray helper function
//
// ByteStreamInfo describes the stream: either set rawStream to a valid stream, or rawData/count, not both.
// it's possible to decode to memorystreams, but using rawData will always be faster.
//
// Example use: 
//     ByteStreamInfo streamInfo = { fileStream.rdbuf() };
// or 
//     ByteStreamInfo streamInfo = FromByteArray( bytePtr, byteCount);
//
struct ByteStreamInfo
{
    std::basic_streambuf<char>* rawStream;
    uint8_t* rawData;
    std::size_t count;
};


inline ByteStreamInfo FromByteArray(const void* bytes, std::size_t count)
{
    ByteStreamInfo info = ByteStreamInfo();
    info.rawData = (uint8_t*) bytes;
    info.count = count;
    return info;
}

#endif

#endif
