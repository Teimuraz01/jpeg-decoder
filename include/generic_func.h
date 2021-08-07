#ifndef __GENERIC_FUNC_H__
#define __GENERIC_FUNC_H__

#include <stdint.h>
#include <math.h>

#define BLOCK_SIZE 8
#define BLOCK_SQUARE 64
#define M_PI acos(-1)

// Génére le tableau de cosinus pour l'IDCT originale
extern float **tab_cosinus();

// Libére le tableau de cosinus généré pour l'IDCT originale
extern void libere_tab_cos(float **tab_cosinus);

// Sature une valeur entre 0 et 255
extern uint8_t sature(float val);

// Libére la mémoire allouée à la mcu
extern void libere_mcu(uint8_t **tab, uint8_t nb_blocs);

// Message d'erreur d'allocation mémoire + abandon du programme
extern void alloc_error(const char *fname);

// Message d'erreur d'ouverture de fichier + abandon du programme
extern void file_error(const char *fname);

#endif