// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "../../include/charls/jpegls_encoder.hpp"

#include <FuzzedDataProvider.h>
#include <filesystem>
#include <fstream>
#include <vector>

using std::byte;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, const size_t size)
{
    FuzzedDataProvider fdp(data, size);

    const auto width{fdp.ConsumeIntegralInRange<uint32_t>(1, 512)};
    const auto height{fdp.ConsumeIntegralInRange<uint32_t>(1, 512)};
    const auto bits_per_sample{fdp.ConsumeIntegralInRange<int32_t>(2, 16)};
    const auto component_count{fdp.ConsumeIntegralInRange<int32_t>(1, 4)};
    const auto near_lossless{fdp.ConsumeIntegralInRange<int32_t>(0, 9)};
    const auto interleave_mode{fdp.ConsumeIntegralInRange<int32_t>(0, 2)};
    
    const auto pixels{fdp.ConsumeRemainingBytes<byte>()};

    try
    {
        charls::jpegls_encoder encoder;

        encoder.frame_info({width, height, bits_per_sample, component_count});
        std::vector<byte> destination(encoder.estimated_destination_size());
        encoder.destination(destination);
        encoder.near_lossless(near_lossless);
        encoder.interleave_mode(static_cast<charls::interleave_mode>(interleave_mode));

        encoder.encode(pixels);
    }
    catch (const charls::jpegls_error&) // NOLINT(bugprone-empty-catch)
    {
    }

    return 0;
}

// Rename the function to main to retrieve code coverage from the collected corpus files.
void main_coverage()
{
    for (auto& entry : std::filesystem::directory_iterator("corpus"))
    {
        std::ifstream f(entry.path(), std::ios::binary);
        std::vector<uint8_t> data(std::istreambuf_iterator(f), {});
        LLVMFuzzerTestOneInput(data.data(), data.size());
    }
}
