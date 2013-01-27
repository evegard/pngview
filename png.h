#ifndef PNG_H
#define PNG_H

#include <stdint.h>
#include <stdio.h>

#include "zlib.h"

typedef struct chunk_header {
    uint32_t length;
    char type[4];
} chunk_header_t;

typedef struct chunk {
    chunk_header_t header;
    char *data;
    struct chunk *next_chunk;
} chunk_t;

typedef struct ihdr {
    uint32_t width;
    uint32_t height;

    uint8_t depth;
    uint8_t color_type;
    uint8_t compression;
    uint8_t filter;
    uint8_t interlace;
} ihdr_t;

typedef struct png {
    char *name;

    int width;
    int height;

    int depth;
    int bpp;

    int palette;
    int color;
    int alpha;

    char *comp_data;
    int comp_data_length;

    char *unfiltered_data;
    int unfiltered_data_length;

    char *data;
    int data_length;

    chunk_t *first_chunk;
    ihdr_t *ihdr;
    zlib_t *zlib;
    char *palette_data;
} png_t;

png_t *png_read(char *filename, FILE *file);
void png_print_information(png_t *png);
char *png_get_data(png_t *png);

#endif
