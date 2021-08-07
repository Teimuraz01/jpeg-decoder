#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "baseline_write.h"
#include "generic_func.h"

struct pixel {
	uint8_t rouge;
	uint8_t vert;
	uint8_t bleu;
};

static char *str_prep(const char *filename, const char *ext)
{
	uint32_t pos = strlen(filename);
	while(filename[--pos] != '.');
	char *output = malloc(pos + 6);
	if (!output)
		alloc_error(__func__);

	for (uint32_t i = 0; i < pos; i++)
		output[i] = filename[i];

	output[pos] = '\0';
	strcat(output, ext);
	return output;
}

void to_pgm(uint8_t **tab, const char *filename, uint16_t im_H, uint16_t im_V)
{
	char *name = str_prep(filename, ".pgm");
	FILE *fichier = fopen(name, "w");
	if (!fichier)
		file_error(__func__);

	free(name);

	uint32_t ceil_H = ceil((float) im_H / BLOCK_SIZE),
	        ceil_V = ceil((float) im_V / BLOCK_SIZE),
	        cpt = 0;
	uint8_t tronc_H = im_H % BLOCK_SIZE,
	        tronc_V = im_V % BLOCK_SIZE;

	char entete[100];
	sprintf(entete, "P5 %u %u 255\n", im_H, im_V);
	fwrite(entete, sizeof(char), strlen(entete), fichier);
	// Pour chaque ligne de blocs
	uint8_t borne_ligne = BLOCK_SIZE,
	        borne_col = BLOCK_SIZE;
	for (uint32_t h = 0; h < ceil_V; h++) {
		if (tronc_V && h == ceil_V - 1)
			borne_ligne = tronc_V;

		uint8_t lignes_lues = 0;
		// Pour chaque ligne dans cette ligne de blocs
		while (lignes_lues < borne_ligne) {
			// Pour chaque bloc
			for (uint32_t l = 0; l < ceil_H; l++) {
				borne_col = BLOCK_SIZE;
				if (tronc_H && l == ceil_H - 1)
					borne_col = tronc_H;

				// Pour chaque colonne du bloc
				for (uint8_t j = 0; j < borne_col; j++) {
					fwrite(&tab[h * ceil_H + l][lignes_lues * BLOCK_SIZE + j],
						sizeof(uint8_t), 1, fichier);
				 	cpt++;
				}
			}
			lignes_lues++;
		}
	}
	fclose(fichier);
}

void to_ppm(struct pixel ***tab, const char *filename, uint16_t im_H, uint16_t im_V,
	struct mcu *unit, struct composante *img_comp)
{
    uint8_t idx_Y = get_mcu_idx_Y(unit),
            Y_H = get_comp_sample_H(img_comp, idx_Y),
	        Y_V = get_comp_sample_V(img_comp, idx_Y);

	char *name = str_prep(filename, ".ppm");
	FILE *fichier = fopen(name, "wb");
	if (!fichier)
		file_error(__func__);

	free(name);

	uint32_t ceil_mcu_H = ceil((float) im_H / (BLOCK_SIZE * Y_H)),
	        ceil_mcu_V = ceil((float) im_V / (BLOCK_SIZE * Y_V)),
	        ceil_H = ceil_mcu_H * Y_H,
	        ceil_V = ceil_mcu_V * Y_V;

	uint8_t tronc_H = im_H % (BLOCK_SIZE * Y_H),
	        tronc_V = im_V %  (BLOCK_SIZE * Y_V),
	        blocs_vires_H = Y_H - (1 + tronc_H / BLOCK_SIZE) ,
	        blocs_vires_V = Y_V - (1 + tronc_V / BLOCK_SIZE) ;

	if (tronc_V == 0) 
		blocs_vires_V = 0;

	if (tronc_H == 0) 
		blocs_vires_H = 0;

	char entete[100];
	sprintf(entete, "P6 %u %u 255\n", im_H, im_V);
	fwrite(entete, sizeof(char), strlen(entete), fichier);

	// Pour chaque ligne de blocs
	uint8_t borne_ligne = BLOCK_SIZE,
	        borne_col = BLOCK_SIZE;
	for (uint32_t h = 0; h < ceil_V - blocs_vires_V; h++) {
		if (tronc_V && h == ceil_V - blocs_vires_V - 1)
			borne_ligne = tronc_V % BLOCK_SIZE;

		// Pour chaque ligne
		for (uint8_t j = 0; j < borne_ligne; j++) {
			// Pour chaque bloc
			for (uint32_t l = 0; l <  ceil_H - blocs_vires_H; l++) {
				borne_col = BLOCK_SIZE;
				if (tronc_H && l == ceil_H - blocs_vires_H - 1 )
					borne_col = tronc_H % BLOCK_SIZE;

				// Pour chaque colonne
				for (uint8_t k = 0; k < borne_col; k++) {
					struct pixel pix = tab[h * ceil_H +  l][j][k];
					fwrite(&pix.rouge, sizeof(uint8_t), 1, fichier);
					fwrite(&pix.vert, sizeof(uint8_t), 1, fichier);
					fwrite(&pix.bleu, sizeof(uint8_t), 1, fichier);
				}
			}
		}
	}
	fclose(fichier);
}

void free_idct(struct mcu *unit, uint8_t **tab)
{
	uint32_t blocs_totaux = get_mcu_nb(unit) * get_mcu_blocs_par_mcu(unit);
	for (uint32_t i = 0; i < blocs_totaux; i++)
		free(tab[i]);

	free(tab);
}

