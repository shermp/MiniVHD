#ifndef MINIVHD_UTIL_H
#define MINIVHD_UTIL_H

#include <stdint.h>

bool mvhd_is_conectix_str(const void* buffer);
void mvhd_generate_uuid(uint8_t *uuid);

#endif