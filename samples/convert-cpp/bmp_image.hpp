// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cstddef>
#include <fstream>
#include <vector>

class bmp_image final
{
public:
    struct bmp_header final
    {
        uint16_t magic;     // the magic number used to identify the BMP file:
                            // 0x42 0x4D (Hex code points for B and M).
                            // The following entries are possible:
                            // BM - Windows 3.1x, 95, NT, ... etc
                            // BA - OS/2 Bitmap Array
                            // CI - OS/2 Color Icon
                            // CP - OS/2 Color Pointer
                            // IC - OS/2 Icon
                            // PT - OS/2 Pointer.
        uint32_t file_size; // the size of the BMP file in bytes
        uint32_t reserved;  // reserved.
        uint32_t offset;    // the offset, i.e. starting address, of the byte where the bitmap data can be found.
    };

    struct bmp_dib_header final
    {
        uint32_t header_size;             // the size of this header (40 bytes)
        uint32_t width;                   // the bitmap width in pixels
        int32_t height;                   // the bitmap height in pixels (if negative image is top-down)
        uint16_t number_planes;           // the number of color planes being used. Must be set to 1
        uint16_t depth;                   // the number of bits per pixel,which is the color depth of the image.
                                          // Typical values are 1, 4, 8, 16, 24 and 32.
        uint32_t compress_type;           // the compression method being used.
        uint32_t bmp_byte_size;           // the image size. This is the size of the raw bitmap
                                          // data (see below), and should not be confused with the file size.
        uint32_t horizontal_resolution;   // the horizontal resolution of the image. (pixel per meter)
        uint32_t vertical_resolution;     // the vertical resolution of the image. (pixel per meter)
        uint32_t number_colors;           // the number of colors in the color palette, or 0 to default to 2^depth.
        uint32_t number_important_colors; // the number of important colors used, or 0 when every color is important
                                          // generally ignored.
    };

    explicit bmp_image(const char* filename)
    {
        std::ifstream input;
        input.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
        input.open(filename, std::ios_base::in | std::ios_base::binary);

        header = read_bmp_header(input);
        if (header.magic != 0x4D42)
            throw std::istream::failure("Missing BMP identifier");

        dib_header = read_dib_header(input);
        if (dib_header.header_size < 40 || dib_header.compress_type != 0 || dib_header.depth != 24)
            throw std::istream::failure("Can only read uncompressed 24 bits BMP files");

        if (dib_header.width == 0 || dib_header.height == 0)
            throw std::istream::failure("Can only process an image that is 1 x 1 or bigger");

        // The BMP format requires that the size of each row is rounded up to a multiple of 4 bytes by padding.
        constexpr int bytes_per_pixel{3};
        stride = ((dib_header.width * bytes_per_pixel) + 3) / 4 * 4;

        pixel_data = read_pixel_data(input, header.offset, dib_header.height, stride);
    }

    bmp_header header;
    bmp_dib_header dib_header;
    uint32_t stride{};
    std::vector<uint8_t> pixel_data;

private:
    static bmp_header read_bmp_header(std::istream& input)
    {
        bmp_header result{};

        read(input, result.magic);
        read(input, result.file_size);
        read(input, result.reserved);
        read(input, result.offset);

        return result;
    }

    static bmp_dib_header read_dib_header(std::istream& input)
    {
        bmp_dib_header result{};

        read(input, result.header_size);
        read(input, result.width);
        read(input, result.height);
        read(input, result.number_planes);
        read(input, result.depth);
        read(input, result.compress_type);
        read(input, result.bmp_byte_size);
        read(input, result.horizontal_resolution);
        read(input, result.vertical_resolution);
        read(input, result.number_colors);
        read(input, result.number_important_colors);

        return result;
    }

    static std::vector<uint8_t> read_pixel_data(std::istream& input, const uint32_t offset, const int32_t height,
                                                const uint32_t stride)
    {
        input.seekg(offset);

        std::vector<uint8_t> pixel_data(static_cast<size_t>(height) * stride);
        input.read(reinterpret_cast<char*>(pixel_data.data()), static_cast<std::streamsize>(pixel_data.size()));

        return pixel_data;
    }

    template<typename T>
    static void read(std::istream& input, T& value)
    {
        input.read(reinterpret_cast<char*>(&value), sizeof value);
    }
};
