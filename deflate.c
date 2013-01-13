#include <math.h>
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

#define PRINTABLE(c)    ((c) >= 0x20 && (c) <= 0x7e ? (c) : ('.'))

int *deflate_get_symbol_sequence(int count)
{
    int *sequence = calloc(count, sizeof(int));
    for (int i = 0; i < count; i++) {
        sequence[i] = i;
    }
    return sequence;
}

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

int deflate_get_extra_bits_for_literal(int literal)
{
    if (literal >= 265 && literal <= 284) {
        return (literal - 265 + 4) / 4;
    }
    return 0;
}

int deflate_get_length_for_literal(int literal)
{
    if (literal < 257) {
        return 0;
    }
    if (literal == 285) {
        return 258;
    }
    int length = 3;
    for (int i = 257; i < literal; i++) {
        length += pow(2, deflate_get_extra_bits_for_literal(i));
    }
    return length;
}

int deflate_get_extra_bits_for_dist(int dist)
{
    if (dist < 2) {
        return 0;
    }
    return (dist - 2) / 2;
}

int deflate_get_length_for_dist(int dist)
{
    int length = 1;
    for (int i = 0; i < dist; i++) {
        length += pow(2, deflate_get_extra_bits_for_dist(i));
    }
    return length;
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

        printf("first byte = 0x%02hhx\n", PEEK_BYTE());
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

            htree_t *ht_literals, *ht_distances;

            if (btype == 1) {
                ht_literals = deflate_get_fixed_literals_htree();
                ht_distances = deflate_get_fixed_distances_htree();
            } else {
                int hlit = 0, hdist = 0, hclen = 0;
                for (int i = 0; i < 5; i++) hlit |= READ_BIT() << i;
                for (int i = 0; i < 5; i++) hdist |= READ_BIT() << i;
                for (int i = 0; i < 4; i++) hclen |= READ_BIT() << i;
                printf("  hlit = %d, hdist = %d, hclen = %d\n",
                    hlit, hdist, hclen);
                hlit += 257;
                hdist += 1;
                hclen += 4;
                int cl_order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5,
                    11, 4, 12, 3, 13, 2, 14, 1, 15 };
                int cl_symbols[19] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                    11, 12, 13, 14, 15, 16, 17, 18 };
                int cl_lengths[19] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0 };
                for (int i = 0; i < hclen; i++) {
                    int len = 0;
                    for (int j = 0; j < 3; j++) len |= READ_BIT() << j;
                    printf(
                        "  Code length for code length symbol %d is %d.\n",
                        cl_order[i], len);
                    cl_lengths[cl_order[i]] = len;
                }
                htree_t *ht_cl = huffman_create_tree(19, cl_symbols,
                    cl_lengths);
                huffman_print_tree(ht_cl, 2);

                int tot = hlit + hdist;
                int *lengths = calloc(tot, sizeof(int));
                int cur_len = 0;

                do {
                    htree_t *cur_cl = ht_cl;
                    while (!huffman_get_symbol(&cur_cl, READ_BIT()));
                    printf("  Code length symbol %d. ", cur_cl->symbol);

                    int times = 0;

                    switch (cur_cl->symbol) {
                        case 16:
                            /* Copy previous length 3-6 times. */
                            for (int i=0;i<2;i++) times|=READ_BIT()<<i;
                            times += 3;
                            int previous = lengths[cur_len - 1];
                            printf("Repeating prev. %d for %d times.\n",
                                previous , times);
                            for (int i = 0; i < times; i++) {
                                lengths[cur_len++] = previous;
                            }
                            break;
                        case 17:
                            /* Repeat 0 for 3-10 times. */
                            for (int i=0;i<3;i++) times|=READ_BIT()<<i;
                            times += 3;
                            printf("Repeating 0 for %d times.\n",
                                times);
                            for (int i = 0; i < times; i++) {
                                lengths[cur_len++] = 0;
                            }
                            break;
                        case 18:
                            /* Repeat 0 for 11-138 times. */
                            for (int i=0;i<7;i++) times|=READ_BIT()<<i;
                            times += 11;
                            printf("Repeating 0 for %d times.\n",
                                times);
                            for (int i = 0; i < times; i++) {
                                lengths[cur_len++] = 0;
                            }
                            break;
                        default:
                            /* Emit length. */
                            printf("Length at pos %d is %d.\n",
                                cur_len, cur_cl->symbol);
                            lengths[cur_len++] = cur_cl->symbol;
                            break;
                    }
                } while (cur_len < tot);
                if (cur_len > tot) {
                    printf("Error! Went too far!\n");
                }
                printf("  cur_len is %d, hlit + hdist is %d\n",
                    cur_len, tot);

                /* We don't need the code length Huffman tree anymore. */
                huffman_free_tree(ht_cl);

                int *symbols = deflate_get_symbol_sequence(288);
                int *lit_lengths = &lengths[0];
                int *dist_lengths = &lengths[hlit];

                ht_literals = huffman_create_tree(hlit,symbols,lit_lengths);
                printf("  Literals tree:\n");
                huffman_print_tree(ht_literals, 4);

                ht_distances = huffman_create_tree(
                    hdist,symbols,dist_lengths);
                printf("  Distances tree:\n");
                huffman_print_tree(ht_distances, 4);

                /* Free the data arrays used to create the Huffman
                 * trees. */
                free(lengths);
                free(symbols);
            }

            htree_t *cur_literal, *cur_distance;

            do {
                cur_literal = ht_literals;
                while (!huffman_get_symbol(&cur_literal, READ_BIT()));

                printf("  Literal/length symbol %d (%c)\n",
                    cur_literal->symbol, PRINTABLE(cur_literal->symbol));

                if (cur_literal->symbol < 256) {
                    buffer[pos++] = cur_literal->symbol;
                } else if (cur_literal->symbol > 256) {
                    int extra = deflate_get_extra_bits_for_literal(
                        cur_literal->symbol);

                    int length = 0;
                    for (int i=0;i<extra;i++) length |= READ_BIT() << i;
                    printf("    Read %d extra bits. Length is %d + %d.\n",
                        extra, length, deflate_get_length_for_literal(
                            cur_literal->symbol));

                    length += deflate_get_length_for_literal(
                        cur_literal->symbol);

                    cur_distance = ht_distances;
                    while (!huffman_get_symbol(&cur_distance, READ_BIT()));
                    printf("    Distance symbol %d\n",
                        cur_distance->symbol);

                    extra = deflate_get_extra_bits_for_dist(
                        cur_distance->symbol);

                    int distance = 0;
                    for (int i=0;i<extra;i++) distance |= READ_BIT() << i;
                    printf("    Read %d extra bits. Distance is %d + %d.\n",
                        extra, distance, deflate_get_length_for_dist(
                            cur_distance->symbol));

                    distance += deflate_get_length_for_dist(
                        cur_distance->symbol);

                    for (int i = 0; i < length; i++) {
                        buffer[pos + i] = buffer[pos - distance + i];
                    }
                    pos += length;
                }
            } while (cur_literal->symbol != 256);

            /* Free the Huffman trees. */
            huffman_free_tree(ht_literals);
            huffman_free_tree(ht_distances);
        }
    }

    return buffer;
}
