#ifndef MINIVHD_H
#define MINIVHD_H

#include <stdio.h>
#include "minivhd_internal.h"

typedef enum MVHDType {
    MVHD_TYPE_FIXED = 2,
    MVHD_TYPE_DYNAMIC = 3,
    MVHD_TYPE_DIFF = 4
} MVHDType;

typedef enum MVHDError {
    MVHD_ERR_MEM = -128,
    MVHD_ERR_FILE,
    MVHD_ERR_NOT_VHD,
    MVHD_ERR_TYPE,
    MVHD_ERR_FOOTER_CHECKSUM,
    MVHD_ERR_SPARSE_CHECKSUM
} MVHDError;

typedef struct MVHDGeom {
    uint16_t cyl;
    uint8_t heads;
    uint8_t spt;
} MVHDGeom;

typedef struct MVHDMeta {
    FILE* f;
    char* filename;
    struct MVHDMeta* parent;
    MVHDFooter footer;
    MVHDSparseHeader sparse;
    MVHDBlock* block;
    int sect_per_block;
    int bm_sect_count;
    int (*read_sectors)(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff);
    int (*write_sectors)(MVHDMeta* vhdm, int offset, int num_sectors, void* in_buff);
    int (*format_sectors)(MVHDMeta* vhdm, int offset, int num_sectors);
} MVHDMeta;

MVHDMeta* mvhd_open(const char* path, int* err);
MVHDMeta* mvhd_create_fixed(const char* path, MVHDGeom geom, int* err);
MVHDMeta* mvhd_create_sparse(const char* path, MVHDGeom geom, int* err);
MVHDMeta* mvhd_create_diff(const char* path, const char* par_path, MVHDGeom geom, int* err);
void mvhd_close(MVHDMeta* vhdm);
MVHDGeom mvhd_calculate_geometry(int size_mb, int* new_size);
#endif