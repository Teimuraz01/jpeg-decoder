#include <math.h>
#include <stdlib.h>

#include "generic_func.h"

/* 
 * Fonctions utilisées pour l'IDCT rapide
 *
 * Utilisation de l'algorithme de Van Eijdhoven et Sijstermans (Loeffler modifié)
 */

// Enumération pour récupérer les bonnes valeurs de cos et sin dans les blocs rotators
enum number_n {
    UN,
    TROIS,
    SIX
};

// Bloc butterfly de l'algorithme de Van Eijdhoven et Sijstermans
static void butterfly(float *ligne, uint8_t o1, uint8_t o2)
{
    float tmp1 = (ligne[o1] + ligne[o2]) * 0.5;
    float tmp2 = (ligne[o1] - ligne[o2]) * 0.5;
    ligne[o1] = tmp1;
    ligne[o2] = tmp2;
}

// Bloc rotator de l'algorithme de Van Eijdhoven et Sijstermans
static void rotator(float *ligne, uint8_t o1, uint8_t o2, float k,
    float cosinus[3], float sinus[3], enum number_n n)
{
    float tmp1 = ligne[o1] * k * cosinus[n] - ligne[o2] * k * sinus[n];
    float tmp2 = ligne[o2] * k * cosinus[n] + ligne[o1] * k * sinus[n];
    ligne[o1] = tmp1;
    ligne[o2] = tmp2;
}

// Réordonne les valeurs en fonction de l'algorithme de Van Eijdhoven et Sijstermans
static void reordonnement(float *ligne, float *ligne_tmp)
{
    ligne[0] = ligne_tmp[0];
    ligne[1] = ligne_tmp[4];
    ligne[2] = ligne_tmp[2];
    ligne[3] = ligne_tmp[6];
    ligne[4] = ligne_tmp[7];
    ligne[5] = ligne_tmp[3];
    ligne[6] = ligne_tmp[5];
    ligne[7] = ligne_tmp[1];
}

// Première étape de l'algorithme de Van Eijdhoven et Sijstermans
static void stage_1(float *ligne, float cosinus[3], float sinus[3])
{
    butterfly(ligne, 0, 1);
    rotator(ligne, 2, 3, 1 / sqrt(2), cosinus, sinus, SIX);
    ligne[4] *= sqrt(2);
    ligne[7] *= sqrt(2);
    butterfly(ligne, 7, 4);
}

// Deuxième étape de l'algorithme de Van Eijdhoven et Sijstermans
static void stage_2(float *ligne)
{
    butterfly(ligne, 0, 3);
    butterfly(ligne, 1, 2);
    butterfly(ligne, 4, 6);
    butterfly(ligne, 7, 5);
}

// Troisième étape de l'algorithme de Van Eijdhoven et Sijstermans
static void stage_3(float *ligne, float cosinus[3], float sinus[3])
{
    rotator(ligne, 5, 6, 1 / sqrt(2), cosinus, sinus, UN);
    rotator(ligne, 4, 7, 1 / sqrt(2), cosinus, sinus, TROIS);
}

// Quatrième étape de l'algorithme de Van Eijdhoven et Sijstermans
static void stage_4(float *ligne, float inter[BLOCK_SIZE][BLOCK_SIZE],
    uint8_t *res, uint8_t i, uint8_t step)
{
    if (!step) {
        // Réalise une transposition pour faciliter la suite de l'algorithme
        butterfly(ligne, 0, 7);
        inter[0][i] = ligne[0];
        inter[7][i] = ligne[7];

        butterfly(ligne, 1, 6);
        inter[1][i] = ligne[1];
        inter[6][i] = ligne[6];

        butterfly(ligne, 2, 5);
        inter[2][i] = ligne[2];
        inter[5][i] = ligne[5];

        butterfly(ligne, 3, 4);
        inter[3][i] = ligne[3];
        inter[4][i] = ligne[4];
    } else {
        butterfly(ligne, 0, 7);
        res[i + BLOCK_SIZE * 0] = sature(128 + ligne[0]);
        res[i + BLOCK_SIZE * 7] = sature(128 + ligne[7]);

        butterfly(ligne, 1, 6);
        res[i + BLOCK_SIZE * 1] = sature(128 + ligne[1]);
        res[i + BLOCK_SIZE * 6] = sature(128 + ligne[6]);

        butterfly(ligne, 2, 5);
        res[i + BLOCK_SIZE * 2] = sature(128 + ligne[2]);
        res[i + BLOCK_SIZE * 5] = sature(128 + ligne[5]);

        butterfly(ligne, 3, 4);
        res[i + BLOCK_SIZE * 3] = sature(128 + ligne[3]);
        res[i + BLOCK_SIZE * 4] = sature(128 + ligne[4]);
    }
}

