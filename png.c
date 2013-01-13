#include <math.h>
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

char *png_get_combined_data(png_t *png)
{
    char *comp_data = malloc(png->comp_data_length);
    int position = 0;

    chunk_t *chunk = png->first_chunk;
    while (chunk) {
        if (strncmp(&chunk->header.type[0], "IDAT", 4) == 0) {
            memcpy(&comp_data[position], chunk->data, chunk->header.length);
            position += chunk->header.length;
        }
        chunk = chunk->next_chunk;
    }

    return comp_data;
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
            png->comp_data_length += chunk->header.length;
        }
    }

    /* Extract some basic image properties. */
    png->width = SWAP_BYTES(png->ihdr->width);
    png->height = SWAP_BYTES(png->ihdr->height);

    /* Combine all the IDAT data parts into a common data array. */
    png->comp_data = png_get_combined_data(png);

    /* Calculate the final data length for the raw pixel data. */
    /* TODO: This calculation is very arbitrary. */
    png->data_length = png->width * png->height * 3 * 2;

    /* Parse the zlib header. */
    png->zlib = zlib_read(png->comp_data, png->comp_data_length,
        png->data_length);

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
    printf("  Total data length: %d\n", png->comp_data_length);

    zlib_print_information(png->zlib);
}

/* Algorithm from RFC 2083 (http://tools.ietf.org/html/rfc2083#page-35). */
int png_paeth_predictor(int a, int b, int c)
{
    int p = a + b - c;
    int pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
    if (pa <= pb && pa <= pc) {
        return a;
    } else if (pc <= pc) {
        return b;
    } else {
        return c;
    }
}

char *png_get_data(png_t *png)
{
    if (png->data) {
        return png->data;
    }

    printf("\nDeflating...\n\n");
    char *filtered = zlib_get_data(png->zlib);

    printf("\nReading scanlines...\n\n");
    char *unfiltered = png->data = malloc(png->data_length);
    int filtered_pos = 0, unfiltered_pos = 0;
    char black[3] = { 0, 0, 0 };

    for (int row = 0; row < png->height; row++) {
        int filter = filtered[filtered_pos++];

        for (int col = 0; col < png->width; col++) {
            /* Pointers to neighbouring pixels in the unfiltered image. */
            char *top, *left, *topleft;

            /* Populate the top pointer. */
            if (row > 0) {
                top = &unfiltered[unfiltered_pos - 4*png->width];
            } else {
                top = &black[0];
            }

            /* Populate the left pointer. */
            if (col > 0) {
                left = &unfiltered[unfiltered_pos - 4];
            } else {
                left = &black[0];
            }

            /* Populate the topleft pointer. */
            if (row > 0 && col > 0) {
                topleft = &unfiltered[unfiltered_pos - 4*(png->width + 1)];
            } else {
                topleft = &black[0];
            }

            /* Step through the 3 bytes of this pixel and reverse the
             * filter used. */

            /* TODO: Don't assume 24 bpp. */

            for (int i = 0; i < 3; i++) {
                char filtered_byte = filtered[filtered_pos + i];
                char unfiltered_byte;

                switch (filter) {
                    case 1:
                        unfiltered_byte = filtered_byte + left[i]; break;
                    case 2:
                        unfiltered_byte = filtered_byte + top[i]; break;
                    case 3:
                        unfiltered_byte = filtered_byte +
                            floor((left[i] + top[i]) / 2);
                        break;
                    case 4:
                        unfiltered_byte = filtered_byte +
                            png_paeth_predictor(left[i],top[i],topleft[i]);
                        break;
                    default:
                        printf("Unknown filter %d\n", filter);
                        /* Fall through. */
                    case 0:
                        unfiltered_byte = filtered_byte; break;
                }

                unfiltered[unfiltered_pos + i] = unfiltered_byte;
            }

            filtered_pos += 3;
            unfiltered_pos += 4;
        }
    }

    return png->data;
}
