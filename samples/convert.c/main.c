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
    uint32_t number_important_colors; // the number of important colors used, or 0 when every color is important generally ignored.
} bmp_dib_header_t;


static bool bmp_read_header(FILE* fp, bmp_header_t* header)
{
    assert(fp);
    assert(header);

    return fread(&header->magic, sizeof header->magic, 1, fp) &&
           fread(&header->file_size, sizeof header->file_size, 1, fp) &&
           fread(&header->reserved, sizeof header->reserved, 1, fp) &&
           fread(&header->offset, sizeof header->offset, 1, fp) &&
           header->magic[0] == 0x42 && header->magic[1] == 0x4D;
}


static bool bmp_read_dib_header(FILE* fp, bmp_dib_header_t* header)
{
    assert(fp);
    assert(header);

    return fread(&header->header_size, sizeof header->header_size, 1, fp) &&
           header->header_size >= 40 &&
           fread(&header->width, sizeof header->width, 1, fp) &&
           fread(&header->height, sizeof header->height, 1, fp) &&
           fread(&header->number_planes, sizeof header->number_planes, 1, fp) &&
           fread(&header->depth, sizeof header->depth, 1, fp) &&
           fread(&header->compress_type, sizeof header->compress_type, 1, fp);
}


static void* bmp_read_pixel_data(FILE* fp, const uint32_t offset, const bmp_dib_header_t* header, size_t* buffer_size)
{
    assert(fp);
    assert(header);
    assert(buffer_size);

    if (fseek(fp, offset, SEEK_SET))
        return NULL;

    *buffer_size = (size_t)header->height * header->width * 3;
    void* buffer = malloc(*buffer_size);
    if (buffer && fread(buffer, *buffer_size, 1, fp))
        return buffer;

    free(buffer);
    return NULL;
}


static void* handle_encoder_failure(const charls_jpegls_errc error, const char* step, charls_jpegls_encoder* encoder, void* buffer)
{
    printf("Failed to %s: %i, %s\n", step, error, charls_get_error_message(error));
    charls_jpegls_encoder_destroy(encoder);
    free(buffer);
    return NULL;
}


static void triplet_to_planar(const char* triplet_buffer, char* planar_buffer, const uint32_t width, const uint32_t height)
{
    const size_t byte_count = (size_t)width * height;
    for (size_t index = 0; index < byte_count; ++index)
    {
        planar_buffer[index] = triplet_buffer[index * 3 + 0];
        planar_buffer[index + 1 * byte_count] = triplet_buffer[index * 3 + 1];
        planar_buffer[index + 2 * byte_count] = triplet_buffer[index * 3 + 2];
    }
}


static void convert_bgr_to_rgb(char* triplet_buffer, const size_t size)
{
    for (size_t i = 0; i < size; i += 3)
    {
        const char temp = triplet_buffer[i];
        triplet_buffer[i] = triplet_buffer[i + 2];
        triplet_buffer[i + 2] = temp;
    }
}


static bool convert_bottom_up_to_top_down(char* triplet_buffer, const uint32_t width, const uint32_t height)
{
    const size_t row_length = (size_t)width * 3;
    const size_t stride = row_length;
    char* temp_row = malloc(row_length);
    if (!temp_row)
        return false;

    for (size_t i = 0; i < height / 2; ++i)
    {
        memcpy(temp_row, &triplet_buffer[i * stride], row_length);
        const size_t bottom_row = height - i - 1;
        memcpy(&triplet_buffer[i * stride], &triplet_buffer[bottom_row * stride], row_length);
        memcpy(&triplet_buffer[bottom_row * stride], temp_row, row_length);
    }

    return true;
}


