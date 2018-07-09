#ifndef HDD_FILE_VHD_H
#define HDD_FILE_VHD_H
/* Brief notes on the VHD format, as used by MiniVHD.

   The format is documented in the word doc titled
   "Virtual Hard Disk Image Format Specification", provided by Microsoft.
   This document was used as the primary source of information when
   implementing MiniVHD.

   Fixed VHD images are simply raw disk images, with a 512 byte footer
   appended at the end. Essential fields from the fixed footer are
   represented in the VHDMeta struct.

   Sparse, or dynamic images include a copy of the footer at the beginning
   of the file, followed by the sparse header (1024 bytes), followed by the 
   Block Allocation Table (BAT), followed by the data blocks, finally ending
   in the footer. Note that all structures in the file are aligned to a 
   sector boundary.

   Data blocks are preceeded by a sector bitmap (padded to a sector boundary).
   According to the spec, the purpose of the sector bitmap (for dynamic
   images) is to mark which sectors in the block are 'dirty' by setting the
   bitfield to '1'. Windows appears to mostly ignore the sector bitmap by
   setting the entire bitmap to '1' when the block is allocated, and
   otherwise not touching it. To simplify implementation, MiniVHD does the same.
   The overheads of managing the sector bitmap are probably not worth the
   effort and overheads it introduces, especially for writes. Note that the
   main purpose of the sector bitmap is for differencing images, which MiniVHD
   does not support.

   Accessing a sector is accomplished with the following formula:
        blk_num = floor(desired_sector / sectors_per_blk)
        sector_in_blk = desired_sector % sectors_per_blk

        abs_file_sect = BAT[blk_num] + sector_bitmap_sz + sector_in_blk

   Where BAT[] is an array of absolute sector offsets.

   Data blocks are allocated on demand when a write is made to a sector which
   resides in a block that is sparse (not yet allocated).
   */
  
/* Offset of each field in the VHD footer */
#define VHD_FOFF_COOKIE 0
#define VHD_FOFF_FEATURES 8
#define VHD_FOFF_VER 12
#define VHD_FOFF_DAT_OFF 16
#define VHD_FOFF_TS 24
#define VHD_FOFF_CR 28
#define VHD_FOFF_CR_VER 32
#define VHD_FOFF_CR_HST 36
#define VHD_FOFF_OG_SZ 40
#define VHD_FOFF_CU_SZ 48
#define VHD_FOFF_CYL 56
#define VHD_FOFF_HEAD 58
#define VHD_FOFF_SPT 59
#define VHD_FOFF_TYPE 60
#define VHD_FOFF_CHK 64
#define VHD_FOFF_UUID 68
#define VHD_FOFF_SS 84
#define VHD_FOFF_RES 85

/* Offset of each field in the VHD sparse header */
#define VHD_SOFF_COOKIE 0
#define VHD_SOFF_DAT_OFF 8
#define VHD_SOFF_BAT_OFF 16
#define VHD_SOFF_VERS 24
#define VHD_SOFF_MAX_BAT 28
#define VHD_SOFF_BLK_SZ 32
#define VHD_SOFF_CHK 36
#define VHD_SOFF_PAR_UUID 40
#define VHD_SOFF_PAR_TS 56
#define VHD_SOFF_RES_1 60
#define VHD_SOFF_PAR_NM 64
#define VHD_SOFF_PAR_LE_1 576
#define VHD_SOFF_RES_2 768

#define VHD_FOOTER_SZ 512
#define VHD_SPARSE_HEAD_SZ 1024
#define VHD_SECTOR_SZ 512

#define VHD_SPARSE_BLK 0xffffffff
#define VHD_DEF_BLOCK_SZ 2097152
#define VHD_START_TS 946684800
// #define MAX_BAT_SIZE_BYTES 16896
#define VHD_MAX_BAT_SIZE_BYTES 64512 /* Enough entries for largest VHD PCem can create */
#define VHD_MAX_CYL 65535 /* VHD stores the cylinders as a 16-bit unsigned int */
#define VHD_MAX_SZ_MB 32255 /* Using max (65535 * 16 * 63) geom  */
/* Win 10 appears to add 7 sectors of zero padding between blocks, and before the footer. */
#define VHD_BLK_PADDING 3584

typedef enum VHDError
{
        VHD_VALID,
        VHD_INVALID,
        VHD_WARN_BAD_CHECKSUM,
        VHD_WARN_SPT_SZ,
        VHD_ERR_GEOM_SIZE_MISMATCH,
        VHD_ERR_TYPE_UNSUPPORTED,
        VHD_ERR_BAD_DYN_CHECKSUM,
        VHD_RET_OK,
        VHD_RET_NOT_VHD,
        VHD_RET_MALLOC_ERROR
} VHDError;

typedef enum VHDType
{
        VHD_FIXED = 2,
        VHD_DYNAMIC = 3,
        VHD_DIFF = 4
} VHDType;

extern uint8_t VFT_CONECTIX_COOKIE[];
extern uint8_t VFT_CREATOR[];
extern uint8_t VFT_CREATOR_HOST_OS[];
extern uint8_t VHD_CXSPARSE_COOKIE[];

typedef struct VHDGeom
{
        uint16_t cyl;
        uint8_t heads;
        uint8_t spt;
} VHDGeom;
typedef struct VHDMeta
{
        uint32_t type;
        uint64_t curr_size;
        VHDGeom geom;
        uint64_t sparse_header_offset;
        uint64_t sparse_bat_offset;
        uint32_t *sparse_bat_arr;
        uint32_t sparse_max_bat;
        uint32_t sparse_block_sz;
        uint32_t sparse_spb;
        uint32_t sparse_sb_sz;
        uint8_t raw_footer[512];
        uint8_t raw_sparse_header[1024];
} VHDMeta;

int vhd_file_is_vhd(FILE *f);
VHDError vhd_read_file(FILE *f, VHDMeta *vhdm);
void vhd_create_file_sz(FILE *f, VHDMeta *vhdm, int sz_mb, VHDType type);
void vhd_create_file(FILE *f, VHDMeta *vhdm, int cyl, int heads, int spt, VHDType type);
VHDError vhd_check_validity(VHDMeta *vhdm);
VHDGeom vhd_calc_chs(uint32_t sz_mb);
time_t vhd_get_created_time(VHDMeta *vhdm);
int vhd_read_sectors(VHDMeta *vhdm, FILE *f, int offset, int nr_sectors, void *buffer);
int vhd_write_sectors(VHDMeta *vhdm, FILE *f, int offset, int nr_sectors, void *buffer);
int vhd_format_sectors(VHDMeta *vhdm, FILE *f, int offset, int nr_sectors);
void vhd_close(VHDMeta *vhdm);
#endif
