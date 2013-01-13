#ifndef BITSTREAM_H
#define BITSTREAM_H

typedef struct bitstream {
    char *data;
    int current_byte;
    int current_bit;
} bitstream_t;

unsigned char peek_byte(bitstream_t *bitstream);
unsigned char read_byte(bitstream_t *bitstream);
unsigned int peek_bit(bitstream_t *bitstream);
unsigned int read_bit(bitstream_t *bitstream);
unsigned int read_bits(bitstream_t *bitstream, int bits);
void skip_to_next_byte(bitstream_t *bitstream);

#endif
