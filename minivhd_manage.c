#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "bswap.h"
#include "minivhd_internal.h"
#include "minivhd.h"

static bool mvhd_file_is_vhd(FILE* f);
static void mvhd_read_footer(MVHDMeta* vhdm);
static void mvhd_read_sparse_header(MVHDMeta* vhdm);

static bool mvhd_file_is_vhd(FILE* f) {
    if (f) {
        uint8_t con_str[8];
        fseeko64(f, -MVHD_FOOTER_SIZE, SEEK_END);
        fread(con_str, sizeof con_str, 1, f);
        return mvhd_is_conectix_str(con_str);
    } else {
        return false;
    }
}

static void mvhd_read_footer(MVHDMeta* vhdm) {
    uint8_t buffer[MVHD_FOOTER_SIZE];
    fseeko64(vhdm->f, -MVHD_FOOTER_SIZE, SEEK_END);
    fread(buffer, sizeof buffer, 1, vhdm->f);
    mvhd_buffer_to_footer(&vhdm->footer, buffer);
}

static void mvhd_read_sparse_header(MVHDMeta* vhdm) {
    uint8_t buffer[MVHD_SPARSE_SIZE];
    fseeko64(vhdm->f, -MVHD_SPARSE_SIZE, SEEK_END);
    fread(buffer, sizeof buffer, 1, vhdm->f);
    mvhd_buffer_to_header(&vhdm->sparse, buffer);
}

MVHDMeta* mvhd_open(const char* path, int* err) {
    MVHDMeta *vhdm = calloc(sizeof *vhdm, 1);
    if (vhdm == NULL) {
        *err = MVHD_ERR_MEM;
        goto end;
    }
    vhdm->f = fopen64(path, "r+");
    if (vhdm->f == NULL) {
        *err = MVHD_ERR_FILE;
        goto cleanup_vhdm;
    }
    if (!mvhd_file_is_vhd(vhdm->f)) {
        *err = MVHD_ERR_NOT_VHD;
        goto cleanup_file;
    }
    mvhd_read_footer(vhdm);
    if (vhdm->footer.disk_type == MVHD_TYPE_DIFF || vhdm->footer.disk_type == MVHD_TYPE_DYNAMIC) {
        mvhd_read_sparse_header(vhdm);
    } else if (vhdm->footer.disk_type != MVHD_TYPE_FIXED) {
        *err = MVHD_ERR_TYPE;
        goto cleanup_file;
    }
    /* If we've reached this point, we are good to go, so skip the cleanup steps */
    goto end;
cleanup_file:
    fclose(vhdm->f);
    vhdm->f = NULL;
cleanup_vhdm:
    free(vhdm);
    vhdm = NULL;
end:
    return vhdm;
}