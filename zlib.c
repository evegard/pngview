#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "zlib.h"
#include "deflate.h"

zlib_t *zlib_read(char *comp_data, int comp_data_length, int data_length)
{
    /* We require at least 2 bytes for the header and 4 bytes for the
     * Adler-32 checksum. */
    if (comp_data_length < 6) {
        return 0;
    }

    zlib_t *zlib = calloc(1, sizeof(zlib_t));
    zlib->method = comp_data[0] & 0xf;
    zlib->info = (comp_data[0] & 0xf0) >> 4;
    zlib->flags = comp_data[1];

    if (zlib->method == 8) {
        zlib->window_size = pow(2, zlib->info + 8);
    }

    if (zlib->flags & 0x20) {
        /* Preset dictionary is not allowed. */
        free(zlib);
        return 0;
    }

    zlib->comp_data = &comp_data[2];
    zlib->comp_data_length = comp_data_length - 6;

    zlib->data_length = data_length;

    return zlib;
}

void zlib_print_information(zlib_t *zlib)
{
    printf("zlib header:\n");
    printf("  Compression method: %s\n",
        zlib->method == 8 ? "deflate" : "UNKNOWN");
    printf("  LZ77 window size: %d\n", zlib->window_size);
    printf("  Preset dictionary: %s\n", zlib->flags & 0x20 ? "Yes" : "No");
    printf("  Compression level: %d\n", (zlib->flags & 0xc0) >> 6);
}

char *zlib_get_data(zlib_t *zlib)
{
    if (zlib->data) {
        return zlib->data;
    }

    zlib->data = deflate_decompress(zlib->comp_data,
        zlib->comp_data_length, zlib->data_length);
    return zlib->data;
}
