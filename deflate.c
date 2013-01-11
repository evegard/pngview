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
        btype <<= 1;
        btype |= READ_BIT();

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
            int symbols[8] = { 1,2,3,4,5,6,7,8 };
            int lengths[8] = { 3,3,3,3,3,2,4,4 };
            huffman_print_tree(huffman_create_tree(8, symbols, lengths), 2);
        }
    }

    return buffer;
}
