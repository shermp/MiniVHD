#ifndef MINIVHD_IO_H
#define MINIVHD_IO_H
#include "minivhd.h"

int mvhd_fixed_read(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff);
int mvhd_sparse_read(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff);
int mvhd_diff_read(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff);

int mvhd_fixed_write(MVHDMeta* vhdm, int offset, int num_sectors, void* in_buff);
int mvhd_sparse_diff_write(MVHDMeta* vhdm, int offset, int num_sectors, void* in_buff);
int mvhd_noop_write(MVHDMeta* vhdm, int offset, int num_sectors, void* in_buff);

#endif