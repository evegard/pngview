#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "png.h"
#include "zlib.h"

#define SWAP_BYTES(word)    ((((word) & 0x000000ff) << 24) | \
                             (((word) & 0x0000ff00) <<  8) | \
                             (((word) & 0x00ff0000) >>  8) | \
                             (((word) & 0xff000000) >> 24))

int png_read_header(FILE *file)
{
    char buffer[8];
    size_t count = fread(&buffer[0], sizeof(char), 8, file);
    if (count != 8) {
        return 0;
    }
    return strncmp(&buffer[0], "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a", 8) == 0;
}

chunk_t *png_read_chunk(FILE *file)
{
    chunk_t *chunk = calloc(1, sizeof(chunk_t));
    size_t count = fread(&chunk->header, sizeof(chunk_header_t), 1, file);
    if (count != 1) {
        free(chunk);
        return 0;
    }
    chunk->header.length = SWAP_BYTES(chunk->header.length);
    chunk->data = malloc(chunk->header.length);
    count = fread(chunk->data, sizeof(char), chunk->header.length, file);
    if (count != chunk->header.length) {
        free(chunk->data);
        free(chunk);
        return 0;
    }
    uint32_t crc;
    count = fread(&crc, sizeof(uint32_t), 1, file);
    if (count != 1) {
        free(chunk->data);
        free(chunk);
        return 0;
    }
    return chunk;
}

png_t *png_read(FILE *file)
{
    /* Read the header. */
    if (!png_read_header(file)) {
        return 0;
    }

    /* Allocate the png_t structure. */
    png_t *png = calloc(1, sizeof(png_t));

    /* Loop through the file and read all the chunks. */
    chunk_t **next_chunk_ptr = &png->first_chunk;

    chunk_t *chunk;
    while (chunk = png_read_chunk(file)) {
        /* Link up the singly linked list of chunks. */
        *next_chunk_ptr = chunk;
        next_chunk_ptr = &chunk->next_chunk;

        /* Check for interesting chunks. */
        if (strncmp(&chunk->header.type[0], "IHDR", 4) == 0) {
            png->ihdr = (ihdr_t *)chunk->data;
        } else if (strncmp(&chunk->header.type[0], "IDAT", 4) == 0) {
            png->data = chunk->data;
            png->data_length += chunk->header.length;
        }
    }

    /* Extract some basic image properties. */
    png->width = SWAP_BYTES(png->ihdr->width);
    png->height = SWAP_BYTES(png->ihdr->height);

    /* Parse the zlib stream. */
    png->zlib = zlib_read(png->data, png->data_length);

    /* Return the png_t structure. */
    return png;
}

void png_print_chunk(chunk_t *chunk)
{
    printf("Type %c%c%c%c, size %d\n",
        chunk->header.type[0], chunk->header.type[1],
        chunk->header.type[2], chunk->header.type[3],
        chunk->header.length);
}

void png_print_information(png_t *png)
{
    printf("Header:\n");

    printf("  Width: %d px\n", png->width);
    printf("  Height: %d px\n", png->height);
    printf("  Bit depth: %d bpp\n", png->ihdr->depth);
    printf("  Color type:%s%s%s\n",
        png->ihdr->color_type & 1 ? " palette" : "",
        png->ihdr->color_type & 2 ? " color" : "",
        png->ihdr->color_type & 4 ? " alpha" : "");
    printf("  Compression method: %s\n",
        png->ihdr->compression == 0 ? "deflate" : "UNKNOWN");
    printf("  Filter method: %s\n",
        png->ihdr->filter == 0 ? "0" : "UNKNOWN");
    printf("  Interlace method: %s\n",
        png->ihdr->interlace == 0 ? "no interlace" :
        png->ihdr->interlace == 1 ? "adam7 interlace" : "UNKNOWN");

    printf("Chunks:\n");
    chunk_t *chunk = png->first_chunk;
    while (chunk) {
        printf("  ");
        png_print_chunk(chunk);
        chunk = chunk->next_chunk;
    }

    printf("Other information:\n");
    printf("  Total data length: %d\n", png->data_length);

    zlib_print_information(png->zlib);
}
