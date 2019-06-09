#include <stdbool.h>
#include <string.h>
#include "minivhd_internal.h"
//#include "minivhd.h"

bool mvhd_is_conectix_str(const void* buffer) {
    if (strncmp(buffer, MVHD_CONECTIX_COOKIE, strlen(MVHD_CONECTIX_COOKIE)) == 0) {
        return true;
    } else {
        return false;
    }
}