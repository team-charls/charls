// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "performance.hpp"

#include "portable_anymap_file.hpp"
#include "util.hpp"

#include <chrono>
#include <iostream>
#include <ratio>
#include <tuple>
#include <vector>

using charls::frame_info;
using charls::jpegls_decoder;
using charls::jpegls_encoder;
using charls::jpegls_error;
using std::byte;
using std::cout;
using std::istream;
using std::milli;
using std::vector;
using std::chrono::duration;
using std::chrono::steady_clock;

namespace {

void test_performance(const int loop_count)
{
    // RGBA image (This is a common PNG sample)
    test_file("test/alphatest.raw", 0, {380, 287}, 8, 4, false, loop_count);

    // 16-bit mono
    test_file("test/MR2_UNC", 1728, {1024, 1024}, 16, 1, true, loop_count);

    // 8-bit mono
    test_file("test/0015.raw", 0, {1024, 1024}, 8, 1, false, loop_count);

    // 8-bit color
    test_file("test/desktop.ppm", 40, {1280, 1024}, 8, 3, false, loop_count);
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

    try
    {
        // This test expect the file decodetest.jls to exist.
        // It can be any valid JPEG-LS file.
        // Changing the content of this file allows different performance measurements.
        const auto encoded_source{read_file("decodetest.jls")};

        // Pre-allocate the destination outside the measurement loop.
        // std::vector initializes its elements and this step needs to be excluded from the measurement.
        vector<byte> destination(jpegls_decoder{encoded_source, true}.get_destination_size());

        const auto start{steady_clock::now()};
        for (int i{}; i != loop_count; ++i)
        {
            jpegls_decoder decoder{encoded_source, true};

            decoder.decode(destination);
        }

        const auto end{steady_clock::now()};
        const auto diff{end - start};
        cout << "Total decoding time is: " << duration<double, milli>(diff).count() << " ms\n";
        cout << "Decoding time per image: " << duration<double, milli>(diff).count() / loop_count << " ms\n";
    }
    catch (const jpegls_error& e)
    {
        cout << "Decode failure: " << e.what() << "\n";
    }
    catch (const std::ios_base::failure& e)
    {
        cout << "IO failure (missing decodetest.jls?): " << e.what() << "\n";
    }
}

void encode_performance_tests(const int loop_count)
{
    cout << "Test encode performance with loop count " << loop_count << "\n";

    try
    {
        const charls_test::portable_anymap_file anymap_file("encode-test.pnm");

        const frame_info info{static_cast<uint32_t>(anymap_file.width()), static_cast<uint32_t>(anymap_file.height()),
                              anymap_file.bits_per_sample(), anymap_file.component_count()};
        const auto interleave_mode{anymap_file.component_count() > 1 ? charls::interleave_mode::sample
                                                                     : charls::interleave_mode::none};

        jpegls_encoder encoder1;
        encoder1.frame_info(info).interleave_mode(interleave_mode);
        vector<byte> destination(encoder1.estimated_destination_size());

        const auto start{steady_clock::now()};
        for (int i{}; i != loop_count; ++i)
        {
            jpegls_encoder encoder2;
            encoder2.frame_info(info).interleave_mode(interleave_mode);
            encoder2.destination(destination);

            std::ignore = encoder2.encode(anymap_file.image_data());
        }

        const auto end{steady_clock::now()};
        const auto diff{end - start};
        cout << "Total encoding time is: " << duration<double, milli>(diff).count() << " ms\n";
        cout << "Encoding time per image: " << duration<double, milli>(diff).count() / loop_count << " ms\n";
    }
    catch (const jpegls_error& e)
    {
        cout << "Encoding failure: " << e.what() << "\n";
    }
    catch (const std::ios_base::failure& e)
    {
        cout << "IO failure (missing encode-test.pnm?): " << e.what() << "\n";
    }
}
