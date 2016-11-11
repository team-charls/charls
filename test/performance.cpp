// 
// (C) Jan de Vaan 2007-2010, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 

#include "performance.h"
#include "config.h"
#include "util.h"
#include "../src/charls.h"

#include <vector>

namespace
{

void TestFile16BitAs12(SZC strName, int ioffs, Size size2, int ccomp, bool littleEndianFile)
{
    std::vector<uint8_t> rgbyteUncompressed;
    if (!ReadFile(strName, &rgbyteUncompressed, ioffs))
        return;

    FixEndian(&rgbyteUncompressed, littleEndianFile);

    USHORT* pushort = reinterpret_cast<USHORT*>(rgbyteUncompressed.data());

    for (size_t i = 0; i < rgbyteUncompressed.size() / 2; ++i)
    {
        pushort[i] = pushort[i] >> 4;
    }

    TestRoundTrip(strName, rgbyteUncompressed, size2, 12, ccomp);
}


void TestPerformance(int loopCount)
{
    ////TestFile("test/bad.raw", 0, Size(512, 512),  8, 1);

    // RGBA image (This is a common PNG sample)
    TestFile("test/alphatest.raw", 0, Size(380, 287), 8, 4, false, loopCount);

    Size size1024 = Size(1024, 1024);
    Size size512 = Size(512, 512);

    // 16 bit mono
    TestFile("test/MR2_UNC", 1728, size1024, 16, 1, true, loopCount);

    // 8 bit mono
    TestFile("test/0015.raw", 0, size1024, 8, 1, false, loopCount);
    TestFile("test/lena8b.raw", 0, size512, 8, 1, false, loopCount);

    // 8 bit color
    TestFile("test/desktop.ppm", 40, Size(1280, 1024), 8, 3, false, loopCount);

    // 12 bit RGB
    TestFile("test/SIEMENS-MR-RGB-16Bits.dcm", -1, Size(192, 256), 12, 3, true, loopCount);
    TestFile16BitAs12("test/DSC_5455.raw", 142949, Size(300, 200), 3, true);

    // 16 bit RGB
    TestFile("test/DSC_5455.raw", 142949, Size(300, 200), 16, 3, true, loopCount);
}


void TestLargeImagePerformance(int loopCount)
{
    TestFile("test/rgb8bit/artificial.ppm", 17, Size(3072, 2048), 8, 3, false, loopCount);
    TestFile("test/rgb8bit/bridge.ppm", 17, Size(2749, 4049), 8, 3, false, loopCount);
    TestFile("test/rgb8bit/flower_foveon.ppm", 17, Size(2268, 1512), 8, 3, false, loopCount);
    ////TestFile("test/rgb8bit/big_building.ppm", 17, Size(7216,5412),  8, 3);
    ////TestFile("test/rgb16bit/bridge.ppm", 19, Size(2749,4049),  16, 3, true);
}

} // namespace


void PerformanceTests(int loopCount)
{
    printf("Test Perf (with loop count %i)\r\n", loopCount);
    TestPerformance(loopCount);

#ifndef _DEBUG
    printf("Test Large Images Performance\r\n");
    TestLargeImagePerformance(loopCount);
#endif
}

void DecodePerformanceTests(int loopCount)
{
    printf("Test decode Perf (with loop count %i)\r\n", loopCount);

    std::vector<uint8_t> jpeglsCompressed;
    if (!ReadFile("decodetest.jls", &jpeglsCompressed, 0, 0))
        return;

    JlsParameters params;
    auto result = JpegLsReadHeader(jpeglsCompressed.data(), jpeglsCompressed.size(), &params, nullptr);
    if (result != charls::ApiResult::OK)
        return;

    std::vector<uint8_t> uncompressed(params.height * params.width * 2);

    for (int i = 0; i < loopCount; ++i)
    {

        result = JpegLsDecode(uncompressed.data(), uncompressed.size(), jpeglsCompressed.data(), jpeglsCompressed.size(), &params, nullptr);
        if (result != charls::ApiResult::OK)
            return;
    }
}
