#ifndef HUFFMAN_H
#define HUFFMAN_H

typedef struct htree {
    int symbol;
    struct htree *left, *right;
} htree_t;

htree_t *huffman_create_tree(int count, int *symbols, int *lengths);
void huffman_print_tree(htree_t *root, int indentation);
int huffman_get_symbol(htree_t **node_ptr, int bit);

#endif
