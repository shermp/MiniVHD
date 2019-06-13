/**
 * \file
 * \brief Utility functions
 */

#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UUID_H
#include <uuid/uuid.h>
#endif
#include "libxml2_encoding.h"
#include "minivhd_internal.h"
#include "minivhd_util.h"

const char MVHD_CONECTIX_COOKIE[] = "conectix";
const char MVHD_CREATOR[] = "pcem";
const char MVHD_CREATOR_HOST_OS[] = "Wi2k";
const char MVHD_CXSPARSE_COOKIE[] = "cxsparse";

/**
 * \brief Check if provided buffer begins with the string "conectix"
 * 
 * \param [in] buffer The buffer to compare. Must be at least 8 bytes in length
 * 
 * \return true if the buffer begins with "conectix"
 * \return false if the buffer does not begin with "conectix"
 */
bool mvhd_is_conectix_str(const void* buffer) {
    if (strncmp(buffer, MVHD_CONECTIX_COOKIE, strlen(MVHD_CONECTIX_COOKIE)) == 0) {
        return true;
    } else {
        return false;
    }
}

/**
 * \brief Generate a raw 16 byte UUID
 * 
 * \param [out] uuid A 16 byte buffer in which the generated UUID will be stored to
 */
void mvhd_generate_uuid(uint8_t* uuid)
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

/**
 * \brief Calculate a VHD formatted timestamp from the current time
 */
uint32_t vhd_calc_timestamp(void)
{
        time_t start_time;
        time_t curr_time;
        double vhd_time;
        start_time = MVHD_START_TS; /* 1 Jan 2000 00:00 */
        curr_time = time(NULL);
        vhd_time = difftime(curr_time, start_time);
        return (uint32_t)vhd_time;
}

/**
 * \brief Return the created time from a VHD image
 * 
 * \param [in] vhdm Pointer to the MiniVHD metadata structure
 * 
 * \return The created time, as a Unix timestamp
 */
time_t vhd_get_created_time(MVHDMeta *vhdm)
{
        time_t vhd_time = (time_t)vhdm->footer.timestamp;
        time_t vhd_time_unix = MVHD_START_TS + vhd_time;
        return vhd_time_unix;
}

FILE* mvhd_fopen(const char* path, const char* mode, int* err) {
    FILE* f = NULL;
#ifdef _WIN32
    size_t path_len = strlen(path);
    size_t mode_len = strlen(mode);
    mvhd_utf16 new_path[260] = {0};
    int new_path_len = (sizeof new_path) - 2;
    mvhd_utf16 mode_str[5] = {0};
    int new_mode_len = (sizeof mode_str) - 2;
    int path_res = UTF8ToUTF16LE((unsigned char*)new_path, &new_path_len, (const unsigned char*)path, &path_len);
    int mode_res = UTF8ToUTF16LE((unsigned char*)mode_str, &new_mode_len, (const unsigned char*)mode, &mode_len);
    if (path_res > 0 && mode_res > 0) {
        f = _wfopen(new_path, mode_str);
        if (f == NULL) {
            mvhd_errno = errno;
            *err = MVHD_ERR_FILE;
        }
    } else {
        if (path_res == -1 || mode_res == -1) {
            *err = MVHD_ERR_UTF_SIZE;
        } else if (path_res == -2 || mode_res == -2) {
            *err = MVHD_ERR_UTF_TRANSCODING_FAILED;
        }
    }
#else
    f = fopen(path, mode);
    if (f == NULL) {
        mvhd_errno = errno;
        *err = MVHD_ERR_FILE;
    }
#endif
    return f;
}