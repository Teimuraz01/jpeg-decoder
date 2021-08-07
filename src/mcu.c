#include <stdlib.h>
#include <math.h>

#include "mcu.h"
#include "generic_func.h"

struct mcu {
	uint32_t nb;
	uint8_t *blocs_par_comp;
	uint8_t blocs_par_mcu;
	uint8_t idx_Y;
	uint8_t idx_Cb;
	uint8_t idx_Cr;
	uint8_t nbr_Y;
};

void libere_struct_mcu(struct mcu *unit)
{
	free(unit->blocs_par_comp);
	free(unit);
}

/* 
 * Renvoie le nombre de MCU en fonction de la taille de l'image et du sous-
 * échantillonnage de Y.
 */
static uint32_t calcul_mcu(uint16_t im_H, uint16_t im_V, struct composante *img_comp)
{
	uint16_t par_ver = ceil(im_V / 8.0);
	uint16_t par_hor = ceil(im_H / 8.0);
	par_ver = ceil(par_ver / (float) get_comp_sample_V(img_comp, 0));
	par_hor = ceil(par_hor / (float) get_comp_sample_H(img_comp, 0));
	return par_ver * par_hor;
}

struct mcu *init_mcu(uint16_t im_H, uint16_t im_V, struct composante *img_comp, uint8_t nb_comp)
{
	struct mcu *unit = malloc(sizeof(*unit));
	if (!unit)
		alloc_error(__func__);

	unit->blocs_par_comp = malloc(nb_comp * sizeof(*(unit->blocs_par_comp)));
	if (!unit->blocs_par_comp)
		alloc_error(__func__);

	unit->blocs_par_mcu = 0;

	for (uint8_t i = 0; i < nb_comp; i++) {
		unit->blocs_par_comp[i] = get_comp_sample_H(img_comp, i) * 
			get_comp_sample_V(img_comp, i);
		unit->blocs_par_mcu += unit->blocs_par_comp[i];
		if (get_comp_type(img_comp, i) == 0) {
			unit->nb = calcul_mcu(im_H, im_V, get_comp(img_comp, i));
			unit->idx_Y = i;
			unit->nbr_Y = unit->blocs_par_comp[i];
		}
		if (get_comp_type(img_comp, i) == 1)
			unit->idx_Cb = i;

		if (get_comp_type(img_comp, i) == 2)
			unit->idx_Cr = i;
	}
	return unit;
}

uint32_t get_mcu_nb(struct mcu *unit)
{
	return unit->nb;
}

uint8_t get_mcu_blocs_par_comp(struct mcu *unit, uint8_t index)
{
	return unit->blocs_par_comp[index];
}

uint8_t get_mcu_blocs_par_mcu(struct mcu *unit)
{
	return unit->blocs_par_mcu;
}

uint8_t get_mcu_idx_Y(struct mcu *unit)
{
	return unit->idx_Y;
}

uint8_t get_mcu_idx_Cb(struct mcu *unit)
{
	return unit->idx_Cb;
}

uint8_t get_mcu_idx_Cr(struct mcu *unit)
{
	return unit->idx_Cr;
}

uint8_t get_mcu_nbr_Y(struct mcu *unit)
{
	return unit->nbr_Y;
}
