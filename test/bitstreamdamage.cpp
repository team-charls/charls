// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#include "config.h"
#include "util.h"

#include "../src/charls.h"

#include <iostream>
#include <vector>


void TestDamagedBitStream1()
{
    std::vector<BYTE> rgbyteCompressed; 
    if (!ReadFile("test/incorrect_images/InfiniteLoopFFMPEG.jls", &rgbyteCompressed, 0))
        return;

    std::vector<BYTE> rgbyteOut(256 * 256 * 2); 
    auto error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);
    Assert::IsTrue(error == charls::ApiResult::InvalidCompressedData);
}


void TestDamagedBitStream2()
{
    std::vector<BYTE> rgbyteCompressed;
    if (!ReadFile("test/lena8b.jls", &rgbyteCompressed, 0))
        return;

    rgbyteCompressed.resize(900);
    rgbyteCompressed.resize(40000,3);

    std::vector<BYTE> rgbyteOut(512 * 512); 
    auto error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);
    Assert::IsTrue(error == charls::ApiResult::InvalidCompressedData);
}


void TestDamagedBitStream3()
{
    std::vector<BYTE> rgbyteCompressed;
    if (!ReadFile("test/lena8b.jls", &rgbyteCompressed, 0))
        return;

    rgbyteCompressed[300] = 0xFF;
    rgbyteCompressed[301] = 0xFF;

    std::vector<BYTE> rgbyteOut(512 * 512);
    auto error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressed[0], rgbyteCompressed.size(), nullptr, nullptr);
    Assert::IsTrue(error == charls::ApiResult::InvalidCompressedData);
}


void TestFileWithRandomHeaderDamage(SZC filename)
{
    std::vector<BYTE> rgbyteCompressedOrg;
    if (!ReadFile(filename, &rgbyteCompressedOrg, 0))
        return;

    srand(102347325);

    std::vector<BYTE> rgbyteOut(512 * 512);

    for (int i = 0; i < 40; ++i)
    {
        std::vector<BYTE> rgbyteCompressedTest(rgbyteCompressedOrg);
        std::vector<int> errors(10, 0);

        for (int j = 0; j < 20; ++j)
        {
            rgbyteCompressedTest[i] = static_cast<BYTE>(rand());
            rgbyteCompressedTest[i+1] = static_cast<BYTE>(rand());
            rgbyteCompressedTest[i+2] = static_cast<BYTE>(rand());
            rgbyteCompressedTest[i+3] = static_cast<BYTE>(rand());

            auto error = JpegLsDecode(&rgbyteOut[0], rgbyteOut.size(), &rgbyteCompressedTest[0], rgbyteCompressedTest.size(), nullptr, nullptr);
            errors[static_cast<int>(error)]++;
        }

        std::cout << "With garbage input at index " << i << ": ";
        for(unsigned int error = 0; error < errors.size(); ++error)
        {
            if (errors[error] == 0)
                continue;

            std::cout <<  errors[error] << "x error (" << error << "); ";
        }

        std::cout << "\r\n";
    }
}


void TestRandomMalformedHeader()
{
    TestFileWithRandomHeaderDamage("test/conformance/T8C0E0.JLS");
    TestFileWithRandomHeaderDamage("test/conformance/T8C1E0.JLS");
    TestFileWithRandomHeaderDamage("test/conformance/T8C2E0.JLS");
}


void DamagedBitstreamTests()
{
    printf("Test Damaged bitstream\r\n");
    TestDamagedBitStream1();
    TestDamagedBitStream2();
    TestDamagedBitStream3();

    printf("Begin random malformed bitstream tests: \r\n");
    TestRandomMalformedHeader();
    printf("End randommalformed bitstream tests: \r\n");
}
