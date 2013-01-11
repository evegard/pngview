#include <stdio.h>

#include "deflate.h"

#define PEEK_BYTE()     (data[cur_byte])
#define READ_BYTE()     (cur_bit = 7, data[cur_byte++])
#define PEEK_BIT()      ((PEEK_BYTE() >> cur_bit) & 1)
#define READ_BIT()      (cur_bit > 0 ? \
                            ((PEEK_BYTE() >> cur_bit--) & 1) : \
                            (READ_BYTE() & 1))
#define SKIP_TO_BYTE()  (cur_bit < 7 && (cur_bit = 7, cur_byte++))

char *deflate_decompress(char *data, int data_length)
{
    int cur_byte = 0;
    int cur_bit = 7;

    for (int i = 0; i < 5; i++) {
        printf("Byte %d: 0x%02hhx\t\t", i, READ_BYTE());
        printf("(cur_byte is now %d and cur_bit is %d)\n",
            cur_byte, cur_bit);
    }
    printf("\n");

    cur_byte = 0;
    cur_bit = 7;

    for (int i = 0; i < 9; i++) {
        printf("Bit %d: %d\t\t", i, READ_BIT());
        printf("(cur_byte is now %d and cur_bit is %d)\n",
            cur_byte, cur_bit);
    }
    printf("cur_byte is %d and cur_bit is %d\n", cur_byte, cur_bit);
    SKIP_TO_BYTE();
    printf("Skipped to next byte\n");
    printf("cur_byte is %d and cur_bit is %d\n", cur_byte, cur_bit);

    for (int i = 0; i < 16; i++) {
        printf("Bit %d: %d\t\t", i, READ_BIT());
        printf("(cur_byte is now %d and cur_bit is %d)\n",
            cur_byte, cur_bit);
    }
}
