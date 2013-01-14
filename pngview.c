#include <stdio.h>
#include <stdlib.h>

#include "png.h"
#include "gui.h"

#define MIN(a, b)   ((a) < (b) ? (a) : (b))

int main(int argc, char **argv)
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

    gui_display_image(png);

    exit(0);
}
