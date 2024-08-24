// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "../include/charls/jpegls_decoder.hpp"

#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, const size_t size)
{
    charls::jpegls_decoder decoder(data, size, false);

    try
    {
        decoder.read_header();

        if (const auto& frame_info{decoder.frame_info()};
            frame_info.height < 500 && frame_info.width < 500 && frame_info.component_count < 4)
        {
            std::vector<uint8_t> destination(decoder.get_destination_size());
            decoder.decode(destination);
        }
    }
    catch (const charls::jpegls_error&) // NOLINT(bugprone-empty-catch)
    {
    }

    return 0;
}
