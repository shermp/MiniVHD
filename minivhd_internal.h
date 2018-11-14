#ifndef MINIVHD_INTERNAL_H
#define MINIVHD_INTERNAL_H

#include <stdint.h>

extern uint8_t VFT_CONECTIX_COOKIE[];
extern uint8_t VFT_CREATOR[];
extern uint8_t VFT_CREATOR_HOST_OS[];
extern uint8_t VHD_CXSPARSE_COOKIE[];

/* Don't align struct members, so we can copy the header 
   and footer directly into the struct from the file.
   
   Should be compatible with MSVC and GCC */
# pragma pack(push, 1)
/* All values big endian */
typedef struct VHDFooterStruct
{
        uint8_t cookie[8];
        uint32_t features;
        uint32_t fi_fmt_vers;
        uint64_t data_offset;
        uint32_t timestamp;
        uint8_t cr_app[4];
        uint32_t cr_vers;
        uint8_t cr_host_os[4];
        uint64_t orig_sz;
        uint64_t curr_sz;
        struct {
                uint16_t cyl;
                uint8_t heads;
                uint8_t spt;
        } geom;
        uint32_t disk_type;
        uint32_t checksum;
        uint8_t uuid[16];
        uint8_t saved_st;
        uint8_t reserved[427];
} VHDFooterStruct;
/* All values big endian */
typedef struct VHDSparseStruct
{
        uint8_t cookie[8];
        uint64_t dat_offset;
        uint64_t table_offset;
        uint32_t head_vers;
        uint32_t max_bat_ent;
        uint32_t block_sz;
        uint32_t checksum;
        uint8_t par_uuid[16];
        uint32_t par_timestamp;
        uint32_t reserved_1;
        uint8_t par_utf16_name[512];
        struct {
                uint32_t plat_code;
                uint32_t plat_data_space;
                uint32_t plat_data_len;
                uint32_t reserved;
                uint64_t plat_data_offset;
        } par_loc_entry[8];
        uint8_t reserved_2[256];
} VHDSparseStruct;
/* Restore default alignment behaviour */
# pragma pack(pop)

#endif