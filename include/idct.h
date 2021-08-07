#ifndef __IDCT_H__
#define __IDCT_H__

#include <stdint.h>

// IDCT rapide avec l'algorithme de Van Eijdhoven et Sijstermans
extern uint8_t *idct_fast(int16_t *freq);

// IDCT originale avec un algorithme diviser pour régner
extern uint8_t *idct_slow(int16_t *freq, float **tab_cos);

#endif