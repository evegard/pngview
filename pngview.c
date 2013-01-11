#include <stdio.h>
#include <stdlib.h>

#include "png.h"

void main(int argc, char **argv)
{
    FILE *file = fopen(argv[1], "r");

    png_t *png = png_read(file);
    fclose(file);

    if (!png) {
        printf("Invalid png.\n");
        exit(1);
    }

    png_print_information(png);
    png_get_data(png);

    for (int i = 0; i < 100; i++) {
        printf("%02hhx  ", png->data[i]);
    }
    printf("\n");

    exit(0);
}
