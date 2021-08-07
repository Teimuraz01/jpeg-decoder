#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jpeg_reader.h"
#include "huffman.h"
#include "generic_func.h"

enum comp_attributes {
	ID,
	H_Sampl,
	V_Sampl,
	Q_Idx,
};

struct jpeg_desc {
	const char *filename;
	uint16_t img_H;
	uint16_t img_V;
	uint8_t nb_comp;
	uint8_t **comp_data;
	uint8_t **scan_data;
	struct huff_table ***huff_data;
	uint8_t **q_tables;
	struct bitstream *data_start;
};

// Message d'erreur en cas de mauvaise lecture dans le flux
static void read_error_msg(void)
{
	fprintf(stderr, "[Jpeg_reader] Erreur de lecture dans l'en-tête.\n");
	exit(-1);
}

// Message d'erreur en cas de format de fichier incorrect
static void format_error_msg(const char *msg)
{
	fprintf(stderr, "[Jpeg_reader] Erreur: format de fichier incorrect. %s\n", msg);
	exit(-1);
}

// Message d'erreur en cas de mauvais marqueur
static void marker_error_msg(uint32_t byte)
{
	fprintf(stderr, "[Jpeg_reader] Erreur: marqueur 0xff attendu, octet %x lu.\n", byte >> 8);
	exit(-1);
}

// Fonction de lecture d'une section APPx.
static void jpeg_read_appx(struct bitstream *stream)
{
	uint8_t nb_bits;
	uint32_t byte;
	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	if (byte >> 8 != 0xff)
		marker_error_msg(byte);

	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	const char *jfif = "JFIF";
	char read_str[5];
	for (uint8_t i = 0; i < 5; i++) {
		nb_bits = bitstream_read(stream, 8, &byte, false);
		if (nb_bits != 8)
			read_error_msg();
		read_str[i] = byte;
	}
	if (strcmp(jfif, read_str))
		format_error_msg("Pas un fichier JFIF.");

	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	if (byte != 0x101)
		format_error_msg("JFIF 1.1 requis.");

	nb_bits = bitstream_read(stream, 32, &byte, false);
	if (nb_bits != 32)
		read_error_msg();

	nb_bits = bitstream_read(stream, 24, &byte, false);
	if (nb_bits != 24)
		read_error_msg();
}

/* 
 * Fonction de lecture d'une section DQT. Pour chaque table présente dans la 
 * section, identifie l'indice de la table, alloue la mémoire pour la table 
 * si elle n'existe pas et recopie ses valeurs lues dans le flux.
 */
static void jpeg_read_dqt(struct jpeg_desc *jpeg, struct bitstream *stream)
{
	uint8_t nb_bits;
	uint32_t byte, section_len, index;
	nb_bits = bitstream_read(stream, 16, &section_len, false);
	if (nb_bits != 16)
		read_error_msg();

	section_len -= 2;
	while (section_len > 0) {
		// Indice de la table (NB: 4 premiers bits donnent la précision; 0 pour 8 bits)
		nb_bits = bitstream_read(stream, 8, &index, false);
		if (nb_bits != 8)
			read_error_msg();

		section_len--;
		//Possible réécriture d'une table existante
		if (!jpeg->q_tables[index]) {
			jpeg->q_tables[index] = malloc(BLOCK_SQUARE * sizeof(*(jpeg->q_tables[index])));
			if (!jpeg->q_tables[index])
				alloc_error(__func__);
		}

		for (uint8_t i = 0; i < 64; i++) {
			nb_bits = bitstream_read(stream, 8, &byte, false);
			if (nb_bits != 8)
				read_error_msg();
			jpeg->q_tables[index][i] = byte;

		}
		section_len -= 64;
	}
}

// Fonction de lecture d'une section SOFx.
static void jpeg_read_sofx(struct jpeg_desc *jpeg, struct bitstream *stream)
{
	uint8_t nb_bits;
	uint32_t byte;
	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	nb_bits = bitstream_read(stream, 8, &byte, false);
	if (nb_bits != 8)
		read_error_msg();

	if (byte != 8)
		format_error_msg("Précision en bits par composante erronée pour le mode BASELINE.");

	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	jpeg->img_V = byte;
	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	jpeg->img_H = byte;
	nb_bits = bitstream_read(stream, 8, &byte, false);
	if (nb_bits != 8)
		read_error_msg();

	jpeg->nb_comp = byte;
	jpeg->comp_data = malloc(jpeg->nb_comp * sizeof(*(jpeg->comp_data)));
	if (!jpeg->comp_data)
		alloc_error(__func__);

	// Stocker les attributs de chaque composante
	for (uint8_t i = 0; i < jpeg->nb_comp; i++) {
		jpeg->comp_data[i] = malloc(4 * sizeof(*(jpeg->comp_data[i])));
		if (!jpeg->comp_data[i])
			alloc_error(__func__);

		nb_bits = bitstream_read(stream, 8, &byte, false);
		if (nb_bits != 8)
			read_error_msg();

		jpeg->comp_data[i][ID] = byte;
		nb_bits = bitstream_read(stream, 4, &byte, false);
		if (nb_bits != 4)
			read_error_msg();

		jpeg->comp_data[i][H_Sampl] = byte;
		nb_bits = bitstream_read(stream, 4, &byte, false);
		if (nb_bits != 4)
			read_error_msg();

		jpeg->comp_data[i][V_Sampl] = byte;
		nb_bits = bitstream_read(stream, 8, &byte, false);
		if (nb_bits != 8)
			read_error_msg();

		jpeg->comp_data[i][Q_Idx] = byte;
	}
}

