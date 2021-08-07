#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "bitstream.h"
#include "generic_func.h"

struct bitstream {
    FILE *fichier;
	uint64_t end_pos;
	uint8_t last_byte;
	uint8_t bit_pos;
};

// Renvoie la taille d'un fichier
static uint64_t file_size(const char *filename)
{
	struct stat s;
	if (stat(filename, &s) != 0)
		file_error(__func__);

	return s.st_size;
}

extern struct bitstream *bitstream_create(const char *filename)
{
	// Allocation mémoire pour le bitstream
	struct bitstream *new_stream = malloc(sizeof *new_stream);
	if (!new_stream)
		alloc_error(__func__);

	// Ouverture du fichier en mode lecture
    FILE *fichier = fopen(filename, "r");
	if (!fichier)
		file_error(__func__);

	// Initialisation des attributs
	new_stream->fichier = fichier;
	new_stream->end_pos = file_size(filename);
	new_stream->last_byte = fgetc(fichier);
	new_stream->bit_pos = 0;

	return new_stream;
} 

extern void bitstream_close(struct bitstream *stream)
{
    fclose(stream->fichier);
	free(stream);
}

extern bool bitstream_is_empty(struct bitstream *stream)
{
	uint64_t pos = ftell(stream->fichier);

    if (pos == stream->end_pos && stream->bit_pos == 8)
		return true;
	else
		return false;
}

extern uint8_t bitstream_read(struct bitstream *stream, uint8_t nb_bits, uint32_t *dest,
	bool discard_byte_stuffing)
{
	uint32_t res = 0;
	uint8_t nb_bits_lus = 0;

	while (nb_bits_lus < nb_bits) {
		if (bitstream_is_empty(stream))
			break;

		// Lecture de l'octet suivant si l'octet courant est totalement lu + gestion byte stuffing
		if (stream->bit_pos == 8) {
			uint8_t new_byte = fgetc(stream->fichier);
			if (discard_byte_stuffing) {
				if (stream->last_byte == 0xff && new_byte == 0x00)
					stream->last_byte = fgetc(stream->fichier);
				else
					stream->last_byte = new_byte;
			} else {
				stream->last_byte = new_byte;
			}
			stream->bit_pos = 0;
		}
		// Calcul de dest avec mask
		res = res * 2 + ((stream->last_byte >> (7 - stream->bit_pos)) & 1);
		nb_bits_lus++;
		stream->bit_pos++;
	}

	*dest = res;
	return nb_bits_lus;
}

