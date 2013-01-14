#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "deflate.h"
#include "huffman.h"

void deflate_get_fixed_htrees(htree_t **htree_lit, htree_t **htree_dist)
{
    int lengths[288];

    for (int i = 0; i < 288; i++) {
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

   *htree_lit = huffman_create_tree(288, lengths);

   for (int i = 0; i < 32; i++) {
       lengths[i] = 5;
   }

   *htree_dist = huffman_create_tree(32, lengths);
}

int deflate_extra_bits_for_literal(int literal)
{
    if (literal >= 265 && literal <= 284) {
        return (literal - 265 + 4) / 4;
    }
    return 0;
}

int deflate_length_for_literal(int literal)
{
    if (literal < 257) {
        return 0;
    }
    if (literal == 285) {
        return 258;
    }
    int length = 3;
    for (int i = 257; i < literal; i++) {
        length += pow(2, deflate_extra_bits_for_literal(i));
    }
    return length;
}

int deflate_extra_bits_for_distance(int distance)
{
    if (distance < 2) {
        return 0;
    }
    return (distance - 2) / 2;
}

int deflate_length_for_distance(int distance)
{
    int length = 1;
    for (int i = 0; i < distance; i++) {
        length += pow(2, deflate_extra_bits_for_distance(i));
    }
    return length;
}

int deflate_process_uncompressed(char *buf, int pos, bitstream_t *bitstream)
{
    skip_to_next_byte(bitstream);

    uint16_t len = read_byte(bitstream);
    len |= read_byte(bitstream) << 8;

    uint16_t blen = read_byte(bitstream);
    blen |= read_byte(bitstream) << 8;

    if ((uint16_t)len != (uint16_t)~blen) {
        printf("Error: len and blen mismatch\n");
    }

    memcpy(&buf[pos], &bitstream->data[bitstream->current_byte], len);

    pos += len;
    bitstream->current_byte += len;

    return pos;
}

void deflate_parse_htrees(bitstream_t *bitstream,
    htree_t **htree_lit, htree_t **htree_dist)
{
    int hlit = read_bits(bitstream, 5) + 257;
    int hdist = read_bits(bitstream, 5) + 1;
    int hclen = read_bits(bitstream, 4) + 4;

    int cl_order[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
    int cl_lengths[19] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    for (int i = 0; i < hclen; i++) {
        cl_lengths[cl_order[i]] = read_bits(bitstream, 3);
    }

    htree_t *ht_cl = huffman_create_tree(19, cl_lengths);

    int tot = hlit + hdist;
    int *lengths = calloc(tot, sizeof(int));
    int cur_len = 0;

    while (cur_len < tot) {
        htree_t *cur_cl = huffman_get_symbol(ht_cl, bitstream);
        int symbol = 0, repeats = 1;

        if (cur_cl->symbol == 16) {
            /* Symbol 16: Copy previous length 3-6 times. */
            symbol = lengths[cur_len - 1];
            repeats = read_bits(bitstream, 2) + 3;
        } else if (cur_cl->symbol == 17) {
            /* Symbol 17: Repeat 0 for 3-10 times. */
            repeats = read_bits(bitstream, 3) + 3;
        } else if (cur_cl->symbol == 18) {
            /* Symbol 18: Repeat 0 for 11-138 times. */
            repeats = read_bits(bitstream, 7) + 11;
        } else {
            /* Otherwise: Emit length. */
            symbol = cur_cl->symbol;
        }

        for (int i = 0; i < repeats; i++) {
            lengths[cur_len++] = symbol;
        }
    }

    if (cur_len > tot) {
        printf("Error: Code tree lengths defined too far\n");
    }

    /* We don't need the code length Huffman tree anymore. */
    huffman_free_tree(ht_cl);

    int *lit_lengths = &lengths[0];
    int *dist_lengths = &lengths[hlit];

    *htree_lit = huffman_create_tree(hlit, lit_lengths);
    *htree_dist = huffman_create_tree(hdist, dist_lengths);

    /* Free the data arrays used to create the Huffman trees. */
    free(lengths);
}

int deflate_process_huffman(char *buf, int pos, bitstream_t *bitstream,
    htree_t *htree_lit, htree_t *htree_dist)
{
    htree_t *literal, *distance;

    do {
        literal = huffman_get_symbol(htree_lit, bitstream);

        if (literal->symbol < 256) {
            buf[pos++] = literal->symbol;
        } else if (literal->symbol > 256) {
            int extra = deflate_extra_bits_for_literal(literal->symbol);
            int length = read_bits(bitstream, extra);
            length += deflate_length_for_literal(literal->symbol);

            distance = huffman_get_symbol(htree_dist, bitstream);

            extra = deflate_extra_bits_for_distance(distance->symbol);
            int dist = read_bits(bitstream, extra);
            dist += deflate_length_for_distance(distance->symbol);

            for (int i = 0; i < length; i++) {
                buf[pos + i] = buf[pos - dist + i];
            }
            pos += length;
        }
    } while (literal->symbol != 256);

    return pos;
}

char *deflate_decompress(char *data, int data_length, int max_size)
{
    char *buf = malloc(max_size * sizeof(char));
    int pos = 0;

    bitstream_t *bitstream = calloc(1, sizeof(bitstream_t));
    bitstream->data = data;

    int final, type;

    do {
        final = read_bit(bitstream);
        type = read_bits(bitstream, 2);

        if (type == 0) {
            pos = deflate_process_uncompressed(buf, pos, bitstream);
        } else if (type == 1 || type == 2) {
            htree_t *htree_lit, *htree_dist;

            if (type == 1) {
                deflate_get_fixed_htrees(&htree_lit, &htree_dist);
            } else {
                deflate_parse_htrees(bitstream, &htree_lit, &htree_dist);
            }

            pos = deflate_process_huffman(buf, pos, bitstream,
                htree_lit, htree_dist);

            /* Free the Huffman trees. */
            huffman_free_tree(htree_lit);
            huffman_free_tree(htree_dist);
        } else {
            printf("Error: Invalid deflate block type\n");
        }
    } while (!final);

    return buf;
}