// Fonction de lecture d'une section DHT.
void jpeg_read_dht(struct jpeg_desc *jpeg, struct bitstream *stream)
{
	uint8_t nb_bits;
	uint16_t table_len;
	uint32_t byte, section_len;
	nb_bits = bitstream_read(stream, 16, &section_len, false);
	if (nb_bits != 16)
		read_error_msg();

	section_len -= 2;
	while (section_len > 0) {
		nb_bits = bitstream_read(stream, 8, &byte, false);
		section_len--;
		if (nb_bits != 8)
			read_error_msg();

		jpeg->huff_data[byte >> 4][0xf & byte] = huffman_load_table(stream, &table_len);
		if(!jpeg->huff_data[byte >> 4][0xf & byte]) {
			fprintf(stderr, "[Huffman] Echec de la création d'un arbre de Huffman.\n");
			exit(-1);
		}
		section_len -= table_len;
	}
}

// Fonction de lecture d'une section SOS.
void jpeg_read_sos(struct jpeg_desc *jpeg, struct bitstream *stream)
{
	uint8_t nb_bits;
	uint32_t byte, nb_comp;
	nb_bits = bitstream_read(stream, 8, &nb_comp, false);
	if (nb_bits != 8)
		read_error_msg();

	if (nb_comp != jpeg->nb_comp) {
		fprintf(stderr, "[Jpeg_reader] Erreur: corruption de l'en-tête.\n");
		exit(-1);
	}
	jpeg->scan_data = malloc(nb_comp * sizeof(*(jpeg->scan_data)));
	if (!jpeg->scan_data)
		alloc_error(__func__);

	for (uint8_t i = 0; i < nb_comp; i++) {
		jpeg->scan_data[i] = malloc(3 * sizeof(*(jpeg->scan_data[i])));
		if (!jpeg->scan_data[i])
			alloc_error(__func__);

		nb_bits = bitstream_read(stream, 8, &byte, false);
		if (nb_bits != 8)
			read_error_msg();

		jpeg->scan_data[i][ID] = byte;

		nb_bits = bitstream_read(stream, 8, &byte, false);
		if (nb_bits != 8)
			read_error_msg();

		jpeg->scan_data[i][1] = byte >> 4;
		jpeg->scan_data[i][2] = 0xf & byte;
	}
	nb_bits = bitstream_read(stream, 8, &byte, false);
	if (nb_bits != 8)
		read_error_msg();

	if (byte)
		format_error_msg("Sélection spectrale erronée pour le mode BASELINE.");

	nb_bits = bitstream_read(stream, 8, &byte, false);
	if (nb_bits != 8)
		read_error_msg();

	if (byte != 63)
		format_error_msg("Sélection spectrale erronée pour le mode BASELINE.");

	nb_bits = bitstream_read(stream, 8, &byte, false);
	if (nb_bits != 8)
		read_error_msg();

	if (byte)
		format_error_msg("Approximation successive erronée pour le mode BASELINE.");
}

/* 
 * Fonction de parsing de l'en-tête d'un fichier JPEG au format APP0 JFIF. 
 * Parcours de toutes les sections de l'en-tête, stockage des infos dans la
 * structure jpeg_desc, et stockage du bitstream positionné au début des 
 * données compressées.
 */
struct jpeg_desc *jpeg_read(const char *filename)
{
	struct jpeg_desc *jpeg = malloc(sizeof(*jpeg));
	if (!jpeg)
		alloc_error(__func__);

	jpeg->filename = filename;
	struct bitstream *stream = bitstream_create(filename);

	uint8_t nb_bits;
	uint32_t byte, section_len;
	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	if (byte != 0xffd8)
		format_error_msg("Pas un fichier JPEG.");

	// Section APPx
	jpeg_read_appx(stream);

	// Section COM optionnelle
	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	if (byte == 0xfffe) {
		nb_bits = bitstream_read(stream, 16, &section_len, false);
		if (nb_bits != 16)
			read_error_msg();

		section_len -= 2;
		while (section_len > 0) {
			nb_bits = bitstream_read(stream, 8, &byte, false);
			if (nb_bits != 8)
				read_error_msg();

			section_len -= 1;
		}
	}
	// Section DQT
	if (byte != 0xffdb) {
		nb_bits = bitstream_read(stream, 16, &byte, false);
		if (nb_bits != 16)
			read_error_msg();

		if (byte >> 8 != 0xff)
			marker_error_msg(byte);
	}
	jpeg->q_tables = calloc(4, sizeof(*(jpeg->q_tables)));
	if (!jpeg->q_tables)
		alloc_error(__func__);