// Applique l'algorithme de Van Eijdhoven et Sijstermans
static void algo_loeffler_modifie(float *ligne, 
    float inter[BLOCK_SIZE][BLOCK_SIZE], uint8_t *res,
    float cosinus[3], float sinus[3], uint8_t i, uint8_t step)
{
    stage_1(ligne, cosinus, sinus);
    stage_2(ligne);
    stage_3(ligne, cosinus, sinus);
    stage_4(ligne, inter, res, i, step);
}

uint8_t *idct_fast(int16_t *freq)
{
    // Calculs des valeurs de cosinus et sinus utiles
    float cosinus[3] = {cos(M_PI / 16.0), cos((3 * M_PI) / 16.0),
        cos((6 * M_PI) / 16.0)};
    float sinus[3] = {sin(M_PI / 16.0), sin((3 * M_PI) / 16.0),
        sin((6 * M_PI) / 16.0)};

    float inter[BLOCK_SIZE][BLOCK_SIZE];
    float ligne_tmp[BLOCK_SIZE];
    float ligne[BLOCK_SIZE];
    uint8_t *res = malloc(BLOCK_SQUARE * sizeof(*res));
    if (!res)
        alloc_error(__func__);

    // Algorithme de Van Eijdhoven et Sijstermans sur les lignes
    for (uint8_t i = 0; i < BLOCK_SIZE; i++) {
        // Récupération de la ligne courante
        for (uint8_t j = 0; j < BLOCK_SIZE; j++) {
            ligne_tmp[j] = freq[i * BLOCK_SIZE + j] * sqrt(BLOCK_SIZE);
        }

        // Algorithme de Van Eijdhoven et Sijstermans
        reordonnement(ligne, ligne_tmp);
        algo_loeffler_modifie(ligne, inter, res, cosinus, sinus, i, 0);
    }

    // Algorithme de Van Eijdhoven et Sijstermans sur les colonnes obtenues
    for (uint8_t i = 0; i < BLOCK_SIZE; i++) {
        // Récupération de la colonne courante (ligne de la matrice transposée)
        for (uint8_t j = 0; j < BLOCK_SIZE; j++)
            ligne_tmp[j] = inter[i][j] * sqrt(BLOCK_SIZE);

        // Algorithme de Van Eijdhoven et Sijstermans
        reordonnement(ligne, ligne_tmp);
        algo_loeffler_modifie(ligne, inter, res, cosinus, sinus, i, 1);
    }
    return res;
}

/*
 * Fonctions pour l'IDCT "lente"
 *
 * Utilisation d'un diviser pour régner avec la formule de base
 */

// Fonction C définie pour l'IDCT
static float C(int8_t i)
{
	return (!i) ? 1 / sqrt(2) : 1;
}

// Fonction recursive qui applique la formule du idct sur les indices x et y 
static float calcul_rec(uint16_t *freq, float **tab_cos, int8_t x, int8_t y,
    int8_t i_deb, int8_t i_fin, int8_t j_deb, int8_t j_fin)
{
	if (i_deb == i_fin && j_deb == j_fin) {
		int16_t tmp = freq[j_deb * BLOCK_SIZE + i_deb];
		return C(i_deb) * C(j_fin) * tab_cos[x][i_deb] * tab_cos[y][j_deb] * tmp;
	}
	int8_t milieu_i = (i_deb + i_fin) / 2;
	int8_t milieu_j = (j_deb + j_fin) / 2;
	float bloc1 = calcul_rec(freq, tab_cos, x, y,
        i_deb, milieu_i, j_deb, milieu_j);
	float bloc2 = calcul_rec(freq, tab_cos, x, y,
        milieu_i + 1, i_fin, j_deb, milieu_j);
	float bloc3 = calcul_rec(freq, tab_cos, x, y,
        i_deb, milieu_i, milieu_j + 1, j_fin);
	float bloc4 = calcul_rec(freq, tab_cos, x, y,
        milieu_i + 1, i_fin, milieu_j + 1, j_fin);
	return bloc1 + bloc2 + bloc3 + bloc4;
}

uint8_t *idct_slow(uint16_t *freq, float **tab_cos)
{
    uint8_t *res = malloc(BLOCK_SQUARE * sizeof(*res));
    if (!res)
        alloc_error(__func__);

    for (uint8_t y = 0; y < BLOCK_SIZE; y++) {
        for (uint8_t x = 0; x < BLOCK_SIZE; x++) {
            float tmp = calcul_rec(freq, tab_cos, x, y,
                0, BLOCK_SIZE - 1, 0, BLOCK_SIZE - 1);
            res[y * BLOCK_SIZE + x] = sature(128 + 0.25 * tmp);
        }
    }
    return res;
}

