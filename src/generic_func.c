#include <stdlib.h>
#include <stdio.h>

#include "generic_func.h"

// Renvoi le tableau tab[i][j] = cos((2*i+1)*j*M_PI/16) 
float **tab_cosinus()
{
	float **tab = malloc(BLOCK_SIZE * sizeof(float *));
	if (!tab)
		alloc_error(__func__);

	float tmp = M_PI / (BLOCK_SIZE * 2);
	for (uint8_t y = 0; y < BLOCK_SIZE; y++){
		float *ligne = malloc(BLOCK_SIZE * sizeof(float));
		if (!ligne)
			alloc_error(__func__);

		ligne[0] = 1;
		for (uint8_t x = 1; x < BLOCK_SIZE; x++)
			ligne[x] = cos((2 * y + 1) * x * tmp);

		tab[y] = ligne;
	}
	return tab;
}

void libere_tab_cos(float **tab_cosinus)
{
	for (uint8_t y = 0; y < BLOCK_SIZE; y++)
		free(tab_cosinus[y]);

	free(tab_cosinus);
}

uint8_t sature(float val)
{
	if (val < 0)
		return 0;

	if (val > 255)
		return 255;	

	return round(val);
}

void libere_mcu(uint8_t **tab, uint8_t nb_blocs)
{
	for (uint8_t i = 0; i < nb_blocs; i++)
		free(tab[i]);
}

void alloc_error(const char *fname) {
	fprintf(stderr, "Memory allocation error in function %s"
		" : aborting program.\n", fname);
	exit(-1);
}

void file_error(const char *fname) {
	fprintf(stderr, "File open error in function %s"
		" : aborting program.\n", fname);
	exit(-1);
}
