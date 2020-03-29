// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "legacy.h"

#include "util.h"

#include <array>
#include <cstring>
#include <iostream>
#include <vector>

using std::array;
using std::cout;
using std::vector;
using namespace charls;

namespace {

void TestJpegLsReadHeader(const char* fileName, const int width, const int height, const int bitsPerSample, const int stride, const int component_count, const int interleaveMode)
{
    cout << "LegacyAPI JpegLsReadHeader:" << fileName << "\n";

    vector<uint8_t> encodedBuffer = ReadFile(fileName);

    array<char, ErrorMessageSize> error_message{};
    JlsParameters parameters{};
    const auto error = JpegLsReadHeader(encodedBuffer.data(), encodedBuffer.size(), &parameters, error_message.data());
    Assert::IsTrue(error == jpegls_errc::success);

    Assert::IsTrue(parameters.width == width ||
                   parameters.height == height ||
                   parameters.bitsPerSample == bitsPerSample ||
                   parameters.stride == stride ||
                   parameters.components == component_count ||
                   parameters.interleaveMode == static_cast<interleave_mode>(interleaveMode));
}

void TestJpegLsReadHeader()
{
    cout << "Test JpegLsReadHeader\n";

    TestJpegLsReadHeader("test/conformance/T8C0E0.JLS", 256, 256, 8, 256, 3, 0);
    TestJpegLsReadHeader("test/conformance/T8C1E0.JLS", 256, 256, 8, 768, 3, 1);
    TestJpegLsReadHeader("test/conformance/T8C2E0.JLS", 256, 256, 8, 768, 3, 2);
    TestJpegLsReadHeader("test/conformance/T8C0E3.JLS", 256, 256, 8, 256, 3, 0);
    TestJpegLsReadHeader("test/conformance/T8C1E3.JLS", 256, 256, 8, 768, 3, 1);
    TestJpegLsReadHeader("test/conformance/T8C2E3.JLS", 256, 256, 8, 768, 3, 2);
    TestJpegLsReadHeader("test/conformance/T8NDE0.JLS", 128, 128, 8, 128, 1, 0);
    TestJpegLsReadHeader("test/conformance/T8NDE3.JLS", 128, 128, 8, 128, 1, 0);
    TestJpegLsReadHeader("test/conformance/T16E0.JLS", 256, 256, 12, 512, 1, 0);
    TestJpegLsReadHeader("test/conformance/T16E3.JLS", 256, 256, 12, 512, 1, 0);
    TestJpegLsReadHeader("test/lena8b.JLS", 512, 512, 8, 512, 1, 0);
}

} // namespace

void TestLegacyAPIs()
{
    cout << "Test LegacyAPIs\n";

    TestJpegLsReadHeader();
}
