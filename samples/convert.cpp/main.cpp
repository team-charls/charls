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

constexpr size_t bytes_per_rgb_pixel{3};

void convert_bgr_to_rgb(std::vector<uint8_t>& triplet_buffer, const size_t width, const size_t height,
                        const size_t stride) noexcept
{
    for (size_t line{}; line < height; ++line)
    {
        const auto line_start{line * stride};
        for (size_t pixel{}; pixel < width; ++pixel)
        {
            const auto column{pixel * bytes_per_rgb_pixel};
            std::swap(triplet_buffer[line_start + column], triplet_buffer[line_start + column + 2]);
        }
    }
}

std::vector<uint8_t> triplet_to_planar(const std::vector<uint8_t>& buffer, const size_t width, const size_t height,
                                       const size_t stride)
{
    std::vector<uint8_t> result(bytes_per_rgb_pixel * width * height);
    const size_t byte_count_plane{width * height};

    size_t plane_column{};
    for (size_t line{}; line < height; ++line)
    {
        const auto line_start{line * stride};
        for (size_t pixel{}; pixel < width; ++pixel)
        {
            const auto column{line_start + pixel * bytes_per_rgb_pixel};
            result[plane_column] = buffer[column];
            result[plane_column + 1 * byte_count_plane] = buffer[column + 1];
            result[plane_column + 2 * byte_count_plane] = buffer[column + 2];
            ++plane_column;
        }
    }

    return result;
}

void convert_bottom_up_to_top_down(uint8_t* triplet_buffer, const size_t width, const size_t height, const size_t stride)
{
    const size_t row_length{width * bytes_per_rgb_pixel};
    std::vector<uint8_t> temp_row(row_length);

    for (size_t i{}; i < height / 2; ++i)
    {
        memcpy(temp_row.data(), &triplet_buffer[i * stride], row_length);
        const size_t bottom_row{height - i - 1};
        memcpy(&triplet_buffer[i * stride], &triplet_buffer[bottom_row * stride], row_length);
        memcpy(&triplet_buffer[bottom_row * stride], temp_row.data(), row_length);
    }
}

std::vector<uint8_t> encode_bmp_image_to_jpegls(const bmp_image& image, const charls::interleave_mode interleave_mode,
                                                const int near_lossless)
{
    assert(image.dib_header.depth == 24);        // This function only supports 24-bit BMP pixel data.
    assert(image.dib_header.compress_type == 0); // Data needs to be stored by pixel as RGB.

    charls::jpegls_encoder encoder;
    encoder.frame_info({image.dib_header.width, static_cast<uint32_t>(image.dib_header.height), 8, bytes_per_rgb_pixel})
        .interleave_mode(interleave_mode)
        .near_lossless(near_lossless);

    std::vector<uint8_t> buffer(encoder.estimated_destination_size());
    encoder.destination(buffer);

    // The resolution in BMP files is often 0 to indicate that no resolution has been defined.
    // The SPIFF header specification requires however that VRES and HRES are never 0.
    // The ISO 10918-3 recommendation for these cases is to define that the pixels should be interpreted as a square.
    if (image.dib_header.vertical_resolution < 100 || image.dib_header.horizontal_resolution < 100)
    {
        encoder.write_standard_spiff_header(charls::spiff_color_space::rgb, charls::spiff_resolution_units::aspect_ratio, 1,
                                            1);
    }
    else
    {
        encoder.write_standard_spiff_header(
            charls::spiff_color_space::rgb, charls::spiff_resolution_units::dots_per_centimeter,
            image.dib_header.vertical_resolution / 100, image.dib_header.horizontal_resolution / 100);
    }

    size_t encoded_size;
    if (interleave_mode == charls::interleave_mode::none)
    {
        const auto planar_pixel_data{triplet_to_planar(image.pixel_data, image.dib_header.width,
                                                       static_cast<size_t>(image.dib_header.height), image.stride)};
        encoded_size = encoder.encode(planar_pixel_data);
    }
    else
    {
        encoded_size = encoder.encode(image.pixel_data, image.stride);
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
    output.exceptions(std::ios::failbit | std::ios::badbit);

    output.write(static_cast<const char*>(buffer), static_cast<std::streamsize>(buffer_size));
}

void log_failure(const char* message) noexcept
{
    try
    {
        std::cerr << message << "\n";
    }
    catch (...)
    {
        // Catch and ignore all exceptions,to ensure a noexcept log function (but warn in debug builds)
        assert(false);
    }
}

struct options final
{
    const char* input_filename;
    const char* output_filename;
    charls::interleave_mode interleave_mode;
    int near_lossless;

    options(const int argc, char** argv)
    {
        if (argc < 3)
        {
            throw std::runtime_error("Usage: <input_filename> <output_filename> [interleave-mode (none, line, or sample), "
                                     "default = none] [near-lossless, default = 0 (lossless)]\n");
        }

        input_filename = argv[1];
        output_filename = argv[2];

        if (argc > 3)
        {
            interleave_mode = string_to_interleave_mode(argv[3]);
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

private:
    static charls::interleave_mode string_to_interleave_mode(const char* argument)
    {
        if (strcmp(argument, "none") == 0)
            return charls::interleave_mode::none;

        if (strcmp(argument, "line") == 0)
            return charls::interleave_mode::line;

        if (strcmp(argument, "sample") == 0)
            return charls::interleave_mode::sample;

        throw std::runtime_error("Argument interleave-mode needs to be: none, line or sample\n");
    }
};

} // namespace


int main(const int argc, char** argv)
{
    try
    {
        std::ios::sync_with_stdio(false);
        const options options{argc, argv};

        bmp_image bmp_image{options.input_filename};

        // Pixels in the BMP file format are stored bottom up (when the height parameter is positive), JPEG-LS requires top
        // down.
        if (bmp_image.dib_header.height > 0)
        {
            convert_bottom_up_to_top_down(bmp_image.pixel_data.data(), bmp_image.dib_header.width,
                                          static_cast<size_t>(bmp_image.dib_header.height), bmp_image.stride);
        }
        else
        {
            bmp_image.dib_header.height = std::abs(bmp_image.dib_header.height);
        }

        // Pixels in the BMP file format are stored as BGR. JPEG-LS (SPIFF header) only supports the RGB color model.
        // Note: without the optional SPIFF header no color information is stored in the JPEG-LS file and the common
        // assumption is RGB.
        convert_bgr_to_rgb(bmp_image.pixel_data, bmp_image.dib_header.width,
                           static_cast<size_t>(bmp_image.dib_header.height), bmp_image.stride);

        auto encoded_buffer{encode_bmp_image_to_jpegls(bmp_image, options.interleave_mode, options.near_lossless)};
        save_buffer_to_file(encoded_buffer.data(), encoded_buffer.size(), options.output_filename);

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
