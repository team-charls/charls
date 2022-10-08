// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <charls/charls.h>

#include "../src/conditional_static_cast.h"

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>

#define _read read
#define _open open

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
#define __AFL_LOOP(a) true // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
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

        fd = _open(argv[1], O_RDONLY);
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
        const size_t input_length = _read(fd, source.data(), charls::conditional_static_cast<unsigned int>(source.size()));
        source.resize(input_length);

        try
        {
            vector<uint8_t> destination;
            jpegls_decoder::decode(source, destination);
        }
        catch (const jpegls_error&)
        {
        }
    }

#ifndef AFL_LOOP_FOREVER
    return EXIT_SUCCESS;
#endif
}
