// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include <charls/charls.h>

#include <assert.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const static size_t bytes_per_rgb_pixel = 3;

typedef struct
{
    uint8_t magic[2];   // the magic number used to identify the BMP file:
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
} bmp_header_t;

typedef struct
{
    uint32_t header_size;             // the size of this header (40 bytes)
    uint32_t width;                   // the bitmap width in pixels
    int32_t height;                   // the bitmap height in pixels
    uint16_t number_planes;           // the number of color planes being used. Must be set to 1
    uint16_t depth;                   // the number of bits per pixel,which is the color depth of the image.
                                      // Typical values are 1, 4, 8, 16, 24 and 32.
    uint32_t compress_type;           // the compression method being used.
    uint32_t bmp_byte_size;           // the image size. This is the size of the raw bitmap
                                      // data (see below), and should not be confused with the file size.
    uint32_t horizontal_resolution;   // the horizontal resolution of the image. (pixel per meter)
    uint32_t vertical_resolution;     // the vertical resolution of the image. (pixel per meter)
    uint32_t number_colors;           // the number of colors in the color palette, or 0 to default to 2^depth.
    uint32_t number_important_colors; // the number of important colors used, or 0 when every color is important generally
                                      // ignored.
} bmp_dib_header_t;


static bool bmp_read_header(FILE* fp, bmp_header_t* header)
{
    assert(fp);
    assert(header);

    return fread(&header->magic, sizeof header->magic, 1, fp) &&
           fread(&header->file_size, sizeof header->file_size, 1, fp) &&
           fread(&header->reserved, sizeof header->reserved, 1, fp) &&
           fread(&header->offset, sizeof header->offset, 1, fp) && header->magic[0] == 0x42 && header->magic[1] == 0x4D;
}


static bool bmp_read_dib_header(FILE* fp, bmp_dib_header_t* header)
{
    assert(fp);
    assert(header);

    return fread(&header->header_size, sizeof header->header_size, 1, fp) && header->header_size >= 40 &&
           fread(&header->width, sizeof header->width, 1, fp) && fread(&header->height, sizeof header->height, 1, fp) &&
           fread(&header->number_planes, sizeof header->number_planes, 1, fp) &&
           fread(&header->depth, sizeof header->depth, 1, fp) &&
           fread(&header->compress_type, sizeof header->compress_type, 1, fp) &&
           fread(&header->bmp_byte_size, sizeof header->bmp_byte_size, 1, fp) &&
           fread(&header->horizontal_resolution, sizeof header->horizontal_resolution, 1, fp) &&
           fread(&header->vertical_resolution, sizeof header->vertical_resolution, 1, fp) &&
           fread(&header->number_colors, sizeof header->number_colors, 1, fp) &&
           fread(&header->number_important_colors, sizeof header->number_important_colors, 1, fp);
}


static void* bmp_read_pixel_data(FILE* fp, const uint32_t offset, const bmp_dib_header_t* dib_header, size_t* stride)
{
    assert(fp);
    assert(dib_header);
    assert(stride);
    assert(dib_header->compress_type == 0);
    assert(dib_header->depth == 24);

    if (fseek(fp, (long)offset, SEEK_SET))
        return NULL;

    // The BMP format requires that the size of each row is rounded up to a multiple of 4 bytes by padding.
    *stride = ((dib_header->width * bytes_per_rgb_pixel) + 3) / 4 * 4;

    const size_t buffer_size = (size_t)dib_header->height * *stride;
    void* buffer = malloc(buffer_size);
    if (!buffer)
        return NULL;

    if (fread(buffer, buffer_size, 1, fp) != 1)
    {
        free(buffer);
        return NULL;
    }

    return buffer;
}


static void* handle_encoder_failure(const charls_jpegls_errc error, const char* step, charls_jpegls_encoder* encoder,
                                    void* buffer)
{
    printf("Failed to %s: %i, %s\n", step, error, charls_get_error_message(error));
    charls_jpegls_encoder_destroy(encoder);
    free(buffer);
    return NULL;
}


static void convert_bgr_to_rgb(uint8_t* triplet_buffer, const size_t width, const size_t height, const size_t stride)
{
    for (size_t line = 0; line < height; ++line)
    {
        const size_t line_start = line * stride;
        for (size_t pixel = 0; pixel < width; ++pixel)
        {
            const size_t column = pixel * bytes_per_rgb_pixel;

            const size_t a = line_start + column;
            const size_t b = line_start + column + 2;

            const uint8_t temp = triplet_buffer[a];
            triplet_buffer[a] = triplet_buffer[b];
            triplet_buffer[b] = temp;
        }
    }
}


static void triplet_to_planar(const uint8_t* triplet_buffer, uint8_t* planar_buffer, const size_t width, const size_t height,
                              const size_t stride)
{
    const size_t byte_count_plane = width * height;
    size_t plane_column = 0;

    for (size_t line = 0; line < height; ++line)
    {
        const size_t line_start = line * stride;
        for (size_t pixel = 0; pixel < width; ++pixel)
        {
            const size_t column = line_start + pixel * bytes_per_rgb_pixel;

            planar_buffer[plane_column] = triplet_buffer[column];
            planar_buffer[plane_column + 1 * byte_count_plane] = triplet_buffer[column + 1];
            planar_buffer[plane_column + 2 * byte_count_plane] = triplet_buffer[column + 2];
            ++plane_column;
        }
    }
}


