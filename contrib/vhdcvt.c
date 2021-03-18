/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Convert between RAW and VHD disk images.
 *
 * Usage:	vhdcvt [-qv] [-o out_file] [-s] image.img
 *		vhdcvt [-qv] [-o out_file] [-r] image.vhd
 *
 * Version:	@(#)vhdcvt.h	1.0.2	2021/03/16
 *
 * Author:	Fred N. van Kempen, <waltje@varcem.com>
 *
 *		Copyright 2021 Fred N. van Kempen.
 *
 *		Redistribution and  use  in source  and binary forms, with
 *		or  without modification, are permitted  provided that the
 *		following conditions are met:
 *
 *		1. Redistributions of  source  code must retain the entire
 *		   above notice, this list of conditions and the following
 *		   disclaimer.
 *
 *		2. Redistributions in binary form must reproduce the above
 *		   copyright  notice,  this list  of  conditions  and  the
 *		   following disclaimer in  the documentation and/or other
 *		   materials provided with the distribution.
 *
 *		3. Neither the  name of the copyright holder nor the names
 *		   of  its  contributors may be used to endorse or promote
 *		   products  derived from  this  software without specific
 *		   prior written permission.
 *
 * THIS SOFTWARE  IS  PROVIDED BY THE  COPYRIGHT  HOLDERS AND CONTRIBUTORS
 * "AS IS" AND  ANY EXPRESS  OR  IMPLIED  WARRANTIES,  INCLUDING, BUT  NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE  ARE  DISCLAIMED. IN  NO  EVENT  SHALL THE COPYRIGHT
 * HOLDER OR  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON  ANY
 * THEORY OF  LIABILITY, WHETHER IN  CONTRACT, STRICT  LIABILITY, OR  TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN ANY  WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <minivhd.h>


#define VERSION	"1.0.2"


static int	opt_q,				// be quiet
		opt_r,				// create raw image
		opt_s,				// create sparse file
		opt_v;				// verbose mode


static void
usage(void)
{
    fprintf(stderr,
	"Usage: vhdcvt [-qv] [-o out_file] [-s] image.img\n");
    fprintf(stderr,
	"       vhdcvt [-qv] [-o out_file] [-r] image.vhd\n");
    fprintf(stderr,
	"\nIf -r is used, conversion from VHD to RAW will be attempted.\n"
	"Otherwise, the (raw) input file will be converted to a VHD\n"
	"image, optionally in SPARSE mode if the -s option is present.\n\n");

    exit(1);
    /*NOTREACHED*/
}


int
main(int argc, char *argv[])
{
    char temp[1024], *sp;
    char *outname, *name;
    MVHDMeta *vhd;
    FILE *raw;
    int c;

    /* Set defaults. */
    opt_q = opt_r = opt_s = opt_v = 0;
    outname = NULL;

    opterr = 0;
    while ((c = getopt(argc, argv, "o:qrsv")) != EOF) switch(c) {
	case 'q':	// be quiet
		opt_s = 1;
		break;

	case 'v':	// verbose mode
		opt_v++;
		break;

	case 'r':	// convert to raw-mode image file
		opt_r ^= 1;
		break;

	case 's':	// create sparse-mode VHD file
		opt_s ^= 1;
		break;

	case 'o':	// specify output filename
		outname = optarg;
		break;

	default:
		usage();
		/*NOTREACHED*/
    }

    /* Say hello unless we have to be quiet. */
    if (! opt_q) {
	printf("VHDcvt - Convert between RAW and VHD, version %s.\n", VERSION);
	printf("Author: Fred N. van Kempen, <waltje@varcem.com>\n");
	printf("Copyright 2021, The VARCem Team.\n\n");

	if (opt_v) {
		printf("Library version is %s (%08lX)\n\n",
			mvhd_version(), (unsigned long)mvhd_version_id());
	}
    }

    /* Sanity checks. */
    if (opt_r && opt_s) {
	fprintf(stderr, "The -o and -r options cannot be combined!\n");
	usage();
    }
    if (outname && ((argc -optind) > 1)) {
	fprintf(stderr, "The -o option cannot be used when multiple files are to be converted!\n");
	usage();
    }

    /* We need at least one argument. */
    if (optind == argc)
	usage();

    /* Now loop over the given files. */
    while (optind != argc) {
	name = argv[optind++];

	/* Generate suitable filename if none given. */
	if (outname == NULL) {
		strncpy(temp, name, sizeof(temp) - 5);
		if ((sp = strchr(temp, '.')) != NULL)
			*sp = '\0';
		if (opt_r)
			strcat(temp, ".img");
		else
			strcat(temp, ".vhd");
		sp = temp;
	} else
		sp = outname;

	/* Set "no error". */
	c = 0;

	if (opt_r) {
		/* Convert a VHD image to a RAW image. */
		if (! opt_q)
			printf("Converting VHD '%s' to RAW image.\n", name);

		raw = mvhd_convert_to_raw(name, sp, &c);
		if (raw != NULL)
			fclose(raw);
	} else {
		/* Convert from raw image to VHD. */
		if (! opt_q)
			printf("Converting RAW '%s' to %sVHD image.\n",
						name, opt_s?"sparse ":"");
		if (opt_s)
			vhd = mvhd_convert_to_vhd_sparse(name, sp, &c);
		else
			vhd = mvhd_convert_to_vhd_fixed(name, sp, &c);
		if (vhd != NULL)
			mvhd_close(vhd);
	}

	if (c != 0) {
		/* Some kind of error occurred. */
		fprintf(stderr, "\nERROR: %s\n", mvhd_strerr(c));
		break;
	}
    }

    return(c);
}
