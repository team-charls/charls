#define CHARLS_STATIC 1
#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "../../src/interface.h"


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
    uint16_t creator1;  /* reserved. */
    uint16_t creator2;  /* reserved. */
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


static void bmp_read_header(FILE *fp, bmp_header_t *header)
{
    fread(&(header->magic), sizeof(header->magic), 1, fp);
    fread(&(header->filesz), sizeof(uint32_t), 1, fp);
    fread(&(header->creator1), sizeof(uint16_t), 1, fp);
    fread(&(header->creator2), sizeof(uint16_t), 1, fp);
    fread(&(header->offset), sizeof(uint32_t), 1, fp);
}


static void bmp_read_dib_header(FILE *fp, bmp_dib_header_t *header)
{
    fread(&(header->header_sz), sizeof(uint32_t), 1, fp);
    fread(&(header->width), sizeof(uint32_t), 1, fp);
    fread(&(header->height), sizeof(uint32_t), 1, fp);
    fread(&(header->nplanes), sizeof(uint16_t), 1, fp);
    fread(&(header->depth), sizeof(uint16_t), 1, fp);
}

//static bool 

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("Usage: [input file name] [output file name]\n");
        return EXIT_FAILURE;
    }

    FILE *fp;
    int row;
    unsigned char *buf;
    bmp_header_t header;
    bmp_dib_header_t dib_header;
    struct JlsParameters info;

    int result = EXIT_FAILURE;

    /* Create the file */
    if ((fp = fopen(argv[1], "rb")))
    {
        bmp_read_header(fp, &header);


        bmp_read_dib_header(fp, &dib_header);
        result = EXIT_SUCCESS;
    }


    size_t bufferSize;
    void* uncompressedData;
    void* compressedData;

    size_t byteCountWritten;

   
    //result = JpegLsEncode(compressedData, bufferSize, &byteCountWritten, uncompressedData, bufferSize, &info);

    return result;
}