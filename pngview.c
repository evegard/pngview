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

    for (int row = 0; row < png->height; row++) {
        for (int col = 0; col < png->width; col++) {
            printf("#%02hhx%02hhx%02hhx ",
                png->data[3 * (row * png->width + col)],
                png->data[3 * (row * png->width + col) + 1],
                png->data[3 * (row * png->width + col) + 2]);
        }
        printf("\n");
    }

    exit(0);
}
