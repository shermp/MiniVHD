#ifndef MINIVHD_UTIL_H
#define MINIVHD_UTIL_H

#include <stdint.h>
#include <stdio.h>
#include "minivhd_internal.h"
#include "minivhd.h"
#define MVHD_START_TS 946684800

bool mvhd_is_conectix_str(const void* buffer);
void mvhd_generate_uuid(uint8_t *uuid);
uint32_t vhd_calc_timestamp(void);
time_t vhd_get_created_time(MVHDMeta *vhdm);
FILE* mvhd_fopen(const char* path, const char* mode, int* err);
void mvhd_set_encoding_err(int encoding_retval, int* err);
uint64_t mvhd_calc_size_bytes(MVHDGeom *geom);
uint32_t mvhd_calc_size_sectors(MVHDGeom *geom);
uint32_t mvhd_gen_footer_checksum(MVHDFooter* footer);
uint32_t mvhd_gen_sparse_checksum(MVHDSparseHeader* header);
#endif