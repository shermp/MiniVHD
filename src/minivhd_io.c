#include <stdlib.h>
#include <string.h>
#include "minivhd_internal.h"
#include "minivhd.h"

static inline void mvhd_check_sectors(int offset, int num_sectors, int total_sectors, int* transfer_sect, int* trunc_sect) {
    *transfer_sect = num_sectors;
    *trunc_sect = 0;
    if ((total_sectors - offset) < *transfer_sect) {
        *transfer_sect = total_sectors - offset;
        *trunc_sect = num_sectors - *transfer_sect;
    }
}
int mvhd_fixed_read(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff) {
    int64_t addr;
    int transfer_sectors, truncated_sectors;
    int total_sectors = vhdm->footer.geom.cyl * vhdm->footer.geom.heads * vhdm->footer.geom.spt;
    mvhd_check_sectors(offset, num_sectors, total_sectors, &transfer_sectors, &truncated_sectors);
    addr = (int64_t)offset * MVHD_SECTOR_SIZE;
    fseeko64(vhdm->f, addr, SEEK_SET);
    fread(out_buff, transfer_sectors*MVHD_SECTOR_SIZE, 1, vhdm->f);
    return truncated_sectors;
}

int mvhd_sparse_read(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff) {
    int transfer_sectors, truncated_sectors;
    int total_sectors = vhdm->footer.geom.cyl * vhdm->footer.geom.heads * vhdm->footer.geom.spt;
    mvhd_check_sectors(offset, num_sectors, total_sectors, &transfer_sectors, &truncated_sectors);
    uint8_t* buff = (uint8_t*)out_buff;
    int64_t addr;
    int s, ls, blk, prev_blk, sib;
    ls = offset + transfer_sectors;
    prev_blk = -1;
    for (s = offset; s <= ls; s++) {
        blk = s / vhdm->sect_per_block;
        sib = s % vhdm->sect_per_block;
        if (vhdm->block[blk].offset == MVHD_SPARSE_BLK) {
            memset(buff, 0, MVHD_SECTOR_SIZE);
        } else {
            if (blk != prev_blk) {
                prev_blk = blk;
                addr = (int64_t)((vhdm->block[blk].offset + vhdm->bm_sect_count + sib) * MVHD_SECTOR_SIZE);
                fseeko64(vhdm->f, addr, SEEK_SET);
            }
            fread(buff, MVHD_SECTOR_SIZE, 1, vhdm->f);
        }
        buff += MVHD_SECTOR_SIZE;
    }
    return truncated_sectors;
}