// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "performance.h"

#include "portable_anymap_file.h"
#include "util.h"

#include <chrono>
#include <iostream>
#include <ratio>
#include <vector>

using charls::frame_info;
using charls::jpegls_decoder;
using charls::jpegls_encoder;
using charls::jpegls_error;
using std::cout;
using std::istream;
using std::milli;
using std::vector;
using std::chrono::duration;
using std::chrono::steady_clock;

namespace {

void test_file16_bit_as12(const char* filename, const int offset, const rect_size size2, const int component_count,
                          const bool little_endian_file)
{
    vector<uint8_t> uncompressed_data = read_file(filename, offset);

    fix_endian(&uncompressed_data, little_endian_file);

    auto* const p = reinterpret_cast<uint16_t*>(uncompressed_data.data());

    for (size_t i = 0; i < uncompressed_data.size() / 2; ++i)
    {
        p[i] = p[i] >> 4;
    }

    test_round_trip(filename, uncompressed_data, size2, 12, component_count);
}


void test_performance(const int loop_count)
{
    ////TestFile("test/bad.raw", 0, rect_size(512, 512),  8, 1);

    // RGBA image (This is a common PNG sample)
    test_file("test/alphatest.raw", 0, rect_size(380, 287), 8, 4, false, loop_count);

    const rect_size size1024 = rect_size(1024, 1024);
    const rect_size size512 = rect_size(512, 512);

    // 16 bit mono
    test_file("test/MR2_UNC", 1728, size1024, 16, 1, true, loop_count);

    // 8 bit mono
    test_file("test/0015.raw", 0, size1024, 8, 1, false, loop_count);
    test_file("test/lena8b.raw", 0, size512, 8, 1, false, loop_count);

    // 8 bit color
    test_file("test/desktop.ppm", 40, rect_size(1280, 1024), 8, 3, false, loop_count);

    // 12 bit RGB
    test_file("test/SIEMENS-MR-RGB-16Bits.dcm", -1, rect_size(192, 256), 12, 3, true, loop_count);
    test_file16_bit_as12("test/DSC_5455.raw", 142949, rect_size(300, 200), 3, true);

    // 16 bit RGB
    test_file("test/DSC_5455.raw", 142949, rect_size(300, 200), 16, 3, true, loop_count);
}


} // namespace


void performance_tests(const int loop_count)
{
#ifdef _DEBUG
    cout << "NOTE: running performance test in debug mode, performance may be slow!\n";
#endif
    cout << "Test Perf (with loop count " << loop_count << ")\n";
    test_performance(loop_count);
}

void test_large_image_performance_rgb8(const int loop_count)
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

void decode_performance_tests(const int loop_count)
{
    cout << "Test decode performance with loop count " << loop_count << "\n";

    const vector<uint8_t> jpegls_compressed = read_file("decodetest.jls");

    try
    {
        vector<uint8_t> uncompressed;

        const auto start = steady_clock::now();
        for (int i = 0; i < loop_count; ++i)
        {
            jpegls_decoder::decode(jpegls_compressed, uncompressed);
        }

        const auto end = steady_clock::now();
        const auto diff = end - start;
        cout << "Total decoding time is: " << duration<double, milli>(diff).count() << " ms\n";
        cout << "Decoding time per image: " << duration<double, milli>(diff).count() / loop_count << " ms\n";
    }
    catch (const jpegls_error& e)
    {
        cout << "Decode failure: " << e.what() << "\n";
    }
}

void encode_performance_tests(const int loop_count)
{
    cout << "Test encode performance with loop count " << loop_count << "\n";

    const charls_test::portable_anymap_file anymap_file("encode-test.pnm");

    try
    {
        const frame_info info{static_cast<uint32_t>(anymap_file.width()), static_cast<uint32_t>(anymap_file.height()),
                              anymap_file.bits_per_sample(), anymap_file.component_count()};
        const auto interleave_mode =
            anymap_file.component_count() > 1 ? charls::interleave_mode::sample : charls::interleave_mode::none;

        jpegls_encoder encoder1;
        encoder1.frame_info(info).interleave_mode(interleave_mode);
        vector<uint8_t> destination(encoder1.estimated_destination_size());

        const auto start = steady_clock::now();
        for (int i = 0; i < loop_count; ++i)
        {
            jpegls_encoder encoder2;
            encoder2.frame_info(info).interleave_mode(interleave_mode);
            encoder2.destination(destination);

            static_cast<void>(encoder2.encode(anymap_file.image_data()));
        }

        const auto end = steady_clock::now();
        const auto diff = end - start;
        cout << "Total encoding time is: " << duration<double, milli>(diff).count() << " ms\n";
        cout << "Encoding time per image: " << duration<double, milli>(diff).count() / loop_count << " ms\n";
    }
    catch (const jpegls_error& e)
    {
        cout << "Encoding failure: " << e.what() << "\n";
    }
}
