// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <charls/charls.h>

#ifdef _MSC_VER

#include <io.h>

#else
#include <unistd.h>

#define _write write
#define _read read
#define _open open

#endif

#include <fcntl.h>

#include <cstring>
#include <iostream>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif

#ifndef __AFL_INIT
#define __AFL_INIT() // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#endif

#ifndef __AFL_LOOP
#define __AFL_LOOP(a) true // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif


namespace {

auto generate_once()
{
    const std::vector<uint8_t> source(3);
    return charls::jpegls_encoder::encode(source, {1, 1, 8, 3});
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
                auto encoded_data{generate_once()};
                const int result{
                    static_cast<int>(_write(1, encoded_data.data(), static_cast<unsigned int>(encoded_data.size())))};
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
            std::cerr << "Failed to open: " << argv[1] << strerror(errno) << '\n';
            return EXIT_FAILURE;
        }
    }

    __AFL_INIT();

    while (__AFL_LOOP(100))
    {
        std::vector<uint8_t> source(1024 * 1024);
        const size_t input_length = _read(fd, source.data(), static_cast<unsigned int>(source.size()));
        source.resize(input_length);

        try
        {
            std::vector<uint8_t> destination;
            charls::jpegls_decoder::decode(source, destination);
        }
        catch (const charls::jpegls_error&)
        {
        }
    }

    return EXIT_SUCCESS;
}
