#ifndef MINIVHD_STRUCT_RW_H
#define minivhd_struct_rw

#include "minivhd_internal.h"

void mvhd_buffer_to_footer(MVHDFooter* footer, uint8_t* buffer);
void mvhd_buffer_to_header(MVHDSparseHeader* header, uint8_t* buffer);
void mvhd_footer_to_buffer(MVHDFooter* footer, uint8_t* buffer);
void mvhd_header_to_buffer(MVHDSparseHeader* header, uint8_t* buffer);

#endif