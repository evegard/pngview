#ifndef ZLIB_H
#define ZLIB_H

typedef struct zlib {
    int method;
    int info;
    int flags;

    int window_size;

    char *data;
    int data_length;
} zlib_t;

zlib_t *zlib_read(char *data, int data_length);
void zlib_print_information(zlib_t *zlib);

#endif
