// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "bitstreamdamage.h"
#include "util.h"

#include <iostream>
#include <vector>

using std::cout;
using std::vector;
using charls::jpegls_errc;

namespace
{

void TestDamagedBitStream1()
{
    vector<uint8_t> encodedBuffer = ReadFile("test/incorrect_images/InfiniteLoopFFMPEG.jls");

    vector<uint8_t> destination(256 * 256 * 2);
    const auto error = JpegLsDecode(destination.data(), destination.size(), encodedBuffer.data(), encodedBuffer.size(), nullptr, nullptr);
    Assert::IsTrue(error == jpegls_errc::invalid_encoded_data);
}


void TestDamagedBitStream2()
{
    vector<uint8_t> encodedBuffer = ReadFile("test/lena8b.jls");
    
    encodedBuffer.resize(900);
    encodedBuffer.resize(40000, 3);

    vector<uint8_t> destination(512 * 512);
    const auto error = JpegLsDecode(destination.data(), destination.size(), encodedBuffer.data(), encodedBuffer.size(), nullptr, nullptr);
    Assert::IsTrue(error == jpegls_errc::invalid_encoded_data);
}


void TestDamagedBitStream3()
{
    vector<uint8_t> encodedBuffer = ReadFile("test/lena8b.jls");

    encodedBuffer[300] = 0xFF;
    encodedBuffer[301] = 0xFF;

    vector<uint8_t> destination(512 * 512);
    const auto error = JpegLsDecode(destination.data(), destination.size(), encodedBuffer.data(), encodedBuffer.size(), nullptr, nullptr);
    Assert::IsTrue(error == jpegls_errc::invalid_encoded_data);
}


void TestFileWithRandomHeaderDamage(const char* filename)
{
    const vector<uint8_t> encodedBufferOriginal = ReadFile(filename);

    srand(102347325);

    vector<uint8_t> destination(512 * 512);

    for (size_t i = 0; i < 40; ++i)
    {
        vector<uint8_t> encodedBuffer(encodedBufferOriginal);
        vector<int> errors(10, 0);

        for (int j = 0; j < 20; ++j)
        {
            encodedBuffer[i] = static_cast<uint8_t>(rand());
            encodedBuffer[i+1] = static_cast<uint8_t>(rand());
            encodedBuffer[i+2] = static_cast<uint8_t>(rand());
            encodedBuffer[i+3] = static_cast<uint8_t>(rand());

            const auto error = JpegLsDecode(destination.data(), destination.size(), &encodedBuffer[0], encodedBuffer.size(), nullptr, nullptr);
            errors[static_cast<int>(error)]++;
        }

        cout << "With garbage input at index " << i << ": ";
        for(unsigned int error = 0; error < errors.size(); ++error)
        {
            if (errors[error] == 0)
                continue;

            cout <<  errors[error] << "x error (" << error << "); ";
        }

        cout << "\r\n";
    }
}


void TestRandomMalformedHeader()
{
    TestFileWithRandomHeaderDamage("test/conformance/T8C0E0.JLS");
    TestFileWithRandomHeaderDamage("test/conformance/T8C1E0.JLS");
    TestFileWithRandomHeaderDamage("test/conformance/T8C2E0.JLS");
}


} // namespace


void DamagedBitStreamTests()
{
    cout << "Test Damaged bit stream\r\n";
    TestDamagedBitStream1();
    TestDamagedBitStream2();
    TestDamagedBitStream3();

    cout << "Begin random malformed bit stream tests:\n";
    TestRandomMalformedHeader();
    cout << "End random malformed bit stream tests:\n";
}
