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
using std::error_code;
using std::swap;
using std::vector;
using namespace charls;

namespace {

    void TestJpegLsReadHeader(const char* fileName, int width, int height, int bitsPerSample, int stride, int components, int interleaveMode) 
    {
        cout << "LegacyAPI JpegLsReadHeader:" << fileName << "\n";

        vector<uint8_t> encodedBuffer = ReadFile(fileName);

        char error_message[ErrorMessageSize];
        JlsParameters parameters;
        auto error = JpegLsReadHeader(encodedBuffer.data(), encodedBuffer.size(), &parameters, error_message);
        if (error != CharlsApiResultType::OK)
            Assert::IsTrue(false);

        if (parameters.width != width ||
            parameters.height != height ||
            parameters.bitsPerSample != bitsPerSample ||
            parameters.stride != stride ||
            parameters.components != components ||
            parameters.interleaveMode != (CharlsInterleaveModeType)interleaveMode)
            Assert::IsTrue(false);
    }

    void TestJpegLsReadHeader() {
          cout << "Test JpegLsReadHeader\n";

        ::TestJpegLsReadHeader("test/conformance/T8C0E0.JLS", 256, 256, 8, 256, 3, 0);
        ::TestJpegLsReadHeader("test/conformance/T8C1E0.JLS", 256, 256, 8, 768, 3, 1);
        ::TestJpegLsReadHeader("test/conformance/T8C2E0.JLS", 256, 256, 8, 768, 3, 2);
        ::TestJpegLsReadHeader("test/conformance/T8C0E3.JLS", 256, 256, 8, 256, 3, 0);
        ::TestJpegLsReadHeader("test/conformance/T8C1E3.JLS", 256, 256, 8, 768, 3, 1);
        ::TestJpegLsReadHeader("test/conformance/T8C2E3.JLS", 256, 256, 8, 768, 3, 2);
        ::TestJpegLsReadHeader("test/conformance/T8NDE0.JLS", 128, 128, 8, 128, 1, 0);
        ::TestJpegLsReadHeader("test/conformance/T8NDE3.JLS", 128, 128, 8, 128, 1, 0);
        ::TestJpegLsReadHeader("test/conformance/T16E0.JLS", 256, 256, 12, 512, 1, 0);
        ::TestJpegLsReadHeader("test/conformance/T16E3.JLS", 256, 256, 12, 512, 1, 0);
        ::TestJpegLsReadHeader("test/lena8b.JLS", 512, 512, 8, 512, 1, 0);
    }

}

void TestLegacyAPIs()
{
    cout << "Test LegacyAPIs\n";

    TestJpegLsReadHeader();
}