static bool convert_bottom_up_to_top_down(uint8_t* triplet_buffer, const size_t width, const size_t height,
                                          const size_t stride)
{
    const size_t row_length = width * bytes_per_rgb_pixel;
    void* temp_row = malloc(row_length);
    if (!temp_row)
        return false;

    for (size_t i = 0; i < height / 2; ++i)
    {
        memcpy(temp_row, &triplet_buffer[i * stride], row_length);
        const size_t bottom_row = height - i - 1;
        memcpy(&triplet_buffer[i * stride], &triplet_buffer[bottom_row * stride], row_length);
        memcpy(&triplet_buffer[bottom_row * stride], temp_row, row_length);
    }

    free(temp_row);

    return true;
}


static void* encode_bmp_to_jpegls(const void* pixel_data, const size_t stride, const bmp_dib_header_t* header,
                                  const charls_interleave_mode interleave_mode, const int near_lossless,
                                  size_t* bytes_written)
{
    assert(header->depth == 24 && "This function only supports 24-bit BMP pixel data.");
    assert(header->compress_type == 0 && "Data needs to be stored by pixel as RGB.");
    assert(header->width > 0 && "0 width not supported, may cause 0 byte malloc");
    assert(header->height > 0 && "0 and negative height not supported, may cause 0 byte malloc");

    charls_jpegls_encoder* encoder = charls_jpegls_encoder_create();
    if (!encoder)
    {
        printf("Failed to create JPEG-LS encoder\n");
        return NULL;
    }

    charls_frame_info frame_info = {.bits_per_sample = 8, .component_count = 3};
    frame_info.width = header->width;
    frame_info.height = (uint32_t)header->height;
    charls_jpegls_errc error = charls_jpegls_encoder_set_frame_info(encoder, &frame_info);
    if (error)
    {
        return handle_encoder_failure(error, "set frame_info", encoder, NULL);
    }

    error = charls_jpegls_encoder_set_interleave_mode(encoder, interleave_mode);
    if (error)
    {
        return handle_encoder_failure(error, "set near interleave mode", encoder, NULL);
    }

    error = charls_jpegls_encoder_set_near_lossless(encoder, near_lossless);
    if (error)
    {
        return handle_encoder_failure(error, "set near lossless", encoder, NULL);
    }

    size_t encoded_buffer_size;
    error = charls_jpegls_encoder_get_estimated_destination_size(encoder, &encoded_buffer_size);
    if (error)
    {
        return handle_encoder_failure(error, "get estimated destination size", encoder, NULL);
    }

    void* encoded_buffer = malloc(encoded_buffer_size);
    if (!encoded_buffer)
    {
        return handle_encoder_failure(error, "malloc failed", encoder, NULL);
    }

    error = charls_jpegls_encoder_set_destination_buffer(encoder, encoded_buffer, encoded_buffer_size);
    if (error)
    {
        return handle_encoder_failure(error, "set destination buffer", encoder, encoded_buffer);
    }

    // The resolution in BMP files is often 0 to indicate that no resolution has been defined.
    // The SPIFF header specification requires however that VRES and HRES are never 0.
    // The ISO 10918-3 recommendation for these cases is to define that the pixels should be interpreted as a square.
    if (header->vertical_resolution < 100 || header->horizontal_resolution < 100)
    {
        error = charls_jpegls_encoder_write_standard_spiff_header(encoder, CHARLS_SPIFF_COLOR_SPACE_RGB,
                                                                  CHARLS_SPIFF_RESOLUTION_UNITS_ASPECT_RATIO, 1, 1);
    }
    else
    {
        error = charls_jpegls_encoder_write_standard_spiff_header(
            encoder, CHARLS_SPIFF_COLOR_SPACE_RGB, CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_CENTIMETER,
            header->vertical_resolution / 100, header->horizontal_resolution / 100);
    }

    if (error)
    {
        return handle_encoder_failure(error, "write_standard_spiff_header", encoder, encoded_buffer);
    }

    if (interleave_mode == CHARLS_INTERLEAVE_MODE_NONE)
    {
        const size_t pixel_data_size = (size_t)header->height * header->width * bytes_per_rgb_pixel;
        void* planar_pixel_data = malloc(pixel_data_size);
        if (!planar_pixel_data)
        {
            return handle_encoder_failure(CHARLS_JPEGLS_ERRC_NOT_ENOUGH_MEMORY, "malloc", encoder, encoded_buffer);
        }

        triplet_to_planar(pixel_data, planar_pixel_data, header->width, (size_t)header->height, stride);
        error = charls_jpegls_encoder_encode_from_buffer(encoder, planar_pixel_data, pixel_data_size, 0);
        free(planar_pixel_data);
    }
    else
    {
        const size_t pixel_data_size = (size_t)header->height * stride;
        error = charls_jpegls_encoder_encode_from_buffer(encoder, pixel_data, pixel_data_size, (uint32_t)stride);
    }

    if (error)
    {
        return handle_encoder_failure(error, "encode", encoder, encoded_buffer);
    }

    error = charls_jpegls_encoder_get_bytes_written(encoder, bytes_written);
    if (error)
    {
        return handle_encoder_failure(error, "get bytes written", encoder, encoded_buffer);
    }

    charls_jpegls_encoder_destroy(encoder);

    return encoded_buffer;
}


