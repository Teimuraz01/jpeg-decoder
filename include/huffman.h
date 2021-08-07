#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#include <stdint.h>

#include "bitstream.h"

struct huff_node;
struct huff_table;

/* 
 * Construit une table de Huffman à partir des données de la section DHT de
 * l'en-tête. En cas d'échec lors de la création, la fonction retourne NULL et 
 * nb_byte_read vaut -1.
 */
extern struct huff_table *huffman_load_table(struct bitstream *stream,
    uint16_t *nb_byte_read);

/* 
 * Décode une séquence de bits de données brutes du fichier jpeg en parcourant
 * l'arbre de Huffman et en retournant le symbole correspondant. Arrêt du programme
 * en cas d'échec.
 */
extern int8_t huffman_next_value(struct huff_table *table,
    struct bitstream *stream);

// Libère la table de huffman
extern void huffman_free_table(struct huff_table *table);

#endif
