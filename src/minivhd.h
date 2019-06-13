#ifndef MINIVHD_H
#define MINIVHD_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

extern int mvhd_errno;

typedef enum MVHDError {
    MVHD_ERR_MEM = -128,
    MVHD_ERR_FILE,
    MVHD_ERR_NOT_VHD,
    MVHD_ERR_TYPE,
    MVHD_ERR_FOOTER_CHECKSUM,
    MVHD_ERR_SPARSE_CHECKSUM,
    MVHD_ERR_UTF_TRANSCODING_FAILED,
    MVHD_ERR_UTF_SIZE,
    MVHD_ERR_FILE
} MVHDError;

typedef struct MVHDGeom {
    uint16_t cyl;
    uint8_t heads;
    uint8_t spt;
} MVHDGeom;

typedef struct MVHDMeta MVHDMeta;

bool mvhd_file_is_vhd(FILE* f);
MVHDMeta* mvhd_open(const char* path, bool readonly, int* err);
MVHDMeta* mvhd_create_fixed(const char* path, MVHDGeom geom, int* err);
MVHDMeta* mvhd_create_sparse(const char* path, MVHDGeom geom, int* err);
MVHDMeta* mvhd_create_diff(const char* path, const char* par_path, MVHDGeom geom, int* err);
void mvhd_close(MVHDMeta* vhdm);
MVHDGeom mvhd_calculate_geometry(int size_mb, int* new_size);
int mvhd_read_sectors(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff);
int mvhd_write_sectors(MVHDMeta* vhdm, int offset, int num_sectors, void* in_buff);
int mvhd_format_sectors(MVHDMeta* vhdm, int offset, int num_sectors);
#endif