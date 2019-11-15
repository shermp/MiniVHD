#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "../src/minivhd.h"

int main(int argc, char* argv[]) {
    if (argc != 6) {
        char *help_text = 
            "Incorrect num arguments. Expected args as follows:\n"
            "minivhd_test RAW_SRC, VHD_FIXED, VHD_SPARSE, RAW_DEST_FIXED, RAW_DEST_SPARSE\n";
        printf(help_text);
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