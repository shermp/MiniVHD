#ifndef MINIVHD_UTIL_H
#define MINIVHD_UTIL_H

#include <stdint.h>
#define MVHD_START_TS 946684800

bool mvhd_is_conectix_str(const void* buffer);
void mvhd_generate_uuid(uint8_t *uuid);

#endif