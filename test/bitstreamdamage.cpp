// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

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
    vector<uint8_t> rgbyteCompressed;
    if (!ReadFile("test/incorrect_images/InfiniteLoopFFMPEG.jls", &rgbyteCompressed, 0))
        return;

    vector<uint8_t> rgbyteOut(256 * 256 * 2);
    const auto error = JpegLsDecode(rgbyteOut.data(), rgbyteOut.size(), rgbyteCompressed.data(), rgbyteCompressed.size(), nullptr, nullptr);
    Assert::IsTrue(error == jpegls_errc::invalid_encoded_data);
}


void TestDamagedBitStream2()
{
    vector<uint8_t> rgbyteCompressed;
    if (!ReadFile("test/lena8b.jls", &rgbyteCompressed, 0))
        return;

    rgbyteCompressed.resize(900);
    rgbyteCompressed.resize(40000,3);

    vector<uint8_t> rgbyteOut(512 * 512);
    const auto error = JpegLsDecode(rgbyteOut.data(), rgbyteOut.size(), rgbyteCompressed.data(), rgbyteCompressed.size(), nullptr, nullptr);
    Assert::IsTrue(error == jpegls_errc::invalid_encoded_data);
}


void TestDamagedBitStream3()
{
    vector<uint8_t> rgbyteCompressed;
    if (!ReadFile("test/lena8b.jls", &rgbyteCompressed, 0))
        return;

    rgbyteCompressed[300] = 0xFF;
    rgbyteCompressed[301] = 0xFF;

    vector<uint8_t> rgbyteOut(512 * 512);
    const auto error = JpegLsDecode(rgbyteOut.data(), rgbyteOut.size(), rgbyteCompressed.data(), rgbyteCompressed.size(), nullptr, nullptr);
    Assert::IsTrue(error == jpegls_errc::invalid_encoded_data);
}


void TestFileWithRandomHeaderDamage(const char* filename)
{
    vector<uint8_t> rgbyteCompressedOrg;
    if (!ReadFile(filename, &rgbyteCompressedOrg, 0))
        return;

    srand(102347325);

    vector<uint8_t> rgbyteOut(512 * 512);

    for (size_t i = 0; i < 40; ++i)
    {
        vector<uint8_t> rgbyteCompressedTest(rgbyteCompressedOrg);
        vector<int> errors(10, 0);

        for (int j = 0; j < 20; ++j)
        {
            rgbyteCompressedTest[i] = static_cast<uint8_t>(rand());
            rgbyteCompressedTest[i+1] = static_cast<uint8_t>(rand());
            rgbyteCompressedTest[i+2] = static_cast<uint8_t>(rand());
            rgbyteCompressedTest[i+3] = static_cast<uint8_t>(rand());

            const auto error = JpegLsDecode(rgbyteOut.data(), rgbyteOut.size(), &rgbyteCompressedTest[0], rgbyteCompressedTest.size(), nullptr, nullptr);
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


void DamagedBitstreamTests()
{
    cout << "Test Damaged bitstream\r\n";
    TestDamagedBitStream1();
    TestDamagedBitStream2();
    TestDamagedBitStream3();

    cout << "Begin random malformed bitstream tests:\n";
    TestRandomMalformedHeader();
    cout << "End randommalformed bitstream tests:\n";
}
