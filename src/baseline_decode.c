#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "baseline_decode.h"
#include "generic_func.h"
#include "idct.h"

// Retourne l'élément à l'indice byte de la magnitude val
static int16_t magnitude_class_index(uint32_t byte, uint8_t val)
{
	int16_t neg_min = - (1 << val) + 1;
	int16_t neg_max = - (1 << (val - 1));
	int16_t res = neg_min + byte;

	if (res > neg_max)
		res += 2 * (- neg_max) - 1;

	return res;
}

// Déchiffre un bloc 8*8
static int16_t *dechiffre_bloc(struct bitstream *stream,
	struct huff_table *table_DC, struct huff_table *table_AC, int16_t predec)
{
	uint32_t byte = 0;
	uint8_t val, tabindex = 0;
	int16_t *bloc = calloc(BLOCK_SQUARE, sizeof(*bloc));
	if (!bloc)
		alloc_error(__func__);

	// On déchiffre le DC en ajoutant le DC précédent
	val = huffman_next_value(table_DC, stream);
	bitstream_read(stream, val, &byte, true);
	bloc[tabindex++] = magnitude_class_index(byte, val) + predec;

	// On déchiffre les AC
	while (tabindex < BLOCK_SQUARE) {
		val = huffman_next_value(table_AC, stream);
		// Si 0x00
		if (!val) 
			break;

        uint8_t coef_nuls = val >> 4;
        if (val == 0xF0) {
        	coef_nuls++;
		}
		tabindex += coef_nuls;
        if (0xf & val) {
            bitstream_read(stream, 0xf & val, &byte, true);
            bloc[tabindex++] = magnitude_class_index(byte, 0xf & val);
        }
	}
	return bloc;
}

int16_t **dechiffrage(struct jpeg_desc *jdesc, struct mcu *unit,
	struct bitstream *stream, struct composante *img_comp, uint8_t nb_comp)
{
	uint8_t blocs_par_mcu = get_mcu_blocs_par_mcu(unit);
	uint32_t blocs_totaux = get_mcu_nb(unit) * blocs_par_mcu;
	int16_t **tab = malloc(blocs_totaux * sizeof(*tab));
	int16_t pred_DC[nb_comp];
	struct huff_table *tab_huff[nb_comp][2];

	// On définit un tableau de correspondance de table de huffman avec les composantes
	for (uint8_t i = 0; i < nb_comp; i++) {
		tab_huff[i][DC] = jpeg_get_huffman_table(jdesc, DC, get_comp_hDC_index(img_comp, i));
		tab_huff[i][AC] = jpeg_get_huffman_table(jdesc, AC, get_comp_hAC_index(img_comp, i));
	}

	for (uint8_t i = 0; i < nb_comp; i++) 
		pred_DC[i] = 0;

	for (uint32_t i = 0; i < blocs_totaux; i += blocs_par_mcu) {
		uint8_t bloc_vu = 0;
		// Pour chaque composante
		for (uint8_t j = 0; j < nb_comp; j++) {
			// Pour chaque bloc d'une composante
			for (uint8_t k = j + bloc_vu; k - (j + bloc_vu) < 
				get_mcu_blocs_par_comp(unit, j); k++) {
				tab[i + k] = dechiffre_bloc(stream, tab_huff[j][DC], tab_huff[j][AC], pred_DC[j]);
				pred_DC[j] = tab[i + k][0];
			}
			bloc_vu += get_mcu_blocs_par_comp(unit, j) - 1;
		}
	}
	return tab;
}

// Retourne l'indice zigzag en fonction des indices i(ligne) et j(colonne) de la matrice 
static uint8_t zig_zag_value(int8_t i, int8_t j)
{
    // Cote haut
	int8_t k;
    if (i + j >= BLOCK_SIZE)
        return BLOCK_SIZE * BLOCK_SIZE - 1 - 
			zig_zag_value(BLOCK_SIZE - 1 - i, BLOCK_SIZE - 1 - j);

    // Cote bas
    k = (i + j) * (i + j + 1) / 2;
	if ((i + j) & 1)
		return k + i;
	else
		return k + j;
}

// Initialisation de la matrice de correspondance des indices de zig-zag 
static void tab_zig(uint8_t tab[BLOCK_SIZE][BLOCK_SIZE])
{
	for (uint8_t y = 0; y < BLOCK_SIZE; y++) {
		for (uint8_t x = 0; x < BLOCK_SIZE; x++)
			tab[x][y] = zig_zag_value(y, x);
	}
}

void quant_inv_zig_zag(struct jpeg_desc *jdesc, struct mcu *unit, int16_t **tab,
	struct composante *img_comp, uint8_t nb_comp)
{
	uint32_t blocs_totaux = get_mcu_nb(unit) * get_mcu_blocs_par_mcu(unit);
	uint8_t *tab_q;
	uint8_t tab_zig_ind[BLOCK_SIZE][BLOCK_SIZE];

	tab_zig(tab_zig_ind);
	// Pour chaque MCU
	for (uint32_t i = 0; i < blocs_totaux; i += get_mcu_blocs_par_mcu(unit)) {
		uint8_t bloc_vu = 0;
		for (uint8_t j = 0; j < nb_comp; j++) {
			tab_q = jpeg_get_quantization_table(jdesc,
				get_comp_q_index(img_comp, j));
			for (uint8_t k = j + bloc_vu; k - (j + bloc_vu) <
				get_mcu_blocs_par_comp(unit, j); k++) {
                
                int16_t *bloc_zig_quant = malloc(BLOCK_SQUARE * sizeof(*bloc_zig_quant));
                if (!bloc_zig_quant)
                    alloc_error(__func__);

				for (int8_t y = 0; y < BLOCK_SIZE; y++) {
			        for (int8_t x = 0; x < BLOCK_SIZE; x++) {
                        uint8_t zig_zag_ind = tab_zig_ind[x][y];
					    bloc_zig_quant[y * BLOCK_SIZE + x] = tab[i + k][zig_zag_ind]
					    	* tab_q[zig_zag_ind];
                    }
                }
                free(tab[i + k]);
                tab[i + k] = bloc_zig_quant;
			}
			bloc_vu += get_mcu_blocs_par_comp(unit, j) - 1;
		}
	}
}

uint8_t **tab_idct(struct mcu *unit, int16_t **tab, uint8_t version)
{
	uint32_t blocs_totaux = get_mcu_nb(unit) * get_mcu_blocs_par_mcu(unit);
	uint8_t **tab_idct = malloc(blocs_totaux * sizeof(*tab_idct));
	if (!tab_idct)
		alloc_error(__func__);

	float **tab_cos = NULL;
	if (version)
		tab_cos = tab_cosinus();

	for (uint32_t b = 0; b < blocs_totaux; b++) {
		// IDCT rapide
		if (!version)
			tab_idct[b] = idct_fast(tab[b]);
		// IDCT "lente"
		else
			tab_idct[b] = idct_slow(tab[b], tab_cos);

		free(tab[b]);
	}
	free(tab);
	if (version)
		libere_tab_cos(tab_cos);

	return tab_idct;
}