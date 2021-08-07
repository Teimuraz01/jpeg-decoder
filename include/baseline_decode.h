#ifndef __BASELINE_DECODE_H__
#define __BASELINE_DECODE_H__

#include <stdint.h>

#include "composante.h"
#include "mcu.h"
#include "jpeg_reader.h"
#include "bitstream.h"
#include "huffman.h"

// Déchiffre toutes les données brutes
extern int16_t **dechiffrage(struct jpeg_desc *jdesc, struct mcu *unit,
    struct bitstream *stream, struct composante *img_comp, uint8_t nb_comp);

/* 
 * Applique la quantification inverse sur les les blocs des struct composantes 
 * en reordonnant le zig-zag
 */
extern void quant_inv_zig_zag(struct jpeg_desc *jdesc, struct mcu *unit, int16_t **tab,
    struct composante *img_comp, uint8_t nb_comp);

/* 
 * Renvoi tous les blocs obtenus après l'application de idct tout en
 * liberant le tableau en argument
 */
extern uint8_t **tab_idct(struct mcu *unit, int16_t **tab, uint8_t version);

#endif
