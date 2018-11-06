// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#pragma once

#include "publictypes.h"
#include "api_abi.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stddef.h>
#endif

/// <summary>
/// Encodes a byte array with pixel data to a JPEG-LS encoded (compressed) byte array.
/// </summary>
/// <param name="destination">Byte array that holds the encoded bytes when the function returns.</param>
/// <param name="destinationLength">Length of the array in bytes. If the array is too small the function will return an error.</param>
/// <param name="bytesWritten">This parameter will hold the number of bytes written to the destination byte array. Cannot be NULL.</param>
/// <param name="source">Byte array that holds the pixels that should be encoded.</param>
/// <param name="sourceLength">Length of the array in bytes.</param>
/// <param name="params">Parameter object that describes the pixel data and how to encode it.</param>
/// <param name="errorMessage">Character array of at least 256 characters or NULL. Hold the error message when a failure occurs, empty otherwise.</param>
CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION JpegLsEncode(
    void* destination,
    size_t destinationLength,
    size_t* bytesWritten,
    const void* source,
    size_t sourceLength,
    const struct JlsParameters* params,
    char* errorMessage);

/// <summary>
/// Retrieves the JPEG-LS header. This info can be used to pre-allocate the uncompressed output buffer.
/// </summary>
/// <param name="compressedData">Byte array that holds the JPEG-LS encoded data of which the header should be extracted.</param>
/// <param name="compressedLength">Length of the array in bytes.</param>
/// <param name="params">Parameter object that describes how the pixel data is encoded.</param>
/// <param name="errorMessage">Character array of at least 256 characters or NULL. Hold the error message when a failure occurs, empty otherwise.</param>
CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION JpegLsReadHeader(
    const void* compressedData,
    size_t compressedLength,
    struct JlsParameters* params,
    char* errorMessage);

/// <summary>
/// Encodes a JPEG-LS encoded byte array to uncompressed pixel data byte array.
/// </summary>
/// <param name="destination">Byte array that holds the uncompressed pixel data bytes when the function returns.</param>
/// <param name="destinationLength">Length of the array in bytes. If the array is too small the function will return an error.</param>
/// <param name="source">Byte array that holds the JPEG-LS encoded data that should be decoded.</param>
/// <param name="sourceLength">Length of the array in bytes.</param>
/// <param name="params">Parameter object that describes the pixel data and how to decode it.</param>
/// <param name="errorMessage">Character array of at least 256 characters or NULL. Hold the error message when a failure occurs, empty otherwise.</param>
CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION JpegLsDecode(
    void* destination,
    size_t destinationLength,
    const void* source,
    size_t sourceLength,
    const struct JlsParameters* params,
    char* errorMessage);

CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION JpegLsDecodeRect(
    void* uncompressedData,
    size_t uncompressedLength,
    const void* compressedData,
    size_t compressedLength,
    struct JlsRect roi,
    const struct JlsParameters* info,
    char* errorMessage);

#ifdef __cplusplus
}

CHARLS_API_IMPORT_EXPORT CharlsApiResultType JpegLsEncodeStream(ByteStreamInfo compressedStreamInfo, size_t& bytesWritten, ByteStreamInfo rawStreamInfo, const JlsParameters& params, char* errorMessage);
CHARLS_API_IMPORT_EXPORT CharlsApiResultType JpegLsDecodeStream(ByteStreamInfo rawStream, ByteStreamInfo compressedStream, const JlsParameters* info, char* errorMessage);
CHARLS_API_IMPORT_EXPORT CharlsApiResultType JpegLsReadHeaderStream(ByteStreamInfo rawStreamInfo, JlsParameters* params, char* errorMessage);

#endif
