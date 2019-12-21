// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "bmp_image.h"

#include <charls/charls.h>

#include <cassert>
#include <iostream>
#include <vector>

namespace {

std::vector<uint8_t> encode_bmp_image_to_jpegls(const bmp_image& image, int near_lossless)
{
    assert(image.dib_header.depth == 24);        // This function only supports 24-bit BMP pixel data.
    assert(image.dib_header.compress_type == 0); // Data needs to be stored by pixel as RGB.

    charls::jpegls_encoder encoder;
    encoder.frame_info({image.dib_header.width, image.dib_header.height, 8, 3})
        .near_lossless(near_lossless);

    std::vector<uint8_t> buffer(encoder.estimated_destination_size());
    encoder.destination(buffer);

    encoder.write_standard_spiff_header(charls::spiff_color_space::rgb,
                                        charls::spiff_resolution_units::dots_per_centimeter,
                                        image.dib_header.vertical_resolution / 100,
                                        image.dib_header.horizontal_resolution / 100);

    const size_t encoded_size = encoder.encode(image.pixel_data);
    buffer.resize(encoded_size);

    return buffer;
}

void save_buffer_to_file(const void* buffer, size_t buffer_size, const char* filename)
{
    assert(filename);
    assert(buffer);
    assert(buffer_size);

    std::ofstream output(filename, std::ios::binary);
    output.write(static_cast<const char*>(buffer), buffer_size);
}

void log_failure(const char* message) noexcept
{
    try
    {
        std::cerr << message  << "\n";
    }
    catch (...)
    {
        assert(false);
    }
}

} // namespace


int main(const int argc, char const* const argv[])
{
    try
    {
        std::ios::sync_with_stdio(false);

        if (argc < 3)
        {
            log_failure("Usage: <input_file_name> <output_file_name> [near-lossless-value, default=0 (lossless)]");
            return EXIT_FAILURE;
        }

        int near_lossless{};
        if (argc > 3)
        {
            near_lossless = static_cast<int>(strtol(argv[3], nullptr, 10));
            if (near_lossless < 0 || near_lossless > 255)
            {
                log_failure("near-lossless-value needs to be in the range [0,255]");
                return EXIT_FAILURE;
            }
        }

        auto encoded_buffer = encode_bmp_image_to_jpegls(bmp_image{argv[1]}, near_lossless);
        save_buffer_to_file(encoded_buffer.data(), encoded_buffer.size(), argv[2]);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        log_failure(error.what());
    }
    catch (...)
    {
        log_failure("Unknown error occurred");
        assert(false);
    }

    return EXIT_FAILURE;
}
