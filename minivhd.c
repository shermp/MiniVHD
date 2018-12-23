/* 
Copyright 2018 Sherman Perry

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UUID_H
#include <uuid/uuid.h>
#endif
#include "bswap.h"
#include "minivhd.h"

uint8_t VFT_CONECTIX_COOKIE[] = {'c', 'o', 'n', 'e', 'c', 't', 'i', 'x'};
uint8_t VFT_CREATOR[] = {'p', 'c', 'e', 'm'};
uint8_t VFT_CREATOR_HOST_OS[] = {'W', 'i', '2', 'k'};
uint8_t VHD_CXSPARSE_COOKIE[] = {'c', 'x', 's', 'p', 'a', 'r', 's', 'e'};

/* Global 'zeroed' and 'full' sector buffers */
uint8_t VHD_ZERO_SECTOR[VHD_SECTOR_SZ];
uint8_t VHD_FULL_SECTOR[VHD_SECTOR_SZ];

/* Internal functions */
static void vhd_init_global_buffers(void);
static void mk_guid(uint8_t *guid);
static uint32_t vhd_calc_timestamp(void);
static void vhd_raw_foot_to_meta(VHDMeta *vhdm);
static void vhd_sparse_head_to_meta(VHDMeta *vhdm);
static void vhd_new_raw(VHDMeta *vhdm);
static int vhd_bat_from_file(VHDMeta *vhdm, FILE *f);
static void vhd_update_bat(VHDMeta *vhdm, FILE *f, int blk);
static uint32_t vhd_generate_be_checksum(VHDMeta *vhdm, uint32_t type);
static VHDError vhd_validate_checksum(VHDMeta *vhdm);
static void vhd_create_blk(VHDMeta *vhdm, FILE *f, int blk_num);

/* A UUID is required, but there are no restrictions on how it needs
   to be generated. */
