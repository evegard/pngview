#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "bitstream.h"

typedef struct htree {
    int symbol;
    struct htree *left, *right;
} htree_t;

htree_t *huffman_create_tree(int count, int *lengths);
void huffman_print_tree(htree_t *root, int indentation);
void huffman_free_tree(htree_t *root);
htree_t *huffman_get_symbol(htree_t *root, bitstream_t* bitstream);

#endif
