#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdint.h>
#include <stdbool.h>

struct bitstream;

// Crée le bitstream
extern struct bitstream *bitstream_create(const char *filename);

// Ferme le bitstream
extern void bitstream_close(struct bitstream *stream);

// Vérifie si le bitstream est vide
extern bool bitstream_is_empty(struct bitstream *stream);

// Lit le bitstream et renvoie le nombre de bits lus
extern uint8_t bitstream_read(struct bitstream *stream,
    uint8_t nb_bits, uint32_t *dest, bool discard_byte_stuffing);

#endif