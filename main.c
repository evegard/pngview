/*
 * pngview - a standalone PNG viewer
 * Copyright 2013 Vegard Edvardsen
 */

#include <stdio.h>
#include <stdlib.h>

#include "png.h"
#include "gui.h"

int main(int argc, char **argv)
{
    png_t *png = png_read(argv[1]);

    if (!png) {
        printf("Invalid png.\n");
        exit(1);
    }

    png_print_information(png);
    gui_display_image(png);

    exit(0);
}
