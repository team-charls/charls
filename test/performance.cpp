// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "performance.h"
#include "util.h"

#include <vector>
#include <ratio>
#include <chrono>
#include <iostream>

using std::vector;
using std::cout;
using std::error_code;
using std::istream;
using std::chrono::steady_clock;
using std::chrono::duration;
using std::milli;

namespace
{

void TestFile16BitAs12(const char* filename, int offset, Size size2, int componentCount, bool littleEndianFile)
{
    vector<uint8_t> uncompressedData = ReadFile(filename, offset);

    FixEndian(&uncompressedData, littleEndianFile);

    const auto p = reinterpret_cast<uint16_t*>(uncompressedData.data());

    for (size_t i = 0; i < uncompressedData.size() / 2; ++i)
    {
        p[i] = p[i] >> 4;
    }

    TestRoundTrip(filename, uncompressedData, size2, 12, componentCount);
}


void TestPerformance(int loopCount)
{
    ////TestFile("test/bad.raw", 0, Size(512, 512),  8, 1);

    // RGBA image (This is a common PNG sample)
    TestFile("test/alphatest.raw", 0, Size(380, 287), 8, 4, false, loopCount);

    const Size size1024 = Size(1024, 1024);
    const Size size512 = Size(512, 512);

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


} // namespace


void PerformanceTests(int loopCount)
{
#ifdef _DEBUG
    cout << "NOTE: running performance test in debug mode, performance may be slow!\n";
#endif
    cout << "Test Perf (with loop count "<< loopCount << ")\n";
    TestPerformance(loopCount);
}

void TestLargeImagePerformanceRgb8(int loopCount)
{
    // Note: the test images are very large and not included in the repository.
    //       The images can be downloaded from: http://imagecompression.info/test_images/

#ifdef _DEBUG
    cout << "NOTE: running performance test in debug mode, performance may be slow!\n";
#endif
    cout << "Test Large Images Performance\n";

    try
    {
        test_portable_anymap_file("test/rgb8bit/artificial.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/big_building.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/big_tree.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/bridge.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/cathedral.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/deer.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/fireworks.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/flower_foveon.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/hdr.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/leaves_iso_200.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/leaves_iso_1600.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/nightshot_iso_100.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/nightshot_iso_1600.ppm", loopCount);
        test_portable_anymap_file("test/rgb8bit/spider_web.ppm", loopCount);
    }
    catch (const istream::failure& error)
    {
        cout << "Test failed " << error.what() << "\n";
    }
}

void DecodePerformanceTests(int loopCount)
{
    cout << "Test decode Perf (with loop count " << loopCount << ")\n";

    vector<uint8_t> jpeglsCompressed = ReadFile("decodetest.jls");

    JlsParameters params{};
    error_code error = JpegLsReadHeader(jpeglsCompressed.data(), jpeglsCompressed.size(), &params, nullptr);
    if (error)
        return;

    vector<uint8_t> uncompressed(static_cast<size_t>(params.height) * params.width * ((params.bitsPerSample + 7) / 8) * params.components);

    const auto start = steady_clock::now();
    for (int i = 0; i < loopCount; ++i)
    {

        error = JpegLsDecode(uncompressed.data(), uncompressed.size(), jpeglsCompressed.data(), jpeglsCompressed.size(), &params, nullptr);
        if (error)
        {
            cout << "Decode failure: " << error.value() << "\n";
            return;
        }
    }

    const auto end = steady_clock::now();
    const auto diff = end - start;
    cout << "Total decoding time is: " << duration<double, milli>(diff).count() << " ms\n";
    cout << "Decoding time per image: " << duration<double, milli>(diff).count() / loopCount << " ms\n";
}
