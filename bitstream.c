/*
 * pngview - a standalone PNG viewer
 * Copyright 2013 Vegard Edvardsen
 */

#include "bitstream.h"

unsigned char peek_byte(bitstream_t *bitstream)
{
    return bitstream->data[bitstream->current_byte];
}

unsigned char read_byte(bitstream_t *bitstream)
{
    bitstream->current_bit = 0;
    return bitstream->data[bitstream->current_byte++];
}

unsigned int peek_bit(bitstream_t *bitstream)
{
    return (peek_byte(bitstream) >> bitstream->current_bit) & 1;
}

unsigned int read_bit(bitstream_t *bitstream)
{
    if (bitstream->current_bit < 7) {
        return (peek_byte(bitstream) >> bitstream->current_bit++) & 1;
    } else {
        return (read_byte(bitstream) >> 7) & 1;
    }
}

unsigned int read_bits(bitstream_t *bitstream, int bits)
{
    unsigned int value = 0;
    for (int bit = 0; bit < bits; bit++) {
        value |= read_bit(bitstream) << bit;
    }
    return value;
}

void skip_to_next_byte(bitstream_t *bitstream)
{
    if (bitstream->current_bit > 0) {
        read_byte(bitstream);
    }
}
