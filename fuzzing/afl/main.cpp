// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <charls/charls.hpp>

#include "../src/conditional_static_cast.hpp"

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>

#define _read read
#define _open open
#define _close close

#endif

#include <fcntl.h>

#include <cstring>
#include <iostream>
#include <vector>

using namespace charls;
using std::vector;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif

#ifndef __AFL_INIT
// ReSharper disable once CppInconsistentNaming
#define __AFL_INIT() // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#endif

#ifndef __AFL_LOOP
// ReSharper disable once CppInconsistentNaming
#define __AFL_LOOP(a) true // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,cppcoreguidelines-macro-usage)
#define AFL_LOOP_FOREVER
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif


namespace {

vector<uint8_t> generate_once()
{
    const vector<uint8_t> source(3);

    jpegls_encoder encoder;
    encoder.frame_info({1, 1, 8, 3});

    vector<uint8_t> destination(encoder.estimated_destination_size());
    encoder.destination(destination);

    const size_t bytes_written{encoder.encode(source)};
    destination.resize(bytes_written);

    return destination;
}

} // namespace


int main(const int argc, const char* const argv[]) // NOLINT(bugprone-exception-escape)
{
    int fd{};
    if (argc == 2)
    {
        if (argv[1][0] == '\0')
        {
            try
            {
                // Write some small-ish JPEG-LS file to stdout
                const auto encoded_data{generate_once()};
#ifdef _MSC_VER
                const int result{_write(1, encoded_data.data(), conditional_static_cast<unsigned int>(encoded_data.size()))};
#else
                const ssize_t result{
                    write(1, encoded_data.data(), conditional_static_cast<unsigned int>(encoded_data.size()))};
#endif
                return result != -1 && result == static_cast<int>(encoded_data.size()) ? EXIT_SUCCESS : EXIT_FAILURE;
            }
            catch (const std::exception& error)
            {
                std::cerr << "Failed to create the once: " << error.what() << '\n';
            }
            return EXIT_FAILURE;
        }

        fd = _open(argv[1], O_RDONLY); // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
        if (fd < 0)
        {
            std::cerr << "Failed to open: " << argv[1] << strerror(errno) << '\n';  // NOLINT(concurrency-mt-unsafe)
            return EXIT_FAILURE;
        }
    }

    __AFL_INIT();

    while (__AFL_LOOP(100))
    {
        vector<uint8_t> source(size_t{1024} * 1024);
        const auto input_length{_read(fd, source.data(), charls::conditional_static_cast<unsigned int>(source.size()))};
        if (input_length < 0)
        {
            _close(fd);
            return EXIT_FAILURE;
        }

        source.resize(static_cast<size_t>(input_length));

        try
        {
            vector<uint8_t> destination;
            jpegls_decoder::decode(source, destination);
        }
        catch (const jpegls_error&)  // NOLINT(bugprone-empty-catch)
        {
        }
    }

#ifndef AFL_LOOP_FOREVER
    return EXIT_SUCCESS;
#endif
}
