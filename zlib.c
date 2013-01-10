#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "zlib.h"

zlib_t *zlib_read(char *data, int data_length)
{
    if (data_length < 2) {
        return 0;
    }

    zlib_t *zlib = calloc(1, sizeof(zlib_t));
    zlib->method = data[0] & 0xf;
    zlib->info = (data[0] & 0xf0) >> 4;
    zlib->flags = data[1];

    if (zlib->method == 8) {
        zlib->window_size = pow(2, zlib->info + 8);
    }

    zlib->data = &data[2];
    zlib->data_length = data_length - 2;

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
