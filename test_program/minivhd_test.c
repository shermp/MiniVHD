#include <stdlib.h>
#include <stdio.h>
#include "../src/minivhd.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("One argument, the full path to a VHD file is expected.\n");
        return 1;
    }
    char* path = argv[1];
    int err;
    MVHDMeta* vhdm = mvhd_open(path, false, &err);
    mvhd_close(vhdm);
    /* Creation Tests */
    MVHDGeom geom = mvhd_calculate_geometry(512 * 1024 * 1024);
    vhdm = mvhd_create_sparse("C:/StandaloneProg/pcem/Images/minivhd/sparse.vhd", geom, &err);
    if (vhdm == NULL) {
        printf("Failed with exit code: %d", err);
        return 1;
    }
    mvhd_close(vhdm);
    return 0;
}