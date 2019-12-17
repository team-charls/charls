// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "util.h"

#include "portable_anymap_file.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

using std::cout;
using std::error_code;
using std::ifstream;
using std::milli;
using std::setprecision;
using std::setw;
using std::swap;
using std::vector;
using std::chrono::duration;
using std::chrono::steady_clock;
using namespace charls;
using namespace charls_test;


namespace {

MSVC_WARNING_SUPPRESS(26497) // cannot be marked constexpr, check must be executed at runtime.

bool IsMachineLittleEndian() noexcept
{
    constexpr int a = 0xFF000001;  // NOLINT(bugprone-narrowing-conversions)
    const auto* chars = reinterpret_cast<const char*>(&a);
    return chars[0] == 0x01;
}

MSVC_WARNING_UNSUPPRESS()


} // namespace


void FixEndian(vector<uint8_t>* buffer, bool littleEndianData) noexcept
{
    if (littleEndianData == IsMachineLittleEndian())
        return;

    for (size_t i = 0; i < buffer->size() - 1; i += 2)
    {
        swap((*buffer)[i], (*buffer)[i + 1]);
    }
}


vector<uint8_t> ReadFile(const char* filename, long offset, size_t bytes)
{
    ifstream input;
    input.exceptions(input.eofbit | input.failbit | input.badbit);
    input.open(filename, input.in | input.binary);

    input.seekg(0, input.end);
    const auto byteCountFile = static_cast<int>(input.tellg());
    input.seekg(offset, input.beg);

    if (offset < 0)
    {
        Assert::IsTrue(bytes != 0);
        offset = static_cast<long>(byteCountFile - bytes);
    }
    if (bytes == 0)
    {
        bytes = static_cast<size_t>(byteCountFile) - offset;
    }

    vector<uint8_t> buffer(bytes);
    input.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    return buffer;
}


void TestRoundTrip(const char* strName, const vector<uint8_t>& decodedBuffer, Size size, int bitsPerSample, int componentCount, int loopCount)
{
    JlsParameters params = JlsParameters();
    params.components = componentCount;
    params.bitsPerSample = bitsPerSample;
    params.height = static_cast<int>(size.cy);
    params.width = static_cast<int>(size.cx);

    TestRoundTrip(strName, decodedBuffer, params, loopCount);
}


void TestRoundTrip(const char* strName, const vector<uint8_t>& originalBuffer, JlsParameters& params, int loopCount)
{
    vector<uint8_t> encodedBuffer(params.height * params.width * params.components * params.bitsPerSample / 4);

    vector<uint8_t> decodedBuffer(static_cast<size_t>(params.height) * params.width * ((params.bitsPerSample + 7) / 8) * params.components);

    if (params.components == 4)
    {
        params.interleaveMode = interleave_mode::line;
    }
    else if (params.components == 3)
    {
        params.interleaveMode = interleave_mode::line;
        params.colorTransformation = color_transformation::hp1;
    }

    size_t encoded_actual_size{};
    auto start = steady_clock::now();
    for (int i = 0; i < loopCount; ++i)
    {
        const error_code error = JpegLsEncode(encodedBuffer.data(), encodedBuffer.size(), &encoded_actual_size,
                                              originalBuffer.data(), originalBuffer.size(), &params, nullptr);
        Assert::IsTrue(!error);
    }

    const auto totalEncodeDuration = steady_clock::now() - start;

    start = steady_clock::now();
    for (int i = 0; i < loopCount; ++i)
    {
        const error_code error = JpegLsDecode(decodedBuffer.data(), decodedBuffer.size(), encodedBuffer.data(), encoded_actual_size, nullptr, nullptr);
        Assert::IsTrue(!error);
    }

    const auto totalDecodeDuration = steady_clock::now() - start;

    const double bitsPerSample = 1.0 * encoded_actual_size * 8 / (static_cast<double>(params.components) * params.height * params.width);
    cout << "RoundTrip test for: " << strName << "\n\r";
    const double encodeTime = duration<double, milli>(totalEncodeDuration).count() / loopCount;
    const double decodeTime = duration<double, milli>(totalDecodeDuration).count() / loopCount;
    const double symbolRate = (static_cast<double>(params.components) * params.height * params.width) / (1000.0 * decodeTime);

    cout << "Size:" << setw(10) << params.width << "x" << params.height << setw(7) << setprecision(2) << ", Encode time:" << encodeTime << " ms, Decode time:" << decodeTime << " ms, Bits per sample:" << bitsPerSample << ", Decode rate:" << symbolRate << " M/s\n";

    const uint8_t* byteOut = decodedBuffer.data();
    for (size_t i = 0; i < decodedBuffer.size(); ++i)
    {
        if (originalBuffer[i] != byteOut[i])
        {
            Assert::IsTrue(false);
            break;
        }
    }
}


void TestFile(const char* filename, int offset, Size size2, int bitsPerSample, int componentCount, bool littleEndianFile, int loopCount)
{
    const size_t byteCount = size2.cx * size2.cy * componentCount * ((bitsPerSample + 7) / 8);
    vector<uint8_t> uncompressedBuffer = ReadFile(filename, offset, byteCount);

    if (bitsPerSample > 8)
    {
        FixEndian(&uncompressedBuffer, littleEndianFile);
    }

    TestRoundTrip(filename, uncompressedBuffer, size2, bitsPerSample, componentCount, loopCount);
}


void test_portable_anymap_file(const char* filename, int loopCount)
{
    portable_anymap_file anymapFile(filename);

    TestRoundTrip(filename, anymapFile.image_data(), Size(anymapFile.width(), anymapFile.height()),
                  anymapFile.bits_per_sample(), anymapFile.component_count(), loopCount);
}
