#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jpeg_reader.h"
#include "bitstream.h"
#include "huffman.h"
#include "composante.h"
#include "mcu.h"
#include "baseline_decode.h"
#include "baseline_write.h"


int main(int argc, char **argv)
{
	if (argc != 2 && argc != 3) {
		fprintf(stderr, "Usage: %s [--idct-slow] fichier.jpeg\n", argv[0]);
		return 0;
	}

	uint8_t version_idct = 0;
	const char *filename = argv[1];

	if (argc == 3) {
		if(!strcmp(argv[1], "--idct-slow")) {
			version_idct = 1;
			filename = argv[2];
		} else {
			fprintf(stderr, "Usage: %s [--idct-slow] fichier.jpeg\n", argv[0]);
			return 0;
		}
	}

	uint32_t byte;

	struct jpeg_desc *jdesc = jpeg_read(filename);

	uint16_t im_H = jpeg_get_image_size(jdesc, H);
	uint16_t im_V = jpeg_get_image_size(jdesc, V);

	uint8_t nb_comp = jpeg_get_nb_components(jdesc);

	struct composante *img_comp = init_comp(jdesc, nb_comp);

	scan_comp(img_comp, jdesc, nb_comp);

	struct bitstream *stream = jpeg_get_bitstream(jdesc);

	//decompression des donnees dans tab
	struct mcu *MCU = init_mcu(im_H, im_V, img_comp, nb_comp);

	int16_t **tab = dechiffrage(jdesc, MCU, stream, img_comp, nb_comp);

	//Finir lecture
	while( !bitstream_is_empty(stream)){
		bitstream_read(stream, 1, &byte, false);
	}

	//quantification inverse + reordonnance
	quant_inv_zig_zag(jdesc, MCU, tab, img_comp, nb_comp);

	//iDCT
	uint8_t **tab_apr_idct = tab_idct(MCU, tab, version_idct);

	if (nb_comp > 1) {
		struct pixel ***tab_rgb = ycbcr_to_rgb(tab_apr_idct, im_H, MCU, img_comp);
		to_ppm(tab_rgb, filename, im_H, im_V, MCU, img_comp);
		free_ppm(MCU, tab_rgb);
	} else {
		to_pgm(tab_apr_idct, filename, im_H, im_V);
		free_idct(MCU,tab_apr_idct);
	}
	free(img_comp);
	libere_struct_mcu(MCU);

	jpeg_close(jdesc);
	free(jdesc);
	return 0;
}