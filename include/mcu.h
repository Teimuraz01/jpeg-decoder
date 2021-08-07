#ifndef __MCU_H__
#define __MCU_H__

#include <stdint.h>

#include "composante.h"

struct mcu;

// Initialise les propriétés des struct mcu
extern struct mcu *init_mcu(uint16_t im_H, uint16_t im_V, struct composante *img_comp,
    uint8_t nb_comp);

// Libere le memoire utilisé par la struct mcu
extern void libere_struct_mcu(struct mcu *unit);

// Accesseurs
extern uint32_t get_mcu_nb(struct mcu *unit);
extern uint8_t get_mcu_blocs_par_comp(struct mcu *unit, uint8_t index);
extern uint8_t get_mcu_blocs_par_mcu(struct mcu *unit);
extern uint8_t get_mcu_idx_Y(struct mcu *unit);
extern uint8_t get_mcu_idx_Cb(struct mcu *unit);
extern uint8_t get_mcu_idx_Cr(struct mcu *unit);
extern uint8_t get_mcu_nbr_Y(struct mcu *unit);

#endif