static void mk_guid(uint8_t *guid)
{
#if defined(HAVE_UUID_H)
        uuid_generate(guid);
//#elif defined(HAVE_OBJBASE_H)
//	CoCreateGuid( (GUID *)guid);
#else
        int n;

        srand(time(NULL));
        for (n = 0; n < 16; n++)
        {
                guid[n] = rand();
        }
        guid[6] &= 0x0F;
        guid[6] |= 0x40; /* Type 4 */
        guid[8] &= 0x3F;
        guid[8] |= 0x80; /* Variant 1 */
#endif
}
/* Init the global sector buffers */
static void vhd_init_global_buffers(void)
{
        memset(VHD_ZERO_SECTOR, 0x00, sizeof VHD_ZERO_SECTOR);
        memset(VHD_FULL_SECTOR, 0xFF, sizeof VHD_FULL_SECTOR);
}
/* Calculate the current timestamp. */
static uint32_t vhd_calc_timestamp(void)
{
        time_t start_time;
        time_t curr_time;
        double vhd_time;
        start_time = VHD_START_TS; /* 1 Jan 2000 00:00 */
        curr_time = time(NULL);
        vhd_time = difftime(curr_time, start_time);

        return (uint32_t)vhd_time;
}
time_t vhd_get_created_time(VHDMeta *vhdm)
{
        time_t vhd_time = (time_t)be32_to_cpu(vhdm->raw_footer.timestamp);
        time_t vhd_time_unix = VHD_START_TS + vhd_time;
        return vhd_time_unix;
}
/* Test if a file is a VHD. */
int vhd_file_is_vhd(FILE *f)
{
        uint8_t buffer[VHD_FOOTER_SZ];
        memset(buffer, 0, sizeof buffer);
        fseeko64(f, -VHD_FOOTER_SZ, SEEK_END);
        fread(buffer, 1, VHD_FOOTER_SZ, f);
        int valid_vhd = 0;
        // Check for valid cookie
        if (strncmp((char *)VFT_CONECTIX_COOKIE, (char *)buffer, 8) == 0)
        {
                valid_vhd = 1;
        }
        return valid_vhd;
}
/* Perform a basic integrity check of the VHD footer and sparse header. */
VHDError vhd_check_validity(VHDMeta *vhdm)
{
        VHDError status, chksum_status;
        chksum_status = vhd_validate_checksum(vhdm);
        if (vhdm->type != VHD_FIXED && vhdm->type != VHD_DYNAMIC)
        {
                return status = VHD_ERR_TYPE_UNSUPPORTED;
        }
        else if (vhdm->curr_size < ((uint64_t)vhdm->geom.cyl * vhdm->geom.heads * vhdm->geom.spt * VHD_SECTOR_SZ))
        {
                return status = VHD_ERR_GEOM_SIZE_MISMATCH;
        }
        else if (vhdm->geom.spt > 63)
        {
                return status = VHD_WARN_SPT_SZ;
        }
        else if (chksum_status == VHD_ERR_BAD_DYN_CHECKSUM)
        {
                return status = VHD_ERR_BAD_DYN_CHECKSUM;
        }
        else if (chksum_status == VHD_WARN_BAD_CHECKSUM)
        {
                return status = VHD_WARN_BAD_CHECKSUM;
        }
        else
        {
                return status = VHD_VALID;
        }
}
VHDError vhd_read_file(FILE *f, VHDMeta *vhdm)
{
        vhd_init_global_buffers();
        VHDError ret = VHD_RET_OK;
        fseeko64(f, -VHD_FOOTER_SZ, SEEK_END);
        fread(&vhdm->raw_footer, 1, VHD_FOOTER_SZ, f);
        // Check for valid cookie
        if (strncmp((char *)VFT_CONECTIX_COOKIE, (char *)vhdm->raw_footer.cookie, 8) == 0)
        {
                /* Don't want a pointer to who knows where... */
                vhdm->sparse_bat_arr = NULL;
                vhd_raw_foot_to_meta(vhdm);
                if (vhdm->type == VHD_DYNAMIC)
                {
                        fseeko64(f, vhdm->sparse_header_offset, SEEK_SET);
                        fread(&vhdm->raw_sparse_header, 1, VHD_SPARSE_HEAD_SZ, f);
                        vhd_sparse_head_to_meta(vhdm);
                        if (!vhd_bat_from_file(vhdm, f))
                        {
                                ret = VHD_RET_MALLOC_ERROR;
                        }
                }
        }
        else
        {
                ret = VHD_RET_NOT_VHD;
        }
        return ret;
}
/* Convenience function to create VHD file by specifiying size in MB */
void vhd_create_file_sz(FILE *f, VHDMeta *vhdm, int sz_mb, VHDType type)
{
        VHDGeom chs = vhd_calc_chs((uint32_t)sz_mb);
        vhd_create_file(f, vhdm, chs.cyl, chs.heads, chs.spt, type);
}
/* Create VHD file from CHS geometry. */
void vhd_create_file(FILE *f, VHDMeta *vhdm, int cyl, int heads, int spt, VHDType type)
{
        vhd_init_global_buffers();
        uint64_t vhd_sz = (uint64_t)cyl * heads * spt * VHD_SECTOR_SZ;
        vhdm->curr_size = vhd_sz;
        vhdm->geom.cyl = (uint16_t)cyl;
        vhdm->geom.heads = (uint8_t)heads;
        vhdm->geom.spt = (uint8_t)spt;
        vhdm->type = type;
        vhdm->sparse_header_offset = VHD_FOOTER_SZ;
        vhdm->sparse_bat_offset = VHD_FOOTER_SZ + VHD_SPARSE_HEAD_SZ;
        vhdm->sparse_block_sz = VHD_DEF_BLOCK_SZ;
        vhdm->sparse_max_bat = vhdm->curr_size / vhdm->sparse_block_sz;
        if (vhdm->curr_size % vhdm->sparse_block_sz != 0)
        {
                vhdm->sparse_max_bat += 1;
        }
        vhd_new_raw(vhdm);
        if (type == VHD_DYNAMIC)
        {
                size_t s;
                uint8_t max_byte = 255U;
                fseeko64(f, 0, SEEK_SET);
                fwrite(&vhdm->raw_footer, VHD_FOOTER_SZ, 1, f);
                fseeko64(f, vhdm->sparse_header_offset, SEEK_SET);
                fwrite(&vhdm->raw_sparse_header, VHD_SPARSE_HEAD_SZ, 1, f);
                fseeko64(f, vhdm->sparse_bat_offset, SEEK_SET);
                for (s = 0; s < VHD_MAX_BAT_SIZE_BYTES; s++)
                {
                        fwrite(&max_byte, sizeof max_byte, 1, f);
                }
                for (s = 0; s < VHD_BLK_PADDING_SECT; s++)
                {
                        fwrite(VHD_ZERO_SECTOR, sizeof VHD_ZERO_SECTOR, 1, f);
                }
                fwrite(&vhdm->raw_footer, VHD_FOOTER_SZ, 1, f);
        }
        else
        {
                uint32_t vhd_sect_sz = vhdm->curr_size / VHD_SECTOR_SZ;
                uint32_t i;
                fseeko64(f, 0, SEEK_SET);
                for (i = 0; i < vhd_sect_sz; i++)
                {
                        fwrite(VHD_ZERO_SECTOR, sizeof VHD_ZERO_SECTOR, 1, f);
                }
                fwrite(&vhdm->raw_footer, VHD_FOOTER_SZ, 1, f);
        }
}
static void vhd_raw_foot_to_meta(VHDMeta *vhdm)
{
        vhdm->type = be32_to_cpu(vhdm->raw_footer.disk_type);
        vhdm->curr_size = be64_to_cpu(vhdm->raw_footer.curr_sz);
        vhdm->geom.cyl = be16_to_cpu(vhdm->raw_footer.geom.cyl);
        vhdm->geom.heads = vhdm->raw_footer.geom.heads;
        vhdm->geom.spt = vhdm->raw_footer.geom.spt;
        vhdm->sparse_header_offset = be64_to_cpu(vhdm->raw_footer.data_offset);
}
static void vhd_sparse_head_to_meta(VHDMeta *vhdm)
{
        vhdm->sparse_bat_offset = be64_to_cpu(vhdm->raw_sparse_header.table_offset);
        vhdm->sparse_max_bat = be32_to_cpu(vhdm->raw_sparse_header.max_bat_ent);
        vhdm->sparse_block_sz = be32_to_cpu(vhdm->raw_sparse_header.block_sz);
        vhdm->sparse_spb = vhdm->sparse_block_sz / VHD_SECTOR_SZ;
        vhdm->sparse_sb_sz = vhdm->sparse_spb / 8;
        if (vhdm->sparse_sb_sz % VHD_SECTOR_SZ != 0)
        {
                vhdm->sparse_sb_sz += (vhdm->sparse_sb_sz % VHD_SECTOR_SZ);
        }
}
static void vhd_new_raw(VHDMeta *vhdm)
{
        /* Zero buffers */
        memset(&vhdm->raw_footer, 0, VHD_FOOTER_SZ);
        memset(&vhdm->raw_sparse_header, 0, VHD_SPARSE_HEAD_SZ);
        /* Write to footer buffer. */
        strncpy((char *)vhdm->raw_footer.cookie, (char *)VFT_CONECTIX_COOKIE, sizeof(VFT_CONECTIX_COOKIE));
        vhdm->raw_footer.features = cpu_to_be32(0x00000002);
        vhdm->raw_footer.fi_fmt_vers = cpu_to_be32(0x00010000);
        if (vhdm->type == VHD_DYNAMIC)
        {
                vhdm->raw_footer.data_offset = cpu_to_be64(vhdm->sparse_header_offset);
        }
        else
        {
                vhdm->raw_footer.data_offset = 0xffffffffffffffff;
        }
        vhdm->raw_footer.timestamp = cpu_to_be32(vhd_calc_timestamp());
        strncpy((char *)vhdm->raw_footer.cr_app, (char *)VFT_CREATOR, sizeof(VFT_CREATOR));
        vhdm->raw_footer.cr_vers = cpu_to_be32(0x000e0000);
        strncpy((char *)vhdm->raw_footer.cr_host_os, (char *)VFT_CREATOR_HOST_OS, sizeof(VFT_CREATOR_HOST_OS));
        vhdm->raw_footer.orig_sz = cpu_to_be64(vhdm->curr_size);
        vhdm->raw_footer.curr_sz = cpu_to_be64(vhdm->curr_size);
        vhdm->raw_footer.geom.cyl = cpu_to_be16(vhdm->geom.cyl);
        vhdm->raw_footer.geom.heads = vhdm->geom.heads;
        vhdm->raw_footer.geom.spt = vhdm->geom.spt;
        vhdm->raw_footer.disk_type = cpu_to_be32(vhdm->type);
        uint8_t uuid[16];
        mk_guid(uuid);
        memcpy(vhdm->raw_footer.uuid, uuid, sizeof(uuid));
        vhdm->raw_footer.checksum = vhd_generate_be_checksum(vhdm, VHD_FIXED);
        /* Write to sparse header buffer */
        strncpy((char *)vhdm->raw_sparse_header.cookie, (char *)VHD_CXSPARSE_COOKIE, sizeof(VHD_CXSPARSE_COOKIE));
        vhdm->raw_sparse_header.dat_offset = 0xffffffffffffffff;
        vhdm->raw_sparse_header.table_offset = cpu_to_be64(vhdm->sparse_bat_offset);
        vhdm->raw_sparse_header.head_vers = cpu_to_be32(0x00010000);
        vhdm->raw_sparse_header.max_bat_ent = cpu_to_be32(vhdm->sparse_max_bat);
        vhdm->raw_sparse_header.block_sz = cpu_to_be32(vhdm->sparse_block_sz);
        vhdm->raw_sparse_header.checksum = vhd_generate_be_checksum(vhdm, VHD_DYNAMIC);
}
/* Create a dynamic array of the Block Allocation Table as stored in the file. */
static int vhd_bat_from_file(VHDMeta *vhdm, FILE *f)
{
        if (!vhdm->sparse_bat_arr)
        {
                int ba_sz = sizeof(uint32_t) * vhdm->sparse_max_bat;
                vhdm->sparse_bat_arr = malloc(ba_sz);
                if (vhdm->sparse_bat_arr)
                {
                        memset(vhdm->sparse_bat_arr, 255, ba_sz);
                }
                else
                {
                        return 0;
                }
        }
        int b;
        for (b = 0; b < vhdm->sparse_max_bat; b++)
        {
                uint32_t curr_entry;
                uint64_t file_offset = vhdm->sparse_bat_offset + (b * 4);
                fseeko64(f, file_offset, SEEK_SET);
                fread(&curr_entry, 4, 1, f);
                vhdm->sparse_bat_arr[b] = be32_to_cpu(curr_entry);
        }
        return 1;
}
/* Updates the Block Allocation Table in the file with the new offset for a block. */
static void vhd_update_bat(VHDMeta *vhdm, FILE *f, int blk)
{
        uint64_t blk_file_offset = vhdm->sparse_bat_offset + (blk * 4);
        uint32_t blk_offset = cpu_to_be32(vhdm->sparse_bat_arr[blk]);
        fseeko64(f, blk_file_offset, SEEK_SET);
        fwrite(&blk_offset, 4, 1, f);
}
/* Calculates the checksum for a footer or header */
static uint32_t vhd_generate_be_checksum(VHDMeta *vhdm, uint32_t type)
{
        uint32_t chk = 0;
        if (type == VHD_DYNAMIC)
        {
                uint8_t *vhd_ptr = (uint8_t *)&vhdm->raw_sparse_header;
                int i;
                for (i = 0; i < VHD_SPARSE_HEAD_SZ; i++)
                {
                        if (i < offsetof(VHDSparseStruct, checksum) || i >= offsetof(VHDSparseStruct, par_uuid))
                        {
                                chk += vhd_ptr[i];
                        }
                }
        }
        else
        {
                uint8_t *vft_ptr = (uint8_t *)&vhdm->raw_footer;
                int i;
                for (i = 0; i < VHD_FOOTER_SZ; i++)
                {
                        if (i < offsetof(VHDFooterStruct, checksum) || i >= offsetof(VHDFooterStruct, uuid))
                        {
                                chk += vft_ptr[i];
                        }
                }
        }
        chk = ~chk;
        return cpu_to_be32(chk);
}
/* Validates the checksums in the VHD file */
static VHDError vhd_validate_checksum(VHDMeta *vhdm)
{
        VHDError ret = VHD_VALID;
        uint32_t stored_chksum, calc_chksum;
        if (vhdm->type == VHD_DYNAMIC)
        {
                stored_chksum = vhdm->raw_sparse_header.checksum;
                calc_chksum = vhd_generate_be_checksum(vhdm, VHD_DYNAMIC);
                if (stored_chksum != calc_chksum)
                {
                        ret = VHD_ERR_BAD_DYN_CHECKSUM;
                        return ret;
                }
        }
        stored_chksum = vhdm->raw_footer.checksum;
        calc_chksum = vhd_generate_be_checksum(vhdm, VHD_FIXED);
        if (stored_chksum != calc_chksum)
        {
                ret = VHD_WARN_BAD_CHECKSUM;
        }
        return ret;
}
/* Calculate the geometry from size (in MB), using the algorithm provided in
   "Virtual Hard Disk Image Format Specification, Appendix: CHS Calculation" */
