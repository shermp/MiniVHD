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
    MVHD_ERR_MEM = -1,
    MVHD_ERR_FILE,
    MVHD_ERR_NOT_VHD,
    MVHD_ERR_TYPE
} MVHDError;

typedef struct MVHDMeta {
    FILE* f;
    char* filename;
    MVHDFooter footer;
    MVHDSparseHeader sparse;
    MVHDMeta* parent;
} MVHDMeta;

MVHDMeta* mvhd_open(const char* path, int* err);
MVHDMeta* mvhd_create_fixed(const char* path, int cyl, int heads, int spt, int* err);
MVHDMeta* mvhd_create_sparse(const char* path, int cyl, int heads, int spt, int* err);
MVHDMeta* mvhd_create_diff(const char* path, const char* par_path, int cyl, int heads, int spt, int* err);
void mvhd_close(MVHDMeta* vhdm);
void mvhd_calculate_geometry(int size_mb, int* new_size, int* cyl, int* heads, int* spt);
int mvhd_read_sectors(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff);
int mvhd_write_sectors(MVHDMeta* vhdm, int offset, int num_sectors, void* in_buff);
int mvhd_format_sectors(MVHDMeta* vhdm, int offset, int num_sectors);
#endif