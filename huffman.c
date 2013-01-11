#include <stdio.h>
#include <stdlib.h>

#include "huffman.h"

#define MAX(a, b)   ((a) > (b) ? (a) : (b))

/* Algorithm from RFC 1951 (http://tools.ietf.org/html/rfc1951#page-7). */
htree_t *huffman_create_tree(int count, int *symbols, int *lengths)
{
    /* Find max(lengths). */
    int max_length;
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
            printf("Code for %d is %x\n", symbols[n], symbol_codes[n]);
        }
    }
}
