#include <stdbool.h>
#include <string.h>
#ifdef HAVE_UUID_H
#include <uuid/uuid.h>
#endif
#include "minivhd_internal.h"
#include "minivhd.h"

const char MVHD_CONECTIX_COOKIE[] = "conectix";
const char MVHD_CREATOR[] = "pcem";
const char MVHD_CREATOR_HOST_OS[] = "Wi2k";
const char MVHD_CXSPARSE_COOKIE[] = "cxsparse";

bool mvhd_is_conectix_str(const void* buffer) {
    if (strncmp(buffer, MVHD_CONECTIX_COOKIE, strlen(MVHD_CONECTIX_COOKIE)) == 0) {
        return true;
    } else {
        return false;
    }
}

MVHDGeom mvhd_calculate_geometry(int size_mb, int* new_size) {
    MVHDGeom chs;
    uint32_t ts = ((uint64_t)size_mb * 1024 * 1024) / MVHD_SECTOR_SIZE;
    uint32_t spt, heads, cyl, cth;
    if (ts > 65535 * 16 * 255) {
        ts = 65535 * 16 * 255;
    }
    if (ts >= 65535 * 16 * 63) {
        ts = 65535 * 16 * 63;
        spt = 255;
        heads = 16;
        cth = ts / spt;
    } else {
        spt = 17;
        cth = ts / spt;
        heads = (cth + 1023) / 1024;
        if (heads < 4) {
            heads = 4;
        }
        if (cth >= (heads * 1024) || heads > 16) {
            spt = 31;
            heads = 16;
            cth = ts / spt;
        }
        if (cth >= (heads * 1024)) {
            spt = 63;
            heads = 16;
            cth = ts / spt;
        }
    }
    cyl = cth / heads;
    chs.heads = heads;
    chs.spt = spt;
    chs.cyl = cyl;

    *new_size = chs.cyl * chs.heads * chs.spt * MVHD_SECTOR_SIZE;
    return chs;
}

/* A UUID is required, but there are no restrictions on how it needs
   to be generated. */
void mvhd_generate_uuid(uint8_t *uuid)
{
#if defined(HAVE_UUID_H)
    uuid_generate(guid);
#else
    int n;
    srand(time(NULL));
    for (n = 0; n < 16; n++) {
        uuid[n] = rand();
    }
    uuid[6] &= 0x0F;
    uuid[6] |= 0x40; /* Type 4 */
    uuid[8] &= 0x3F;
    uuid[8] |= 0x80; /* Variant 1 */
#endif
}