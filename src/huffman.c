#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "huffman.h"
#include "generic_func.h"

struct huff_node {
	int8_t val;
	uint32_t left_child;
	uint32_t right_child;
};

struct huff_table {
	struct huff_node *huff_tree;
	uint32_t len_table;
};

/* Construction de l'arbre de Huffman par un parcours en largeur avec feuilles
à gauche à chaque niveau.
*/
static struct huff_node *huffman_build_tree(struct bitstream *stream, uint32_t *final_codes,
	uint8_t array_len, uint32_t *len_table, uint16_t *nb_byte_read)
{
	struct huff_node *tree = malloc(*len_table * sizeof(*tree));
	if (!tree) 
		return NULL;

	tree[0].val = -1;
	tree[0].left_child = 1;
	tree[0].right_child = 2;

	uint8_t nb_bits = 0;
	uint16_t current_nodes, proc_nodes, current_lvl = 0, next_nodes = 2;
	uint32_t symbol = 0, counter = 1;
	// Parcourir le tableau de codes
	for (uint8_t i = 0; i < array_len; i++) {
		current_nodes = next_nodes;
		current_lvl = proc_nodes = next_nodes = 0;
		// Tant qu'on a pas traité tous les noeuds au niveau courant
		while (proc_nodes < current_nodes) {
			// S'il y a encore des feuilles (=symboles) à rajouter
			if (current_lvl < final_codes[i]) {
				nb_bits = bitstream_read(stream, 8, &symbol, false);
				if (nb_bits != 8)
					return NULL;

				*nb_byte_read += 1;
				tree[counter].val = symbol;
				current_lvl++;
			}
			// Sinon il n'y a que des noeuds internes
			else {
				tree[counter].val = -1;
				tree[counter].left_child = counter + next_nodes + 
					(current_nodes - proc_nodes);
				next_nodes++;
				tree[counter].right_child = counter + next_nodes + 
					(current_nodes - proc_nodes);
				next_nodes++;
			}
			proc_nodes++;
			counter++;
		}
	}
	free(final_codes); 
	final_codes = NULL;
	*len_table = counter;

	return realloc(tree, counter * sizeof(*tree));
}

struct huff_table *huffman_load_table(struct bitstream *stream,
	uint16_t *nb_byte_read)
{
	*nb_byte_read = 0;
	uint8_t nb_bits = 0;
	uint32_t *ord_codes = malloc(16 * sizeof(*ord_codes));
	if (!ord_codes) {
		*nb_byte_read = -1;
		return NULL;
	}
	// Stocker le nombre de mots de code de longueur 1...16
	for (uint8_t i = 0; i < 16; i++) {
		nb_bits = bitstream_read(stream, 8, ord_codes + i, false);
		*nb_byte_read += 1;
		if(nb_bits != 8) {
			*nb_byte_read = -1;
			return NULL;
		}
	}
	// Eliminer la partie finale nulle du tableau si elle existe
	uint8_t j = 16;
	while (!ord_codes[--j]);
	uint32_t *final_codes = realloc(ord_codes, ++j * sizeof(*final_codes));
	if (!final_codes) {
		*nb_byte_read = -1;
		return NULL;
	}
	ord_codes = NULL;
	// Construction de l'arbre
	struct huff_table *huff = malloc(sizeof(*huff));
	if (!huff) {
		*nb_byte_read = -1;
		return NULL;
	}
	huff->len_table = pow(2, j + 1) - 1;
	huff->huff_tree = huffman_build_tree(stream, final_codes, j, &(huff->len_table),
		nb_byte_read);
	if (!huff->huff_tree) {
		*nb_byte_read = -1;
		return NULL;
	}
	return huff;
}

int8_t huffman_next_value(struct huff_table *table, struct bitstream *stream)
{
	uint32_t path = 0, dir;
	uint8_t nb_bits;

	while (1) {
		nb_bits = bitstream_read(stream, 1, &dir, true);
		if (nb_bits != 1) {
			fprintf(stderr, "[Huffman] Erreur de lecture dans le flux.\n");
			exit(-1);
		}
		if (!dir)
			path = table->huff_tree[path].left_child;
		else
			path = table->huff_tree[path].right_child;

		if (path >= table->len_table) {
			fprintf(stderr, "[Huffman] Erreur d'encodage Huffman.\n");
			exit(-1);
		}
		if (table->huff_tree[path].val != -1)
			return table->huff_tree[path].val;
	}
}

void huffman_free_table(struct huff_table *table)
{
	free(table->huff_tree);
	free(table);
}
