/**
 * \file
 * \brief Utility functions
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UUID_H
#include <uuid/uuid.h>
#endif
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