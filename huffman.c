#include <stdio.h>
#include <stdlib.h>

#include "huffman.h"

#define MAX(a, b)   ((a) > (b) ? (a) : (b))

/* Algorithm from RFC 1951 (http://tools.ietf.org/html/rfc1951#page-7). */
htree_t *huffman_create_tree(int count, int *symbols, int *lengths)
{
    /* Find max(lengths). */
    int max_length = 0;
    for (int i = 0; i < count; i++) {
        max_length = MAX(max_length, lengths[i]);
    }

    /* Create bl_count and next_code arrays. */
    int *bl_count = calloc(max_length + 1, sizeof(int));
    int *next_code = calloc(max_length + 1, sizeof(int));

    /* Step 1 in RFC: Count the number of codes for each code length. */

    bl_count[0] = 0;
    for (int i = 0; i < count; i++) {
        bl_count[lengths[i]]++;
    }

    /* Step 2 in RFC: Find the value of the smallest code for each code
     * length. */
     
    int code = 0;
    for (int bits = 1; bits <= max_length; bits++) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    /* Step 3 in RFC: Assign codes to all symbols, consecutively among
     * symbols with the same code length. */

    /* Create a code array for the symbols in the alphabet. */
    int *symbol_codes = calloc(count, sizeof(int));

    /* Loop through all the symbols and create codes. */
    for (int n = 0; n < count; n++) {
        int len = lengths[n];
        if (len != 0) {
            symbol_codes[n] = next_code[len]++;
        }
    }

    /* Build a tree structure from the array of symbol codes. */

    /* Allocate the root node of the tree. */
    htree_t *root = calloc(1, sizeof(htree_t));
    root->symbol = -1;

    /* Loop through all the symbols in the array. */
    for (int n = 0; n < count; n++) {
        /* Check the symbol's code length and skip if it's zero. */
        int len = lengths[n];
        if (len == 0) {
            continue;
        }

        /* Loop through all the bits of the symbol code from left to
         * right and traverse the tree. Allocate nodes as needed. */
        int symbol_code = symbol_codes[n];
        htree_t *cur_node = root;

        for (int bit_num = len - 1; bit_num >= 0; bit_num--) {
            int bit = (symbol_code >> bit_num) & 1;

            /* Maintain a pointer to the pointer referencing the subtree
             * we want to be in. */
            htree_t **node_ptr = bit == 0 ?
                &cur_node->left : &cur_node->right;

            /* Check if we need to allocate a node. */
            if (*node_ptr == 0) {
                /* Allocate a new node and store the pointer in the
                 * spot where it should be. */
                *node_ptr = calloc(1, sizeof(htree_t));
                (*node_ptr)->symbol = -1;
            }

            cur_node = *node_ptr;
        }

        /* We've reached the leaf node, so store the symbol here. */
        cur_node->symbol = symbols[n];
    }

    /* Free the temporary structures that were allocated. */
    free(symbol_codes);
    free(bl_count);
    free(next_code);

    return root;
}

void huffman_print_tree(htree_t *root, int indentation)
{
    if (!root) {
        for (int i = 0; i < indentation; i++) printf(" ");
        printf("NULL\n");
    } else if (!root->left && !root->right) {
        for (int i = 0; i < indentation; i++) printf(" ");
        printf("Symbol %d\n", root->symbol);
    } else {
        for (int i = 0; i < indentation; i++) printf(" ");
        printf("0:\n");
        huffman_print_tree(root->left, indentation + 2);
        for (int i = 0; i < indentation; i++) printf(" ");
        printf("1:\n");
        huffman_print_tree(root->right, indentation + 2);
    }
}

void huffman_free_tree(htree_t *root)
{
    if (root->left) {
        huffman_free_tree(root->left);
    }
    if (root->right) {
        huffman_free_tree(root->right);
    }
    free(root);
}

int huffman_get_symbol(htree_t **node_ptr, int bit)
{
    if (bit == 0) {
        *node_ptr = (*node_ptr)->left;
    } else {
        *node_ptr = (*node_ptr)->right;
    }

    return ((*node_ptr)->left == 0 && (*node_ptr)->right == 0);
}
