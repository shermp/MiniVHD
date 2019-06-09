#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
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