/*
 * MiniVHD	Minimalist VHD implementation in C.
 *		MiniVHD is a minimalist implementation of read/write/creation
 *		of VHD files. It is designed to read and write to VHD files
 *		at a sector level. It does not enable file access, or provide
 *		mounting options. Those features are left to more advanced
 *		libraries and/or the operating system.
 *
 *		This file is part of the MiniVHD Project.
 *
 *		Simple test program for the MiniVHD library.
 *
 * Version:	@(#)tester.c	1.0.1	2021/03/15
 *
 * Author:	Sherman Perry, <shermperry@gmail.com>
 *
 *		Copyright 2019-2021 Sherman Perry.
 *
 *		MIT License
 *
 *		Permission is hereby granted, free of  charge, to any person
 *		obtaining a copy of this software  and associated documenta-
 *		tion files (the "Software"), to deal in the Software without
 *		restriction, including without limitation the rights to use,
 *		copy, modify, merge, publish, distribute, sublicense, and/or
 *		sell copies of  the Software, and  to permit persons to whom
 *		the Software is furnished to do so, subject to the following
 *		conditions:
 *
 *		The above  copyright notice and this permission notice shall
 *		be included in  all copies or  substantial  portions of  the
 *		Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING  BUT NOT LIMITED TO THE  WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN  NO EVENT  SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER  IN AN ACTION OF  CONTRACT, TORT OR  OTHERWISE, ARISING
 * FROM, OUT OF  O R IN  CONNECTION WITH THE  SOFTWARE OR  THE USE  OR  OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <minivhd.h>


int main(int argc, char* argv[]) {
    if (argc != 6) {
        char *help_text = 
            "Incorrect num arguments. Expected args as follows:\n"
            "minivhd_test RAW_SRC, VHD_FIXED, VHD_SPARSE, RAW_DEST_FIXED, RAW_DEST_SPARSE\n";
        printf("%s\n", help_text);
        return 1;
    }
    const char *raw_src_path = argv[1];
    const char *vhd_fixed_path = argv[2];
    const char *vhd_sparse_path = argv[3];
    const char *raw_dest_fixed_path = argv[4];
    const char *raw_dest_sparse_path = argv[5];
    int err;
    /* Conversion tests. This just happens to test VHD creation, 
     * and the read/write functionality all in one go :) */
    time_t start, end;
    printf("Converting raw imaged to fixed VHD\n");
    start = time(0);
    MVHDMeta *vhdm = mvhd_convert_to_vhd_fixed(raw_src_path, vhd_fixed_path, &err);
    if (vhdm == NULL) {
        printf("%s\n", mvhd_strerr(err));
        return EXIT_FAILURE;
    }
    end = time(0);
    printf("Raw image converted to fixed VHD in %f seconds\n", difftime(end, start));
    mvhd_close(vhdm);

    printf("Converting raw imaged to sparse VHD\n");
    start = time(0);
    vhdm = mvhd_convert_to_vhd_sparse(raw_src_path, vhd_sparse_path, &err);
    if (vhdm == NULL) {
        printf("%s\n", mvhd_strerr(err));
        return EXIT_FAILURE;
    }
    end = time(0);
    printf("Raw image converted to fixed VHD in %f seconds\n", difftime(end, start));
    mvhd_close(vhdm);

    printf("Converting fixed VHD to raw image\n");
    start = time(0);
    FILE *raw = mvhd_convert_to_raw(vhd_fixed_path, raw_dest_fixed_path, &err);
    if (raw == NULL) {
        printf("%s\n", mvhd_strerr(err));
        return EXIT_FAILURE;
    }
    fclose(raw);
    end = time(0);
    printf("Fixed VHD converted to raw image in %f seconds\n", difftime(end, start));

    printf("Converting sparse VHD to raw image\n");
    start = time(0);
    raw = mvhd_convert_to_raw(vhd_sparse_path, raw_dest_sparse_path, &err);
    if (raw == NULL) {
        printf("%s\n", mvhd_strerr(err));
        return EXIT_FAILURE;
    }
    fclose(raw);
    end = time(0);
    printf("Sparse VHD converted to raw image in %f seconds\n", difftime(end, start));
    return EXIT_SUCCESS;
}
