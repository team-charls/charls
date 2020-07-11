// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "performance.h"
#include "util.h"

#include <chrono>
#include <iostream>
#include <ratio>
#include <vector>

using std::cout;
using std::error_code;
using std::istream;
using std::milli;
using std::vector;
using std::chrono::duration;
using std::chrono::steady_clock;

namespace {

void TestFile16BitAs12(const char* filename, const int offset, const rect_size size2, const int component_count, const bool little_endian_file)
{
    vector<uint8_t> uncompressedData = ReadFile(filename, offset);

    FixEndian(&uncompressedData, little_endian_file);

    auto* const p = reinterpret_cast<uint16_t*>(uncompressedData.data());

    for (size_t i = 0; i < uncompressedData.size() / 2; ++i)
    {
        p[i] = p[i] >> 4;
    }

    TestRoundTrip(filename, uncompressedData, size2, 12, component_count);
}


void TestPerformance(const int loop_count)
{
    ////TestFile("test/bad.raw", 0, rect_size(512, 512),  8, 1);

    // RGBA image (This is a common PNG sample)
    TestFile("test/alphatest.raw", 0, rect_size(380, 287), 8, 4, false, loop_count);

    const rect_size size1024 = rect_size(1024, 1024);
    const rect_size size512 = rect_size(512, 512);

    // 16 bit mono
    TestFile("test/MR2_UNC", 1728, size1024, 16, 1, true, loop_count);

    // 8 bit mono
    TestFile("test/0015.raw", 0, size1024, 8, 1, false, loop_count);
    TestFile("test/lena8b.raw", 0, size512, 8, 1, false, loop_count);

    // 8 bit color
    TestFile("test/desktop.ppm", 40, rect_size(1280, 1024), 8, 3, false, loop_count);

    // 12 bit RGB
    TestFile("test/SIEMENS-MR-RGB-16Bits.dcm", -1, rect_size(192, 256), 12, 3, true, loop_count);
    TestFile16BitAs12("test/DSC_5455.raw", 142949, rect_size(300, 200), 3, true);

    // 16 bit RGB
    TestFile("test/DSC_5455.raw", 142949, rect_size(300, 200), 16, 3, true, loop_count);
}


} // namespace


void PerformanceTests(const int loop_count)
{
#ifdef _DEBUG
    cout << "NOTE: running performance test in debug mode, performance may be slow!\n";
#endif
    cout << "Test Perf (with loop count " << loop_count << ")\n";
    TestPerformance(loop_count);
}

void TestLargeImagePerformanceRgb8(const int loop_count)
{
    // Note: the test images are very large and not included in the repository.
    //       The images can be downloaded from: http://imagecompression.info/test_images/

#ifdef _DEBUG
    cout << "NOTE: running performance test in debug mode, performance may be slow!\n";
#endif
    cout << "Test Large Images Performance\n";

    try
    {
        test_portable_anymap_file("test/rgb8bit/artificial.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/big_building.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/big_tree.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/bridge.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/cathedral.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/deer.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/fireworks.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/flower_foveon.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/hdr.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/leaves_iso_200.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/leaves_iso_1600.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/nightshot_iso_100.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/nightshot_iso_1600.ppm", loop_count);
        test_portable_anymap_file("test/rgb8bit/spider_web.ppm", loop_count);
    }
    catch (const istream::failure& error)
    {
        cout << "Test failed " << error.what() << "\n";
    }
}

void DecodePerformanceTests(const int loop_count)
{
    cout << "Test decode Perf (with loop count " << loop_count << ")\n";

    vector<uint8_t> jpeglsCompressed = ReadFile("decodetest.jls");

    JlsParameters params{};
    error_code error = JpegLsReadHeader(jpeglsCompressed.data(), jpeglsCompressed.size(), &params, nullptr);
    if (error)
        return;

    vector<uint8_t> uncompressed(static_cast<size_t>(params.height) * params.width * ((params.bitsPerSample + 7) / 8) * params.components);

    const auto start = steady_clock::now();
    for (int i = 0; i < loop_count; ++i)
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
    cout << "Decoding time per image: " << duration<double, milli>(diff).count() / loop_count << " ms\n";
}