static void* encode_bmp_to_jpegls(const void* pixel_data, const size_t pixel_data_size, const bmp_dib_header_t* header, const charls_interleave_mode interleave_mode, const int near_lossless, size_t* bytes_written)
{
    assert(header->depth == 24);        // This function only supports 24-bit BMP pixel data.
    assert(header->compress_type == 0); // Data needs to be stored by pixel as RGB.

    charls_jpegls_encoder* encoder = charls_jpegls_encoder_create();
    if (!encoder)
    {
        printf("Failed to create JPEG-LS encoder\n");
        return NULL;
    }

    charls_frame_info frame_info = {.bits_per_sample = 8, .component_count = 3};
    frame_info.width = header->width;
    frame_info.height = header->height;
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

    error = charls_jpegls_encoder_write_standard_spiff_header(encoder,
                                                              CHARLS_SPIFF_COLOR_SPACE_RGB,
                                                              CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_CENTIMETER,
                                                              header->vertical_resolution / 100, header->vertical_resolution / 100);
    if (error)
    {
        return handle_encoder_failure(error, "write_standard_spiff_header", encoder, encoded_buffer);
    }

    if (interleave_mode == CHARLS_INTERLEAVE_MODE_NONE)
    {
        void* planar_pixel_data = malloc(pixel_data_size);
        if (!planar_pixel_data)
        {
            return handle_encoder_failure(CHARLS_JPEGLS_ERRC_NOT_ENOUGH_MEMORY, "malloc", encoder, encoded_buffer);
        }

        triplet_to_planar(pixel_data, planar_pixel_data, header->width, header->height);
        error = charls_jpegls_encoder_encode_from_buffer(encoder, planar_pixel_data, pixel_data_size, 0);
        free(planar_pixel_data);
    }
    else
    {
        error = charls_jpegls_encoder_encode_from_buffer(encoder, pixel_data, pixel_data_size, 0);
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
    const char* input_file_name;
    const char* output_file_name;
    charls_interleave_mode interleave_mode;
    int near_lossless;
} options_t;


static bool parse_command_line_options(const int argc, char* argv[], options_t* options)
{
    if (argc < 3)
    {
        printf("Usage: <input_file_name> <output_file_name> [interleave_mode (0, 1, or 2), default = 0 (none)] [near_lossless, default=0 (lossless)]\n");
        return false;
    }

    options->input_file_name = argv[1];
    options->output_file_name = argv[2];

    if (argc > 3)
    {
        options->interleave_mode = (charls_interleave_mode)strtol(argv[3], NULL, 10);
        if (options->interleave_mode < CHARLS_INTERLEAVE_MODE_NONE || options->interleave_mode > CHARLS_INTERLEAVE_MODE_SAMPLE)
        {
            printf("Argument interleave_mode needs to be 0 (none), 1 (line), or 2(sample)\n");
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
            printf("Argument near_lossless needs to be in the range [0,255]\n");
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

    FILE* input_stream = fopen(options.input_file_name, "rb");
    if (!input_stream)
    {
        printf("Failed to open file: %s, errno: %d\n", options.input_file_name, errno);
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

    size_t buffer_size;
    void* pixel_data = bmp_read_pixel_data(input_stream, header.offset, &dib_header, &buffer_size);
    fclose(input_stream);

    if (!pixel_data)
    {
        printf("Failed to read the BMP pixel data from the file: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Pixels in a .bmp file are stored as BGR, JPEG-LS only supports RGB color model.
    convert_bgr_to_rgb(pixel_data, buffer_size);

    // Pixels in a .bmp file are stored bottom up (when height is positive), JPEG-LS requires top down.
    if (dib_header.height > 0)
    {
        if (!convert_bottom_up_to_top_down(pixel_data, dib_header.width, dib_header.height))
        {
            printf("Failed to convert the pixels from bottom up to top down\n");
            return EXIT_FAILURE;
        }
    }
    else
    {
        dib_header.height = abs(dib_header.height);
    }

    size_t encoded_size;
    void* encoded_data = encode_bmp_to_jpegls(pixel_data, buffer_size, &dib_header, options.interleave_mode, options.near_lossless, &encoded_size);
    free(pixel_data);
    if (!encoded_data)
        return EXIT_FAILURE; // error already printed.

    const bool result = save_jpegls_file(options.output_file_name, encoded_data, encoded_size);
    free(encoded_data);
    if (!result)
    {
        printf("Failed to write encoded data to the file: %s\n", options.output_file_name);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
