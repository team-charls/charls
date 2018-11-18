// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include <charls/charls.h>
#include <charls/jpegls_error.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>


typedef struct {
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

typedef struct {
    uint32_t header_size;             // the size of this header (40 bytes)
    uint32_t width;                   // the bitmap width in pixels
    uint32_t height;                  // the bitmap height in pixels
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


static bool bmp_read_header(FILE *fp, bmp_header_t *header)
{
    assert(fp);
    assert(header);

    return fread(&header->magic, sizeof header->magic, 1, fp) &&
           fread(&header->file_size, sizeof header->file_size, 1, fp) &&
           fread(&header->reserved, sizeof header->reserved, 1, fp) &&
           fread(&header->offset, sizeof header->offset, 1, fp) &&
           header->magic[0] == 0x42 && header->magic[1] == 0x4D;
}


static bool bmp_read_dib_header(FILE *fp, bmp_dib_header_t *header)
{
    assert(fp);
    assert(header);

    return fread(&header->header_size, sizeof header->header_size, 1, fp) &&
           fread(&header->width, sizeof header->width, 1, fp) &&
           fread(&header->height, sizeof header->height, 1, fp) &&
           fread(&header->number_planes, sizeof header->number_planes, 1, fp) &&
           fread(&header->depth, sizeof header->depth, 1, fp) &&
           fread(&header->compress_type, sizeof header->compress_type, 1, fp);
}


static void *bmp_read_pixel_data(FILE *fp, uint32_t offset, const bmp_dib_header_t *header, size_t *buffer_size)
{
    assert(fp);
    assert(header);
    assert(buffer_size);

    if (fseek(fp, offset, SEEK_SET))
        return NULL;

    *buffer_size = (size_t)header->height * header->width * 3;
    void *buffer = malloc(*buffer_size);
    if (buffer)
    {
        if (!fread(buffer, *buffer_size, 1, fp))
        {
            free(buffer);
            buffer = NULL;
        }
    }

    return buffer;
}


static void* encode_bmp_to_jpegls(const void* pixel_data, size_t pixel_data_size, const bmp_dib_header_t* header, int allowed_lossy_error, size_t* bytes_written)
{
    assert(header->depth == 24); // This function only supports 24-bit BMP pixel data.
    assert(header->compress_type == 0); // Data needs to be stored by pixel as RGB.

    struct JlsParameters params =
    {
        .interleaveMode = CHARLS_IM_SAMPLE,
        .bitsPerSample = 8,
        .components = 3
    };
    params.allowedLossyError = allowed_lossy_error;
    params.width = header->width;
    params.height = header->height;

    // Assume that compressed pixels are smaller or equal to uncompressed pixels and reserve some room for JPEG header.
    const size_t encoded_buffer_size = pixel_data_size + 1024;
    void *encoded_buffer = malloc(encoded_buffer_size);

    const int error_value = JpegLsEncode(encoded_buffer, encoded_buffer_size, bytes_written, pixel_data, pixel_data_size, &params, NULL);
    if (error_value)
    {
        printf("Failed to encode pixel data: %i, %s\n", error_value, charls_get_error_message(error_value));
        free(encoded_buffer);
        encoded_buffer = NULL;
    }

    return encoded_buffer;
}


static bool save_jpegls_file(const char *filename, const void *buffer, size_t buffer_size)
{
    assert(filename);
    assert(buffer);
    assert(buffer_size);

    bool result = false;
    FILE *stream = fopen(filename, "wb");
    if (stream)
    {
        result = fwrite(buffer, buffer_size, 1, stream);
        fclose(stream);
    }

    return result;
}


int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("Usage: input_file_name output_file_name [allowed_lossy_error, default=0 (lossless)]\n");
        return EXIT_FAILURE;
    }

    int allowed_lossy_error = 0;
    if (argc > 3)
    {
        allowed_lossy_error = strtol(argv[3], NULL, 10);
        if (allowed_lossy_error < 0 || allowed_lossy_error > 255)
        {
            printf("allowed_lossy_error needs to be in the range [0,255]\n");
            return EXIT_FAILURE;
        }
    }

    FILE *input_stream = fopen(argv[1], "rb");
    if (!input_stream)
    {
        printf("Failed to open file: %s, errno: %d\n", argv[1], errno);
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
    void *pixel_data = bmp_read_pixel_data(input_stream, header.offset, &dib_header, &buffer_size);
    fclose(input_stream);

    if (!pixel_data)
    {
        printf("Failed to read the BMP pixel data from the file: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    size_t encoded_size;
    void* encoded_data = encode_bmp_to_jpegls(pixel_data, buffer_size, &dib_header, allowed_lossy_error, &encoded_size);
    free(pixel_data);
    if (!encoded_data)
        return EXIT_FAILURE; // error already printed.

    const bool result = save_jpegls_file(argv[2], encoded_data, encoded_size);
    free(encoded_data);
    if (!result)
    {
        printf("Failed to write encoded data to the file: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}