	while ((0xff & byte) == 0xdb) {
		jpeg_read_dqt(jpeg, stream);
		nb_bits = bitstream_read(stream, 16, &byte, false);
		if (nb_bits != 16)
			read_error_msg();

		if (byte >> 8 != 0xff)
			marker_error_msg(byte);
	}
	// Section SOFx: 0xc0 baseline
	if ((0xff & byte) != 0xc0)
		format_error_msg("Ce décodeur ne fonctionne qu'avec l'encodage BASELINE.");

	jpeg_read_sofx(jpeg, stream);

	// Section DHT
	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	if (byte >> 8 != 0xff)
		marker_error_msg(byte);

	jpeg->huff_data = calloc(2, sizeof(*(jpeg->huff_data)));
	if (!jpeg->huff_data)
		alloc_error(__func__);

	for (uint8_t i = 0; i < 2; i++) {
		jpeg->huff_data[i] = calloc(4, sizeof(*(jpeg->huff_data[i])));
		if (!jpeg->huff_data[i])
			alloc_error(__func__);
	}
	while ((0xff & byte) == 0xc4) {
		jpeg_read_dht(jpeg, stream);
		nb_bits = bitstream_read(stream, 16, &byte, false);
		if (nb_bits != 16)
			read_error_msg();

		if (byte >> 8 != 0xff)
			marker_error_msg(byte);
	}
	// Section SOS
	jpeg->scan_data = NULL;
	if (byte >> 8 != 0xff)
		marker_error_msg(byte);

	nb_bits = bitstream_read(stream, 16, &byte, false);
	if (nb_bits != 16)
		read_error_msg();

	jpeg_read_sos(jpeg, stream);
	jpeg->data_start = stream;

	return jpeg;
}

void jpeg_close(struct jpeg_desc *jpeg)
{
	for (uint8_t i = 0; i < jpeg->nb_comp; i++)
		free(jpeg->comp_data[i]);
	free(jpeg->comp_data);

	for (uint8_t i = 0; i < jpeg->nb_comp; i++)
		free(jpeg->scan_data[i]);
	free(jpeg->scan_data);

	for (uint8_t i = 0; i < 4; i++)
		free(jpeg->q_tables[i]);
	free(jpeg->q_tables);

	for (uint8_t i = 0; i < 2; i++) {
		for (uint8_t j = 0; j < 4; j++) {
			if (jpeg->huff_data[i][j])
				huffman_free_table(jpeg->huff_data[i][j]);
		}
		free(jpeg->huff_data[i]);
	}
	free(jpeg->huff_data);
	bitstream_close(jpeg->data_start);
}

const char *jpeg_get_filename(const struct jpeg_desc *jpeg)
{
	return jpeg->filename;
}

struct bitstream *jpeg_get_bitstream(const struct jpeg_desc *jpeg)
{
	return jpeg->data_start;
}

uint8_t jpeg_get_nb_quantization_tables(const struct jpeg_desc *jpeg)
{
	uint8_t count = 0;
	for (uint8_t i = 0; i < 4; i++) {
		if (jpeg->q_tables[i])
			count++;
	}
	return count;
}

uint8_t *jpeg_get_quantization_table(const struct jpeg_desc *jpeg, uint8_t index)
{
	return jpeg->q_tables[index];
}

uint8_t jpeg_get_nb_huffman_tables(const struct jpeg_desc *jpeg,
	enum sample_type acdc)
{
	uint8_t count = 0;
	for (uint8_t j = 0; j < 4; j++) {
		if (jpeg->huff_data[acdc][j])
			count++;
	}
	return count;
}

struct huff_table *jpeg_get_huffman_table(const struct jpeg_desc *jpeg,
    enum sample_type acdc, uint8_t index)
{
	return jpeg->huff_data[acdc][index];
}

uint16_t jpeg_get_image_size(struct jpeg_desc *jpeg, enum direction dir)
{
	if (dir == H)
		return jpeg->img_H;

	return jpeg->img_V;
}

uint8_t jpeg_get_nb_components(const struct jpeg_desc *jpeg)
{
	return jpeg->nb_comp;
}

uint8_t jpeg_get_frame_component_id(const struct jpeg_desc *jpeg,
    uint8_t frame_comp_index)
{
	return jpeg->comp_data[frame_comp_index][ID];
}

uint8_t jpeg_get_frame_component_sampling_factor(const struct jpeg_desc *jpeg,
    enum direction dir, uint8_t frame_comp_index)
{
	return jpeg->comp_data[frame_comp_index][dir+1];
}

uint8_t jpeg_get_frame_component_quant_index(const struct jpeg_desc *jpeg,
    uint8_t frame_comp_index)
{
	return jpeg->comp_data[frame_comp_index][Q_Idx];
}

uint8_t jpeg_get_scan_component_id(const struct jpeg_desc *jpeg,
    uint8_t scan_comp_index)
{
	return jpeg->scan_data[scan_comp_index][ID];
}

uint8_t jpeg_get_scan_component_huffman_index(const struct jpeg_desc *jpeg,
    enum sample_type acdc, uint8_t scan_comp_index)
{
	return jpeg->scan_data[scan_comp_index][acdc + 1];
}
