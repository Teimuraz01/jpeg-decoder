#ifndef __BASELINE_WRITE_H__
#define __BASELINE_WRITE_H__

#include <stdint.h>

#include "composante.h"
#include "mcu.h"

struct pixel;

// Ecrit le fichier pgm pour les images en niveau de gris
extern void to_pgm(uint8_t **tab, const char *filename, uint16_t im_H,
    uint16_t im_V);

// Convertit le tableau de pixels ycbcr en tableau de pixels rgb (eventuellement avec upsampling)
extern struct pixel ***ycbcr_to_rgb(uint8_t **tab, uint16_t im_H, struct mcu *unit,
    struct composante *img_comp);

// Ecrit le fichier ppm pour les images couleurs
extern void to_ppm(struct pixel ***tab, const char *filename, uint16_t im_H,
    uint16_t im_V, struct mcu *unit, struct composante *img_comp);

// Libère le tableau idct
extern void free_idct(struct mcu *unit, uint8_t **tab);

// Libère le tableau pour les images couleurs
extern void free_ppm(struct mcu *unit, struct pixel ***tab);

#endif