void free_ppm(struct mcu *unit, struct pixel ***tab)
{
	uint32_t blocs_totaux = get_mcu_nb(unit) * get_mcu_nbr_Y(unit);
	for (uint32_t i = 0; i < blocs_totaux; i++) {
		for (uint8_t j = 0; j < BLOCK_SIZE; j++)
			free(tab[i][j]);

		free(tab[i]);
	}
	free(tab);
}


struct pixel ***ycbcr_to_rgb(uint8_t **tab, uint16_t im_H, struct mcu *unit,
	struct composante *img_comp)
{
	uint8_t idx_Y = get_mcu_idx_Y(unit),
			idx_Cb = get_mcu_idx_Cb(unit),
			idx_Cr = get_mcu_idx_Cr(unit), 
			nbr_Y = get_mcu_nbr_Y(unit),
			blocs_mcu = get_mcu_blocs_par_mcu(unit),
	        blocs_pre_Y = 0,
			blocs_pre_Cb = 0, 
			blocs_pre_Cr = 0,
			sum = 0;
			
	for (uint8_t i = 0; i < 3; i++) {
		if (get_comp_type(img_comp, i) == 0)
			blocs_pre_Y = sum;

		if (get_comp_type(img_comp, i) == 1)
			blocs_pre_Cb = sum;

		if (get_comp_type(img_comp, i) == 2)
			blocs_pre_Cr = sum;

		sum += get_mcu_blocs_par_comp(unit, i);
	}
	
	
	uint8_t Y_H = get_comp_sample_H(img_comp, idx_Y),
	        Y_V = get_comp_sample_V(img_comp, idx_Y),
	        Cb_H = get_comp_sample_H(img_comp, idx_Cb),
	        Cb_V = get_comp_sample_V(img_comp, idx_Cb),
	        ratio_Cb_h = Y_H / Cb_H,
	        ratio_Cb_v = Y_V / Cb_V,
	        Cr_H = get_comp_sample_H(img_comp, idx_Cr),
	        Cr_V = get_comp_sample_V(img_comp, idx_Cr),
	        ratio_Cr_h = Y_H / Cr_H,
	        ratio_Cr_v = Y_V / Cr_V,
            tmpY, tmpCb, tmpCr;

	uint32_t ceil_mcu_H = ceil((float) im_H / (BLOCK_SIZE * Y_H)),
            ceil_H = ceil_mcu_H * Y_H ;

    uint32_t blocs_totaux = get_mcu_nb(unit) * nbr_Y;
	struct pixel ***tab_apr_rgb = malloc((blocs_totaux) * sizeof(*tab_apr_rgb));
	if (!tab_apr_rgb)
		alloc_error(__func__);

	for (uint32_t cpt_h = 0, k = 0; k < get_mcu_nb(unit) * blocs_mcu; 
		cpt_h += Y_H, k += blocs_mcu) {
		for (uint8_t v = 0; v < Y_V; v++) {
			for (uint8_t h = 0; h < Y_H; h++) {

				struct pixel **bloc_rgb = malloc(BLOCK_SIZE * sizeof(*bloc_rgb));
				if (!bloc_rgb)
					alloc_error(__func__);
				
                /* 
				 * On calcul les offset et les coordonnnes x et y independement 
                 * pour prendre en compte le cas de sous-echantillonage different
                 */
				uint8_t offset_rgb = v * Y_H + h,
                        offset_Cb = (v / ratio_Cb_v) * Cb_H + (h / ratio_Cb_h) % Cb_H,
				        offset_Cr = (v / ratio_Cr_v) * Cr_H + (h / ratio_Cr_h) % Cr_H;

				for (int8_t y = 0; y < BLOCK_SIZE; y++) {
					struct pixel *ligne_rgb = malloc(BLOCK_SIZE * sizeof(*ligne_rgb));
					if (!ligne_rgb)
						alloc_error(__func__);

					for (int8_t x = 0; x < BLOCK_SIZE; x++) {
						uint32_t Cb_y = y / ratio_Cb_v + BLOCK_SIZE /
											ratio_Cb_v * (v % ratio_Cb_v),
								Cb_x = x / ratio_Cb_h + BLOCK_SIZE /
						        			ratio_Cb_h * (h % ratio_Cb_h),
						        Cr_y = y / ratio_Cr_v + BLOCK_SIZE /
						        			ratio_Cr_v * (v % ratio_Cr_v),
						        Cr_x = x / ratio_Cr_h + BLOCK_SIZE /
						        			ratio_Cr_h * (h % ratio_Cr_h);

						tmpY = tab[k + blocs_pre_Y + offset_rgb][y * BLOCK_SIZE + x];

						tmpCb = tab[k + blocs_pre_Cb + offset_Cb]
							[Cb_y * BLOCK_SIZE + Cb_x];

						tmpCr = tab[k + blocs_pre_Cr + offset_Cr]
							[Cr_y * BLOCK_SIZE + Cr_x];

						ligne_rgb[x].rouge = sature(tmpY - 0.0009267 *
							(tmpCb - 128) + 1.4016868 * (tmpCr - 128));
						ligne_rgb[x].vert = sature(tmpY - 0.3436954 *
							(tmpCb - 128) - 0.7141690 * (tmpCr - 128));
						ligne_rgb[x].bleu = sature(tmpY + 1.7721604 *
							(tmpCb - 128) + 0.0009902 * (tmpCr - 128));
					}
					bloc_rgb[y] = ligne_rgb;
				}
				uint32_t ind = (cpt_h / ceil_H * Y_V + v) * ceil_H + cpt_h % ceil_H + h;
				tab_apr_rgb[ind] = bloc_rgb;
			}
		}
		// On libère les struct mcu déjà transformées en rgb
		libere_mcu(&tab[k], blocs_mcu);
	}
	free(tab);
	return tab_apr_rgb;
}