VHDGeom vhd_calc_chs(uint32_t sz_mb)
{
        VHDGeom chs;
        uint32_t ts = ((uint64_t)sz_mb * 1024 * 1024) / VHD_SECTOR_SZ;
        uint32_t spt, heads, cyl, cth;
        if (ts > 65535 * 16 * 255)
        {
                ts = 65535 * 16 * 255;
        }
        if (ts >= 65535 * 16 * 63)
        {
                ts = 65535 * 16 * 63;
                spt = 255;
                heads = 16;
                cth = ts / spt;
        }
        else
        {
                spt = 17;
                cth = ts / spt;
                heads = (cth + 1023) / 1024;
                if (heads < 4)
                {
                        heads = 4;
                }
                if (cth >= (heads * 1024) || heads > 16)
                {
                        spt = 31;
                        heads = 16;
                        cth = ts / spt;
                }
                if (cth >= (heads * 1024))
                {
                        spt = 63;
                        heads = 16;
                        cth = ts / spt;
                }
        }
        cyl = cth / heads;
        chs.heads = heads;
        chs.spt = spt;
        chs.cyl = cyl;
        return chs;
}
/* Create new data block at the location of the existing footer.
   The footer gets replaced after the end of the new data block */
static void vhd_create_blk(VHDMeta *vhdm, FILE *f, int blk_num)
{
        uint8_t ftr[VHD_SECTOR_SZ];
        uint32_t new_blk_offset;
        fseeko64(f, -512, SEEK_END);
        new_blk_offset = (uint64_t)ftello64(f) / VHD_SECTOR_SZ;
        fread(ftr, 1, 512, f);
        fseeko64(f, -512, SEEK_END);
        /* Let's be sure we are not potentially overwriting a data block for some reason. */
        if (strncmp((char *)VFT_CONECTIX_COOKIE, (char *)ftr, 8) == 0)
        {
                uint32_t sb_sz = vhdm->sparse_sb_sz / VHD_SECTOR_SZ;
                uint32_t sect_to_write = sb_sz + vhdm->sparse_spb;
                int s;
                for (s = 0; s < sect_to_write; s++)
                {
                        if (s < sb_sz)
                        {
                                fwrite(VHD_FULL_SECTOR, VHD_SECTOR_SZ, 1, f);
                        }
                        else
                        {
                                fwrite(VHD_ZERO_SECTOR, VHD_SECTOR_SZ, 1, f);
                        }
                }
                for (s = 0; s < VHD_BLK_PADDING_SECT; s++)
                {
                        fwrite(VHD_ZERO_SECTOR, sizeof VHD_ZERO_SECTOR, 1, f);
                }
                fwrite(ftr, VHD_FOOTER_SZ, 1, f);
                vhdm->sparse_bat_arr[blk_num] = new_blk_offset;
                vhd_update_bat(vhdm, f, blk_num);
        }
}

