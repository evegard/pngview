#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deflate.h"
#include "huffman.h"

#define PEEK_BYTE()     ((uint8_t)data[cur_byte])
#define READ_BYTE()     (cur_bit = 0, (uint8_t)data[cur_byte++])
#define PEEK_BIT()      ((PEEK_BYTE() >> cur_bit) & 1)
#define READ_BIT()      (cur_bit < 7 ? \
                            ((PEEK_BYTE() >> cur_bit++) & 1) : \
                            ((READ_BYTE() >>     7    ) & 1))
#define SKIP_TO_BYTE()  (cur_bit > 0 && (cur_bit = 0, cur_byte++))

htree_t *deflate_get_fixed_literals_htree()
{
    int symbols[288], lengths[288];
    for (int i = 0; i < 288; i++) {
        symbols[i] = i;
        if (i < 144) {
            lengths[i] = 8;
        } else if (i < 256) {
            lengths[i] = 9;
        } else if (i < 280) {
            lengths[i] = 7;
        } else {
            lengths[i] = 8;
        }
    }

   return huffman_create_tree(288, symbols, lengths);
}

htree_t *deflate_get_fixed_distances_htree()
{
    int symbols[32], lengths[32];
    for (int i = 0; i < 32; i++) {
        symbols[i] = i;
        lengths[i] = 5;
    }

   return huffman_create_tree(32, symbols, lengths);
}

char *deflate_decompress(char *data, int data_length, int max_size)
{
    char *buffer = malloc(max_size * sizeof(char));
    int pos = 0;

    int cur_byte = 0;
    int cur_bit = 0;

    int bfinal = 0, btype;

    while (bfinal != 1) {
        bfinal = READ_BIT();
        btype = READ_BIT();
        btype |= READ_BIT() << 1;

        printf("bfinal = %d, btype = %d\n", bfinal, btype);

        if (btype == 3) {
            printf("deflate error: invalid block type\n");
        } else if (btype == 0) {
            printf("  Uncompressed data\n");
            SKIP_TO_BYTE();
            uint16_t len = READ_BYTE();
            len |= READ_BYTE() << 8;
            uint16_t blen = READ_BYTE();
            blen |= READ_BYTE() << 8;

            if ((uint16_t)len != (uint16_t)~blen) {
                printf("deflate error: len and blen mismatch\n");
            }

            printf("  len = %hu\n", len);
            memcpy(&buffer[pos], &data[cur_byte], len);

            pos += len;
            cur_byte += len;
        } else {
            printf("  %s Huffman\n", btype == 1 ? "Fixed" : "Dynamic");

            htree_t *ht_literals = deflate_get_fixed_literals_htree();
            htree_t *ht_distances = deflate_get_fixed_distances_htree();

            htree_t *cur_literal, *cur_distance;

            do {
                cur_literal = ht_literals;
                int res;
                do {
                    int bit = READ_BIT();
                    printf("%d", bit);
                    res = huffman_get_symbol(&cur_literal, bit);
                } while (res == 0);

                printf("\nLiteral/length symbol %d\n", cur_literal->symbol);

                //cur_distance = ht_distances;
                //while (!huffman_get_symbol(&cur_distance, READ_BIT()));
                //printf("Distance symbol %d\n", cur_distance->symbol);
            } while (cur_literal->symbol < 256);
        }
    }

    return buffer;
}
