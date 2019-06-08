#ifndef MINIVHD_H
#define MINIVHD_H

#include <stdio.h>
#include "minivhd_internal.h"
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