int vhd_read_sectors(VHDMeta *vhdm, FILE *f, int offset, int nr_sectors, void *buffer)
{
        int transfer_sectors = nr_sectors;
        uint32_t total_sectors = vhdm->geom.cyl * vhdm->geom.heads * vhdm->geom.spt;
        /* This check comes from PCem */
        if ((total_sectors - offset) < transfer_sectors)
        {
                transfer_sectors = total_sectors - offset;
        }
        if (vhdm->type == VHD_DYNAMIC)
        {
                int sbsz = vhdm->sparse_sb_sz / VHD_SECTOR_SZ;
                int prev_blk = -1;
                int curr_blk;               
                uint32_t s, ls, sib;
                uint8_t *buff_ptr = (uint8_t*)buffer;
                ls = offset + (transfer_sectors - 1);
                for (s = offset; s <= ls; s++)
                {
                        curr_blk = s / vhdm->sparse_spb;
                        /* If the data block doesn't yet exist, fill the buffer with zero data */
                        if (vhdm->sparse_bat_arr[curr_blk] == VHD_SPARSE_BLK)
                        {
                                memset(buff_ptr, 0, VHD_SECTOR_SZ);
                        }
                        else
                        {
                                if (curr_blk != prev_blk)
                                {
                                        sib = s % vhdm->sparse_spb;
                                        uint32_t file_sect_offs = vhdm->sparse_bat_arr[curr_blk] + sbsz + sib;
                                        fseeko64(f, (uint64_t)file_sect_offs * VHD_SECTOR_SZ, SEEK_SET);
                                        prev_blk = curr_blk;
                                }
                                fread(buff_ptr, VHD_SECTOR_SZ, 1, f);
                        }
                        buff_ptr += VHD_SECTOR_SZ;
                }
                
        }
        else
        {
                /* Code from PCem */
                uint64_t addr = (uint64_t)offset * VHD_SECTOR_SZ;
                fseeko64(f, addr, SEEK_SET);
                fread(buffer, transfer_sectors * VHD_SECTOR_SZ, 1, f);
        }
        if (nr_sectors != transfer_sectors)
        {
                return 1;
        }
        return 0;
}
int vhd_write_sectors(VHDMeta *vhdm, FILE *f, int offset, int nr_sectors, void *buffer)
{
        int transfer_sectors = nr_sectors;
        uint32_t total_sectors = vhdm->geom.cyl * vhdm->geom.heads * vhdm->geom.spt;
        /* This check comes from PCem */
        if ((total_sectors - offset) < transfer_sectors)
        {
                transfer_sectors = total_sectors - offset;
        }
        if (vhdm->type == VHD_DYNAMIC)
        {
                int sbsz = vhdm->sparse_sb_sz / VHD_SECTOR_SZ;
                int prev_blk, curr_blk;
                prev_blk = -1;
                uint32_t s, ls, sib;
                uint8_t *buff_ptr = (uint8_t*)buffer;
                ls = offset + (transfer_sectors - 1);
                for (s = offset; s <= ls; s++)
                {
                        curr_blk = s / vhdm->sparse_spb;
                        /* We need to create a data block if it does not yet exist. */
                        if (vhdm->sparse_bat_arr[curr_blk] == VHD_SPARSE_BLK)
                        {
                                /* If the sector to write contains all zeros, hold off on block creation and therefore
                                   writing to file for this sector. */
                                if (memcmp(buff_ptr, VHD_ZERO_SECTOR, VHD_SECTOR_SZ) == 0)
                                {
                                        buff_ptr += VHD_SECTOR_SZ;
                                        continue;
                                }
                                else
                                {
                                        vhd_create_blk(vhdm, f, curr_blk);
                                }
                        }
                        if (curr_blk != prev_blk)
                        {
                                sib = s % vhdm->sparse_spb;
                                uint32_t file_sect_offs = vhdm->sparse_bat_arr[curr_blk] + sbsz + sib;
                                fseeko64(f, (uint64_t)file_sect_offs * VHD_SECTOR_SZ, SEEK_SET);
                                prev_blk = curr_blk;
                        }
                        fwrite(buff_ptr, VHD_SECTOR_SZ, 1, f);
                        buff_ptr += VHD_SECTOR_SZ;
                }
        }
        else
        {
                /* Code from PCem */
                uint64_t addr = (uint64_t)offset * VHD_SECTOR_SZ;
                fseeko64(f, addr, SEEK_SET);
                fwrite(buffer, transfer_sectors * VHD_SECTOR_SZ, 1, f);
        }
        if (nr_sectors != transfer_sectors)
        {
                return 1;
        }
        return 0;
}
int vhd_format_sectors(VHDMeta *vhdm, FILE *f, int offset, int nr_sectors)
{
        int transfer_sectors = nr_sectors;
        uint32_t total_sectors = vhdm->geom.cyl * vhdm->geom.heads * vhdm->geom.spt;
        /* This check comes from PCem */
        if ((total_sectors - offset) < transfer_sectors)
        {
                transfer_sectors = total_sectors - offset;
        }

        if (vhdm->type == VHD_DYNAMIC)
        {
                int sbsz = vhdm->sparse_sb_sz / VHD_SECTOR_SZ;
                int prev_blk, curr_blk;
                prev_blk = -1;
                uint32_t s, ls, sib;
                ls = offset + (transfer_sectors - 1);
                for (s = offset; s <= ls; s++)
                {
                        curr_blk = s / vhdm->sparse_spb;
                        if (vhdm->sparse_bat_arr[curr_blk] != VHD_SPARSE_BLK)
                        {
                                if (curr_blk != prev_blk)
                                {
                                        sib = s % vhdm->sparse_spb;
                                        uint32_t file_sect_offs = vhdm->sparse_bat_arr[curr_blk] + sbsz + sib;
                                        fseeko64(f, (uint64_t)file_sect_offs * VHD_SECTOR_SZ, SEEK_SET);
                                        prev_blk = curr_blk;
                                }
                                fwrite(VHD_ZERO_SECTOR, VHD_SECTOR_SZ, 1, f);
                        }
                }
        }
        else
        {
                int c;
                uint64_t addr = (uint64_t)offset * VHD_SECTOR_SZ;
                fseeko64(f, addr, SEEK_SET);
                for (c = 0; c < transfer_sectors; c++)
                {
                        fwrite(VHD_ZERO_SECTOR, VHD_SECTOR_SZ, 1, f);
                }
        }
        if (nr_sectors != transfer_sectors)
        {
                return 1;
        }
        return 0;
}

void vhd_close(VHDMeta *vhdm)
{
        if (vhdm->sparse_bat_arr)
        {
                free(vhdm->sparse_bat_arr);
                vhdm->sparse_bat_arr = NULL;
        }
}
