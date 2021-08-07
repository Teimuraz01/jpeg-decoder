#include <stdio.h>
#include <stdlib.h>

#include "composante.h"
#include "generic_func.h"

struct composante
{
	//comp Y, Cb ou Cr
	uint8_t comp_type;
	//id de la struct composante
	uint8_t comp_id;
	//facteurs sous-échantillonage
	uint8_t comp_sample_V;
	uint8_t comp_sample_H;
	//indice de la table de quant associée à la struct composante
	uint8_t comp_q_index;
	//indice des tables de huff
	uint8_t comp_hDC_index;
	uint8_t comp_hAC_index;
};

struct composante *get_comp(struct composante *comp, uint8_t index)
{
	return comp + index;
}

uint8_t get_comp_type(struct composante *comp, uint8_t index)
{
	return (comp + index)->comp_type;
}

uint8_t get_comp_id(struct composante *comp, uint8_t index)
{
	return (comp + index)->comp_id;
}

uint8_t get_comp_sample_V(struct composante *comp, uint8_t index)
{
	return (comp + index)->comp_sample_V;
}

uint8_t get_comp_sample_H(struct composante *comp, uint8_t index)
{
	return (comp + index)->comp_sample_H;
}

uint8_t get_comp_q_index(struct composante *comp, uint8_t index)
{
	return (comp + index)->comp_q_index;
}

uint8_t get_comp_hDC_index(struct composante *comp, uint8_t index)
{
	return (comp + index)->comp_hDC_index;
}

uint8_t get_comp_hAC_index(struct composante *comp, uint8_t index)
{
	return (comp + index)->comp_hAC_index;
}

struct composante *init_comp(struct jpeg_desc *jdesc, uint8_t nb_comp)
{
	struct composante *img_comp = malloc(nb_comp * sizeof(*img_comp));
	if (!img_comp)
		alloc_error(__func__);

	for (uint8_t i = 0; i < nb_comp; i++) {
		img_comp[i].comp_type = i;
		img_comp[i].comp_id = jpeg_get_frame_component_id(jdesc, i);
		img_comp[i].comp_sample_V = 
			jpeg_get_frame_component_sampling_factor(jdesc, V, i);
		img_comp[i].comp_sample_H = 
			jpeg_get_frame_component_sampling_factor(jdesc, H, i);
		img_comp[i].comp_q_index = 
			jpeg_get_frame_component_quant_index(jdesc, i);
	}
	return img_comp;
}

void scan_comp(struct composante *img_comp, struct jpeg_desc *jdesc, uint8_t nb_comp)
{
	uint8_t scan_comp, hDC, hAC;
	struct composante temp;
	for (uint8_t i = 0; i < nb_comp; i++) {
		scan_comp = jpeg_get_scan_component_id(jdesc, i);
		hDC = jpeg_get_scan_component_huffman_index(jdesc, DC, i);
		hAC = jpeg_get_scan_component_huffman_index(jdesc, AC, i);
		for (uint8_t j = i + 1; j < nb_comp; j++) {
			if (img_comp[j].comp_id == scan_comp) {
				temp = img_comp[i];
				img_comp[i] = img_comp[j];
				img_comp[j] = temp;
				break;
			}
		}
		img_comp[i].comp_hDC_index = hDC;
		img_comp[i].comp_hAC_index = hAC;
	}
}
