#include <stdlib.h>
#include "../src/minivhd.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("One argument, the full path to a VHD file is expected.\n");
        return 1;
    }
    char* path = argv[1];
    int err;
    MVHDMeta* vhdm = mvhd_open(path, &err);
}