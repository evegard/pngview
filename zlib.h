/*
 * pngview - a standalone PNG viewer
 * Copyright 2013 Vegard Edvardsen
 */

#ifndef ZLIB_H
#define ZLIB_H

typedef struct zlib {
    int method;
    int info;
    int flags;

    int window_size;

    char *comp_data;
    int comp_data_length;

    char *data;
    int data_length;
} zlib_t;

zlib_t *zlib_read(char *comp_data, int comp_data_length, int data_length);
void zlib_print_information(zlib_t *zlib);
char *zlib_get_data(zlib_t *zlib);

#endif
