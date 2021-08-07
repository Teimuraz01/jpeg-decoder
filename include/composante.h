#ifndef __composante_H__
#define __composante_H__

#include <stdint.h>

#include "jpeg_reader.h"

struct composante;

// Accesseurs
extern struct composante *get_comp(struct composante *comp, uint8_t index);
uint8_t get_comp_type(struct composante *comp, uint8_t index);
uint8_t get_comp_id(struct composante *comp, uint8_t index);
uint8_t get_comp_sample_V(struct composante *comp, uint8_t index);
uint8_t get_comp_sample_H(struct composante *comp, uint8_t index);
uint8_t get_comp_q_index(struct composante *comp, uint8_t index);
uint8_t get_comp_hDC_index(struct composante *comp, uint8_t index);
uint8_t get_comp_hAC_index(struct composante *comp, uint8_t index);

// Initialise les indices associés aux struct composantes de l'image
extern struct composante *init_comp(struct jpeg_desc *jdesc, uint8_t nb_comp);

/* 
 * Associe les struct composantes dans leur ordre d'apparition à leur index
 * et aux tables de huffman associées.
 */
extern void scan_comp(struct composante *img_comp, struct jpeg_desc *jdesc,
    uint8_t nb_comp);

#endif