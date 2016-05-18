//
// (C) CharLS Team 2015, all rights reserved. See the accompanying "License.txt" for licensed use. 
//


// Use the CharLS DLL on windows, the static library on other platforms.
#ifdef WIN32
#define CHARLS_DLL 1
#else
#define CHARLS_STATIC 1
#endif

#define _CRT_SECURE_NO_DEPRECATE


#include "../../src/charls.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>



typedef struct {
    uint8_t magic[2];   /* the magic number used to identify the BMP file:
                        0x42 0x4D (Hex code points for B and M).
                        The following entries are possible:
                        BM - Windows 3.1x, 95, NT, ... etc
                        BA - OS/2 Bitmap Array
                        CI - OS/2 Color Icon
                        CP - OS/2 Color Pointer
                        IC - OS/2 Icon
                        PT - OS/2 Pointer. */
    uint32_t filesz;    /* the size of the BMP file in bytes */
    uint32_t reserved;  /* reserved. */
    uint32_t offset;    /* the offset, i.e. starting address,
                        of the byte where the bitmap data can be found. */
} bmp_header_t;

typedef struct {
    uint32_t header_sz;     /* the size of this header (40 bytes) */
    uint32_t width;         /* the bitmap width in pixels */
    uint32_t height;        /* the bitmap height in pixels */
    uint16_t nplanes;       /* the number of color planes being used.
                            Must be set to 1. */
    uint16_t depth;         /* the number of bits per pixel,
                            which is the color depth of the image.
                            Typical values are 1, 4, 8, 16, 24 and 32. */
    uint32_t compress_type; /* the compression method being used.
                            See also bmp_compression_method_t. */
    uint32_t bmp_bytesz;    /* the image size. This is the size of the raw bitmap
                            data (see below), and should not be confused
                            with the file size. */
    uint32_t hres;          /* the horizontal resolution of the image.
                            (pixel per meter) */
    uint32_t vres;          /* the vertical resolution of the image.
                            (pixel per meter) */
    uint32_t ncolors;       /* the number of colors in the color palette,
                            or 0 to default to 2<sup><i>n</i></sup>. */
    uint32_t nimpcolors;    /* the number of important colors used,
                            or 0 when every color is important;
                            generally ignored. */
} bmp_dib_header_t;


static bool bmp_read_header(FILE *fp, bmp_header_t *header)
{
    assert(fp);
    assert(header);

    return fread(&(header->magic), sizeof(header->magic), 1, fp) &&
           fread(&(header->filesz), sizeof(uint32_t), 1, fp) &&
           fread(&(header->reserved), sizeof(uint32_t), 1, fp) &&
           fread(&(header->offset), sizeof(uint32_t), 1, fp) &&
           header->magic[0] == 0x42 && header->magic[1] == 0x4D;
}


static bool bmp_read_dib_header(FILE *fp, bmp_dib_header_t *header)
{
    assert(fp);
    assert(header);

    return fread(&(header->header_sz), sizeof(uint32_t), 1, fp) &&
           fread(&(header->width), sizeof(uint32_t), 1, fp) &&
           fread(&(header->height), sizeof(uint32_t), 1, fp) &&
           fread(&(header->nplanes), sizeof(uint16_t), 1, fp) &&
           fread(&(header->depth), sizeof(uint16_t), 1, fp);
}


static void *bmp_read_pixel_data(FILE *fp, uint32_t offset, const bmp_dib_header_t *header, size_t *buffer_size)
{
    assert(fp);
    assert(header);
    assert(buffer_size);

    if (fseek(fp, offset, SEEK_SET))
        return NULL;

    *buffer_size = header->height * header->width * 3;
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


static void *encode_bmp_to_jpegls(const void *pixel_data, size_t pixel_data_size, const bmp_dib_header_t *header, size_t *bytes_writen)
{
    // This function only supports 24-bit BMP pixel data.
    // 24-BMP pixel data is stored by pixel as RGB. JPEG-LS 
    struct JlsParameters params = { .allowedLossyError = 0, .interleaveMode = CHARLS_IM_SAMPLE };

    params.width = header->width;
    params.height = header->height;
    params.bitsPerSample = 8;
    params.components = 3;

    // Assume that compressed pixels are smaller or equal to uncompressed pixels and reserver some room for JPEG header.
    size_t encoded_buffer_size = pixel_data_size + 1024;
    void *encoded_buffer = malloc(encoded_buffer_size);

    char errorMessage[CHARLS_ERROR_MESSAGE_SIZE];
    CharlsApiResultType result = JpegLsEncode(encoded_buffer, encoded_buffer_size, bytes_writen, pixel_data, pixel_data_size, &params, errorMessage);
    if (result != CHARLS_API_RESULT_OK)
    {
        printf("Failed to encode pixel data: %i, %s\n", result, errorMessage);
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
    int result = EXIT_FAILURE;

    if (argc < 3) {
        printf("Usage: [input file name] [output file name]\n");
        return result;
    }

    FILE *stream = fopen(argv[1], "rb");
    if (stream)
    {
        bmp_header_t header;
        bmp_dib_header_t dib_header;

        if (bmp_read_header(stream, &header) &&
            bmp_read_dib_header(stream, &dib_header))
        {
            size_t buffer_size;
            void *pixel_data = bmp_read_pixel_data(stream, header.offset, &dib_header, &buffer_size);
            fclose(stream);

            if (pixel_data)
            {
                size_t encoded_size;
                void *encoded_data = encode_bmp_to_jpegls(pixel_data, buffer_size, &dib_header, &encoded_size);
                free(pixel_data);

                if (encoded_data)
                {
                    if (!save_jpegls_file(argv[2], encoded_data, encoded_size))
                    {
                        printf("Failed to write encoded data to the file: %s\n", argv[2]);
                    }
                    free(encoded_data);
                }

                result = EXIT_SUCCESS;
            }
            else
            {
                printf("Failed to read the BMP pixel data from the file: %s\n", argv[1]);
            }
        }
        else
        {
            printf("Failed to read the BMP info from the file: %s\n", argv[1]);
        }
    }
    else
    {
        printf("Failed to open file: %s\n", argv[1]);
    }

    return result;
}