static bool save_jpegls_file(const char* filename, const void* buffer, const size_t buffer_size)
{
    assert(filename);
    assert(buffer);
    assert(buffer_size);

    bool result = false;
    FILE* stream = fopen(filename, "wb");
    if (stream)
    {
        result = fwrite(buffer, buffer_size, 1, stream);
        fclose(stream);
    }

    return result;
}


typedef struct
{
    const char* input_filename;
    const char* output_filename;
    charls_interleave_mode interleave_mode;
    int near_lossless;
} options_t;


static bool parse_command_line_options(const int argc, char* argv[], options_t* options)
{
    if (argc < 3)
    {
        printf("Usage: <input-filename> <output-filename> [interleave-mode (none, line or sample), default = none] "
               "[near-lossless, default=0 (lossless)]\n");
        return false;
    }

    options->input_filename = argv[1];
    options->output_filename = argv[2];

    if (argc > 3)
    {
        if (strcmp(argv[3], "none") == 0)
        {
            options->interleave_mode = CHARLS_INTERLEAVE_MODE_NONE;
        }
        else if (strcmp(argv[3], "line") == 0)
        {
            options->interleave_mode = CHARLS_INTERLEAVE_MODE_LINE;
        }
        else if (strcmp(argv[3], "sample") == 0)
        {
            options->interleave_mode = CHARLS_INTERLEAVE_MODE_SAMPLE;
        }
        else
        {
            printf("Argument interleave-mode needs to be: none, line or sample\n");
            return false;
        }
    }
    else
    {
        options->interleave_mode = CHARLS_INTERLEAVE_MODE_NONE;
    }

    if (argc > 4)
    {
        options->near_lossless = (int)strtol(argv[4], NULL, 10);
        if (options->near_lossless < 0 || options->near_lossless > 255)
        {
            printf("Argument near-lossless needs to be in the range [0,255]\n");
            return false;
        }
    }
    else
    {
        options->near_lossless = 0;
    }

    return true;
}


int main(const int argc, char* argv[])
{
    options_t options;
    if (!parse_command_line_options(argc, argv, &options))
        return EXIT_FAILURE;

    FILE* input_stream = fopen(options.input_filename, "rb");
    if (!input_stream)
    {
        printf("Failed to open file: %s, errno: %d\n", options.input_filename, errno);
        return EXIT_FAILURE;
    }

    bmp_header_t header;
    bmp_dib_header_t dib_header;
    if (!bmp_read_header(input_stream, &header) || !bmp_read_dib_header(input_stream, &dib_header))
    {
        printf("Failed to read the BMP info from the file: %s\n", argv[1]);
        fclose(input_stream);
        return EXIT_FAILURE;
    }

    if (dib_header.compress_type != 0 || dib_header.depth != 24)
    {
        printf("Can only convert uncompressed 24 bits BMP files");
        fclose(input_stream);
        return EXIT_FAILURE;
    }

    if (dib_header.width == 0 || dib_header.height == 0)
    {
        printf("Can only process an image that is 1 x 1 or bigger");
        fclose(input_stream);
        return EXIT_FAILURE;
    }

    size_t stride;
    void* pixel_data = bmp_read_pixel_data(input_stream, header.offset, &dib_header, &stride);
    fclose(input_stream);

    if (!pixel_data)
    {
        printf("Failed to read the BMP pixel data from the file: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Pixels in the BMP file format are stored bottom up (when the height parameter is positive), JPEG-LS requires top down.
    if (dib_header.height > 0)
    {
        if (!convert_bottom_up_to_top_down(pixel_data, dib_header.width, (size_t)dib_header.height, stride))
        {
            printf("Failed to convert the pixels from bottom up to top down\n");
            return EXIT_FAILURE;
        }
    }
    else
    {
        dib_header.height = abs(dib_header.height);
    }

    // Pixels in the BMP file format are stored as BGR. JPEG-LS (SPIFF header) only supports the RGB color model.
    // Note: without the optional SPIFF header no color information is stored in the JPEG-LS file and the common assumption
    // is RGB.
    convert_bgr_to_rgb(pixel_data, dib_header.width, (size_t)dib_header.height, stride);

    size_t encoded_size;
    void* encoded_data =
        encode_bmp_to_jpegls(pixel_data, stride, &dib_header, options.interleave_mode, options.near_lossless, &encoded_size);
    free(pixel_data);
    if (!encoded_data)
        return EXIT_FAILURE; // error already printed.

    const bool result = save_jpegls_file(options.output_filename, encoded_data, encoded_size);
    free(encoded_data);
    if (!result)
    {
        printf("Failed to write encoded data to the file: %s\n", options.output_filename);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
