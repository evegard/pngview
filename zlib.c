#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "zlib.h"
#include "deflate.h"

zlib_t *zlib_read(char *data, int data_length)
{
    /* We require at least 2 bytes for the header and 4 bytes for the
     * Adler-32 checksum. */
    if (data_length < 6) {
        return 0;
    }

    zlib_t *zlib = calloc(1, sizeof(zlib_t));
    zlib->method = data[0] & 0xf;
    zlib->info = (data[0] & 0xf0) >> 4;
    zlib->flags = data[1];

    if (zlib->method == 8) {
        zlib->window_size = pow(2, zlib->info + 8);
    }

    if (zlib->flags & 0x20) {
        /* Preset dictionary is not allowed. */
        free(zlib);
        return 0;
    }

    zlib->data = &data[2];
    zlib->data_length = data_length - 6;

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

char *zlib_decompress(zlib_t *zlib)
{
    return deflate_decompress(zlib->data, zlib->data_length);
}
