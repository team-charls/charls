// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "pch.h"

#include "bmp_image.h"

#include <charls/charls.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

void convert_bgr_to_rgb(std::vector<uint8_t>& triplet_buffer) noexcept
{
    for (size_t i = 0; i < triplet_buffer.size(); i += 3)
    {
        std::swap(triplet_buffer[i], triplet_buffer[i + 2]);
    }
}

std::vector<uint8_t> triplet_to_planar(const std::vector<uint8_t>& buffer, const uint32_t width, const uint32_t height)
{
    std::vector<uint8_t> result(buffer.size());

    const size_t byteCount = static_cast<size_t>(width) * height;
    for (size_t index = 0; index < byteCount; index++)
    {
        result[index] = buffer[index * 3 + 0];
        result[index + 1 * byteCount] = buffer[index * 3 + 1];
        result[index + 2 * byteCount] = buffer[index * 3 + 2];
    }

    return result;
}

void convert_bottom_up_to_top_down(uint8_t* triplet_buffer, const uint32_t width, const uint32_t height)
{
    const size_t row_length = static_cast<size_t>(width) * 3;
    const size_t stride = row_length;
    std::vector<uint8_t> temp_row(row_length);

    for (size_t i = 0; i < height / 2; ++i)
    {
        memcpy(temp_row.data(), &triplet_buffer[i * stride], row_length);
        const size_t bottom_row = height - i - 1;
        memcpy(&triplet_buffer[i * stride], &triplet_buffer[bottom_row * stride], row_length);
        memcpy(&triplet_buffer[bottom_row * stride], temp_row.data(), row_length);
    }
}

std::vector<uint8_t> encode_bmp_image_to_jpegls(const bmp_image& image, const charls::interleave_mode interleave_mode, const int near_lossless)
{
    assert(image.dib_header.depth == 24);        // This function only supports 24-bit BMP pixel data.
    assert(image.dib_header.compress_type == 0); // Data needs to be stored by pixel as RGB.

    charls::jpegls_encoder encoder;
    encoder.frame_info({image.dib_header.width, static_cast<uint32_t>(image.dib_header.height), 8, 3})
        .interleave_mode(interleave_mode)
        .near_lossless(near_lossless);

    std::vector<uint8_t> buffer(encoder.estimated_destination_size());
    encoder.destination(buffer);

    encoder.write_standard_spiff_header(charls::spiff_color_space::rgb,
                                        charls::spiff_resolution_units::dots_per_centimeter,
                                        image.dib_header.vertical_resolution / 100,
                                        image.dib_header.horizontal_resolution / 100);

    size_t encoded_size;
    if (interleave_mode == charls::interleave_mode::none)
    {
        const auto planar_pixel_data = triplet_to_planar(image.pixel_data, image.dib_header.width, image.dib_header.height);
        encoded_size = encoder.encode(planar_pixel_data);
    }
    else
    {
        encoded_size = encoder.encode(image.pixel_data);
    }
    buffer.resize(encoded_size);

    return buffer;
}

void save_buffer_to_file(const void* buffer, const size_t buffer_size, const char* filename)
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
        std::cerr << message << "\n";
    }
    catch (...)
    {
        assert(false);
    }
}

struct options final
{
    const char* input_file_name;
    const char* output_file_name;
    charls::interleave_mode interleave_mode;
    int near_lossless;

    options(const int argc, char** argv)
    {
        if (argc < 3)
            throw std::runtime_error("Usage: <input_file_name> <output_file_name> [interleave_mode (0, 1, or 2), default = 0 (none)] [near_lossless, default=0 (lossless)]\n");

        input_file_name = argv[1];
        output_file_name = argv[2];

        if (argc > 3)
        {
            const long interleave_mode_arg = strtol(argv[3], nullptr, 10);
            if (interleave_mode_arg < 0 || interleave_mode_arg > 2)
                throw std::runtime_error("Argument interleave_mode needs to be 0 (none), 1 (line), or 2(sample)\n");

            interleave_mode = static_cast<charls::interleave_mode>(interleave_mode_arg);
        }
        else
        {
            interleave_mode = charls::interleave_mode::none;
        }

        if (argc > 4)
        {
            near_lossless = static_cast<int>(strtol(argv[4], nullptr, 10));
            if (near_lossless < 0 || near_lossless > 255)
                throw std::runtime_error("Argument near_lossless needs to be in the range [0,255]\n");
        }
        else
        {
            near_lossless = 0;
        }
    }
};


} // namespace


int main(const int argc, char** argv)
{
    try
    {
        std::ios::sync_with_stdio(false);
        const options options{argc, argv};

        bmp_image bmp_image{options.input_file_name};

        // Pixels in a .bmp file are stored as BGR, JPEG-LS only supports RGB color model.
        convert_bgr_to_rgb(bmp_image.pixel_data);

        // Pixels in a .bmp file are stored bottom up (when height is positive), JPEG-LS requires top down.
        if (bmp_image.dib_header.height > 0)
        {
            convert_bottom_up_to_top_down(bmp_image.pixel_data.data(), bmp_image.dib_header.width, bmp_image.dib_header.height);
        }
        else
        {
            bmp_image.dib_header.height = std::abs(bmp_image.dib_header.height);
        }

        auto encoded_buffer = encode_bmp_image_to_jpegls(bmp_image, options.interleave_mode, options.near_lossless);
        save_buffer_to_file(encoded_buffer.data(), encoded_buffer.size(), options.output_file_name);

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
