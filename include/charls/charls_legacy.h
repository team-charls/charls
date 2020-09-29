// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_error.h"

#include <iostream>

//
// ByteStreamInfo & FromByteArray helper function
//
// ByteStreamInfo describes the stream: either set rawStream to a valid stream, or rawData/count, not both.
// it's possible to decode to memory streams, but using rawData will always be faster.
struct byte_stream_info final
{
    std::basic_streambuf<char>* rawStream;
    uint8_t* rawData;
    std::size_t count;
};


inline byte_stream_info from_byte_array(void* bytes, const std::size_t count) noexcept
{
    return {nullptr, static_cast<uint8_t*>(bytes), count};
}


inline byte_stream_info from_byte_array_const(const void* bytes, const std::size_t count) noexcept
{
    return from_byte_array(const_cast<void*>(bytes), count);
}

CHARLS_API_IMPORT_EXPORT charls::jpegls_errc JpegLsDecodeStream(byte_stream_info destination, byte_stream_info source, const JlsParameters* params);
CHARLS_API_IMPORT_EXPORT charls::jpegls_errc JpegLsReadHeaderStream(byte_stream_info source, JlsParameters* params);
