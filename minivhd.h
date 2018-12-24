#ifndef MINIVHD_H
#define MINIVHD_H
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

#include "minivhd_internal.h"

#define VHD_FOOTER_SZ 512
#define VHD_SPARSE_HEAD_SZ 1024
#define VHD_SECTOR_SZ 512

#define VHD_SPARSE_BLK 0xffffffff
#define VHD_DEF_BLOCK_SZ 2097152
#define VHD_START_TS 946684800

/* Enough entries for largest VHD MiniVHD can create.
   Calculated by ceil(65535 * 16 * 255 / 4096) * 4 */
#define VHD_MAX_BAT_SIZE_BYTES 261120
#define VHD_MAX_CYL 65535 /* VHD stores the cylinders as a 16-bit unsigned int */
#define VHD_MAX_SZ_MB 130559 /* Using max (65535 * 16 * 255) geom  */
/* Most (if not all) VHD implementations limit the block size to 2MB, we shall do the same, and therefore 
   the sector bitmap should be 512 bytes. Note, this value is aligned to sector boundary, so smaller block
   sizes still use a 512 byte sector bitmap. */
#define VHD_SECT_BM_SIZE 512 
/* Win 10 appears to add 7 sectors of zero padding between blocks, and before the footer. */
#define VHD_BLK_PADDING_SECT 7
#define VHD_MAX_PATH 260 /* Limit filepath lengths to Windows length. Length in characters */
#define VHD_PAR_LOC_PLAT_CODE_W2RU 0x57327275

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

typedef struct VHDGeom
{
        uint16_t cyl;
        uint8_t heads;
        uint8_t spt;
} VHDGeom;

typedef struct VHDSectorBitmap
{
        int cached;
        uint8_t bitmap[VHD_SECT_BM_SIZE];
} VHDSectorBitmap;

/* All values except those in raw_* structs are in the host endian format.
   There is no need to perform any conversion, MiniVHD does it for you. */
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
        VHDSectorBitmap *sparse_bitmap_arr;
        struct {
                FILE* f;
                struct VHDMeta *meta;
        } parent;
        VHDFooterStruct raw_footer;
        VHDSparseStruct raw_sparse_header;
} VHDMeta;

/* Check if given file is a VHD image
   f:    Pointer to potential VHD file */
int vhd_file_is_vhd(FILE *f);

/* Read VHD metadata into a VHDMeta struct
   f:    Pointer to VHD file
   vhdm: Pointer to VHDMeta struct
   
   Returns VHD_RET_OK on success. 
   Or VHD_RET_MALLOC_ERROR if there was a memory allocation error.
   Or VHD_RET_NOT_VHD if the provided file was not a VHD image */
VHDError vhd_read_file(FILE *f, VHDMeta *vhdm, const char *path);

/* Create a new, empty VHD image of the given size
   f:      Pointer to VHD file
   vhdm:   Pointer to VHDMeta struct
   sz_mb:  Size of VHD image, in megabytes. Size must be <= VHD_MAX_SZ_MB
   type:   Type of VHD image (VHD_FIXED or VHD_DYNAMIC) */
void vhd_create_file_sz(FILE *f, VHDMeta *vhdm, int sz_mb, VHDType type);

/* Create a new, empty VHD image with the given geometry.
   f:      Pointer to VHD file
   vhdm:   Pointer to VHDMeta struct
   cyl:    Number of cylinders. Max 65535
   heads:  Number of heads. Max 16
   spt:    Sectors per Track. Max 63
   type:   Type of VHD image (VHD_FIXED or VHD_DYNAMIC) */
void vhd_create_file(FILE *f, VHDMeta *vhdm, int cyl, int heads, int spt, VHDType type);

/* Basic VHD integrity check.
   vhdm:   Pointer to VHDMeta struct
   
   Returns VHD_VALID for a valid VHD file.
   Or VHD_ERR_TYPE_UNSUPPORTED if the VHD type is not fixed or dynamic.
   Or VHD_ERR_GEOM_SIZE_MISMATCH if there is a mismatch between size and geometry.
   Or VHD_WARN_SPT_SZ if spt > 63.
   Or VHD_ERR_BAD_DYN_CHECKSUM if the dynamic/sparse header checksum is incorrect.
   Or VHD_WARN_BAD_CHECKSUM if the footer checksum is incorrect. */
VHDError vhd_check_validity(VHDMeta *vhdm);

/* Calculate HDD gemoetry from the provided size, using the VHD algorithm.
   sz_mb:  Size of the desired size, in MB.
   
   Returns the calculated geometry in a VHDGeom struct */
VHDGeom vhd_calc_chs(uint32_t sz_mb);

/* Returns the VHD creation time as a Unix timestamp
   vhdm:   Pointer to VHDMeta struct */
time_t vhd_get_created_time(VHDMeta *vhdm);

/* Read sectors from VHD into a buffer.
   vhdm:       Pointer to VHDMeta struct
   f:          Pointer to VHD file
   offset:     Sector offset to begin reading from
   nr_sectors: Number of sectors to read
   buffer:     Buffer to read data into */
int vhd_read_sectors(VHDMeta *vhdm, FILE *f, int offset, int nr_sectors, void *buffer);

/* Write sectors from buffer to a VHD image.
   vhdm:       Pointer to VHDMeta struct
   f:          Pointer to VHD file
   offset:     Sector offset to begin writing to
   nr_sectors: Number of sectors to write
   buffer:     Buffer to read data from */
int vhd_write_sectors(VHDMeta *vhdm, FILE *f, int offset, int nr_sectors, void *buffer);

/* Format (zero) sectors.
   vhdm:       Pointer to VHDMeta struct
   f:          Pointer to VHD file
   offset:     Sector offset to begin writing to
   nr_sectors: Number of sectors to write */
int vhd_format_sectors(VHDMeta *vhdm, FILE *f, int offset, int nr_sectors);

/* Frees any memory associated with a VHDMeta struct. Does NOT close the VHD file itself.
   vhdm:   Pointer to VHDMeta struct */
void vhd_close(VHDMeta *vhdm);
#endif
