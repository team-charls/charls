// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <vector>
#include "../include/charls/charls_jpegls_decoder.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, const size_t size)
{
    std::vector<uint8_t> destination(100000);
    charls::jpegls_decoder decoder(data, size, false);

    try
    {
        decoder.read_header();
        decoder.decode(destination);
    }
    catch (const charls::jpegls_error&) // NOLINT(bugprone-empty-catch)
    {
    }

    return 0;
}
