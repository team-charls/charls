// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "public_types.h"

#include <iostream>

//
// ByteStreamInfo & FromByteArray helper function
//
// ByteStreamInfo describes the stream: either set rawStream to a valid stream, or rawData/count, not both.
// it's possible to decode to memory streams, but using rawData will always be faster.
struct ByteStreamInfo final
{
    std::basic_streambuf<char>* rawStream;
    uint8_t* rawData;
    std::size_t count;
};


inline ByteStreamInfo FromByteArray(void* bytes, std::size_t count) noexcept
{
    return {nullptr, static_cast<uint8_t*>(bytes), count};
}


inline ByteStreamInfo FromByteArrayConst(const void* bytes, std::size_t count) noexcept
{
    return FromByteArray(const_cast<void*>(bytes), count);
}

CHARLS_API_IMPORT_EXPORT charls::jpegls_errc JpegLsEncodeStream(ByteStreamInfo destination, size_t& bytesWritten, ByteStreamInfo source, const JlsParameters& params);
CHARLS_API_IMPORT_EXPORT charls::jpegls_errc JpegLsDecodeStream(ByteStreamInfo destination, ByteStreamInfo source, const JlsParameters* params);
CHARLS_API_IMPORT_EXPORT charls::jpegls_errc JpegLsReadHeaderStream(ByteStreamInfo source, JlsParameters* params);
