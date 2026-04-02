// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "performance.hpp"

#include <support/portable_anymap_file.hpp>

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

void decode_performance_tests(const char* filename, const int loop_count)
{
    cout << "Test decode performance with loop count " << loop_count << "\n";

    try
    {
        const auto encoded_source{read_file(filename)};

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
        cout << "IO failure (missing input file): " << e.what() << "\n";
    }
}

void encode_performance_tests(const char* filename, const int loop_count)
{
    cout << "Test encode performance with loop count " << loop_count << "and"<< filename << "\n";

    try
    {
        const charls::support::portable_anymap_file anymap_file(filename);

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
        cout << "IO failure (missing input file): " << e.what() << "\n";
    }
}
