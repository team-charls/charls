// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "pch.h"

#include "bmp_image.h"

#include <charls/jpegls_encoder.h>
#include <cassert>
#include <charconv>

using std::cout;
using std::exception;
using std::ofstream;
using std::ios;
using std::vector;
using std::byte;
using charls::jpegls_encoder;
using charls::metadata;

namespace {

vector<byte> encode_bmp_image_to_jpegls(const bmp_image& image, int allowed_lossy_error)
{
    assert(image.dib_header.depth == 24);        // This function only supports 24-bit BMP pixel data.
    assert(image.dib_header.compress_type == 0); // Data needs to be stored by pixel as RGB.

    jpegls_encoder encoder;
    encoder.source(image.pixel_data.data(), image.pixel_data.size(),
                   metadata{static_cast<int32_t>(image.dib_header.width), static_cast<int32_t>(image.dib_header.height), 8, 3});
    encoder.allowed_lossy_error(allowed_lossy_error);

    return encoder.encode();
}


void save_buffer_to_file(const void* buffer, size_t buffer_size, const char* filename)
{
    assert(filename);
    assert(buffer);
    assert(buffer_size);

    ofstream output(filename, ios::binary);
    output.write(static_cast<const char*>(buffer), buffer_size);
}

int from_chars(const char* argument) noexcept
{
    int value;
    std::from_chars(argument, argument + strlen(argument), value);
    return value;
}

} // nameless namespace.


int main(const int argc, char const * const argv[])
{
    try
    {
        if (argc < 3)
        {
            cout << "Usage: input_file_name output_file_name [allowed_lossy_error, default=0 (lossless)]\n";
            return EXIT_FAILURE;
        }

        int allowed_lossy_error = 0;
        if (argc > 3)
        {
            allowed_lossy_error = from_chars(argv[3]);
            if (allowed_lossy_error < 0 || allowed_lossy_error > 255)
            {
                cout << "allowed_lossy_error needs to be in the range [0,255]\n";
                return EXIT_FAILURE;
            }
        }

        auto encoded_buffer = encode_bmp_image_to_jpegls(bmp_image{argv[1]}, allowed_lossy_error);
        save_buffer_to_file(encoded_buffer.data(), encoded_buffer.size(), argv[2]);

        return EXIT_SUCCESS;
    }
    catch (const exception& error)
    {
        cout << error.what();
    }
    catch (...)
    {
        cout << "Unknown error occurred";
    }

    return EXIT_FAILURE;
}
