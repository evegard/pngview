#include <stdint.h>
#include <stdio.h>

#include "deflate.h"

#define PEEK_BYTE()     ((uint8_t)data[cur_byte])
#define READ_BYTE()     (cur_bit = 7, (uint8_t)data[cur_byte++])
#define PEEK_BIT()      ((PEEK_BYTE() >> cur_bit) & 1)
#define READ_BIT()      (cur_bit > 0 ? \
                            ((PEEK_BYTE() >> cur_bit--) & 1) : \
                            (READ_BYTE() & 1))
#define SKIP_TO_BYTE()  (cur_bit < 7 && (cur_bit = 7, cur_byte++))

char *deflate_decompress(char *data, int data_length)
{
    int cur_byte = 0;
    int cur_bit = 7;

    int bfinal = 0, btype;

    while (bfinal != 1) {
        bfinal = READ_BIT();
        btype = READ_BIT();
        btype <<= 1;
        btype |= READ_BIT();

        printf("bfinal = %d, btype = %d\n", bfinal, btype);

        if (btype == 0) {
            printf("Uncompressed data\n");
            SKIP_TO_BYTE();
            uint16_t len = READ_BYTE();
            len |= READ_BYTE() << 8;
            uint16_t blen = READ_BYTE();
            blen |= READ_BYTE() << 8;

            if ((uint16_t)len != (uint16_t)~blen) {
                printf("deflate error: len and blen mismatch\n");
            }

            printf("len = %hu\n", len);
        }

        break;
    }
}
