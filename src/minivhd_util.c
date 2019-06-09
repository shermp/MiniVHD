#include <stdbool.h>
#include <string.h>
#include "minivhd_internal.h"
//#include "minivhd.h"

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