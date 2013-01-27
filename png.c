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
    while ((chunk = png_read_chunk(file)) != 0) {
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
    png->depth = png->ihdr->depth;
    png->palette = png->ihdr->color_type & 1;
    png->color = png->ihdr->color_type & 2;
    png->alpha = png->ihdr->color_type & 4;

    /* Calculate the number of bytes per pixel. */
    /* TODO: This doesn't exhaust all possible combinations. */
    if (png->palette) {
        png->bpp = 1;
    } else {
        png->bpp = png->color ? 3 : 1;

        if (png->alpha) {
            png->bpp++;
        }
    }

    /* Calculate the final data length for the raw pixel data. */
    /* TODO: This calculation is very arbitrary. */
    png->data_length = png->width * png->height * 5;

    /* Combine all the IDAT data parts into a common data array. */
    png->comp_data = png_get_combined_data(png);

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
    printf("  Bit depth: %d bits per channel\n", png->ihdr->depth);
    printf("  Bytes per pixel: %d bpp\n", png->bpp);
    printf("  Color type:%s%s%s\n",
        png->palette ? " palette" : "",
        png->color   ? " color" : "",
        png->alpha   ? " alpha" : "");
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

int png_paeth_predictor(int a, int b, int c)
{
    /* Algorithm from RFC 2083 (http://tools.ietf.org/html/rfc2083). */

    int p = a + b - c;
    int pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);

    if (pa <= pb && pa <= pc) {
        return a;
    } else if (pb <= pc) {
        return b;
    } else {
        return c;
    }
}

int png_unfilter_byte(int filter, int current,
    int top, int left, int topleft)
{
    switch (filter) {
        case 0: return current;
        case 1: return current + left;
        case 2: return current + top;
        case 3: return current + (left + top) / 2;
        case 4: return current + png_paeth_predictor(left, top, topleft);
        default: printf("Unknown filter %d\n", filter); return 0;
    }
}

void png_unfilter_data(char *in, char *out, int width, int height, int bpp)
{
    /* Keep track of our position in the input and the output streams. */
    int in_pos = 0, out_pos = 0;

    /* Loop through all the scanlines of the picture. */
    for (int row = 0; row < height; row++) {
        /* Read the filter number used for this scanline. */
        int filter = in[in_pos++];

        /* Loop through all the pixels of the scanline. */
        for (int col = 0; col < width; col++) {
            /* Step through the bytes of the pixel and undo the filter. */
            for (int i = 0; i < bpp; i++) {
                unsigned char top, left, topleft;

                /* Get the values of the corresponding byte in the
                 * neighbouring pixels in the unfiltered image. */
                top = (row > 0 ? out[out_pos - bpp * width] : 0);
                left = (col > 0 ? out[out_pos - bpp] : 0);
                topleft = (row > 0 && col > 0 ?
                    out[out_pos - bpp * (width + 1)] : 0);

                /* Unfilter the byte. */
                out[out_pos++] = png_unfilter_byte(
                    filter, in[in_pos++], top,  left, topleft);
            }
        }
    }
}

void png_get_color_data(png_t *png)
{
    png->data = malloc(png->data_length);

    for (int row = 0; row < png->height; row++) {
        for (int col = 0; col < png->width; col++) {
            unsigned char r, g, b, a, bg;

            int from_offset = (row * png->width + col) * png->bpp;
            int to_offset   = (row * png->width + col) * 4;
            int current_byte = 0;

            r = g = b = png->unfiltered_data[from_offset + current_byte++];

            if (png->color) {
                g = png->unfiltered_data[from_offset + current_byte++];
                b = png->unfiltered_data[from_offset + current_byte++];
            }

            if (png->alpha) {
                a = png->unfiltered_data[from_offset + current_byte++];
                bg = (((row / 10) % 2) ^ ((col / 10) % 2)) ? 255 : 192;

                r = (r * a + bg * (255 - a)) / 255;
                g = (g * a + bg * (255 - a)) / 255;
                b = (b * a + bg * (255 - a)) / 255;
            }

            png->data[to_offset + 0] = b > 255 ? 255 : b;
            png->data[to_offset + 1] = g > 255 ? 255 : g;
            png->data[to_offset + 2] = r > 255 ? 255 : r;
            png->data[to_offset + 3] = 0;
        }
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
    png->unfiltered_data = malloc(png->data_length);
    png_unfilter_data(filtered, png->unfiltered_data,
        png->width, png->height, png->bpp);

    png_get_color_data(png);

    return png->data;
}
