#include <stdlib.h>
#include "minivhd_internal.h"
#include "minivhd.h"

static inline int mvhd_check_sectors(int offset, int num_sectors, int total_sectors) {
    int transfer_sect = num_sectors;
    int trunc_sect = 0;
    if ((total_sectors - offset) < transfer_sect) {
        transfer_sect = total_sectors - offset;
        trunc_sect = num_sectors - transfer_sect;
    }
    return trunc_sect;
}
int mvhd_fixed_read(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff) {
    int64_t addr;
    int transfer_sectors = num_sectors;
    int total_sectors = vhdm->footer.geom.cyl * vhdm->footer.geom.heads * vhdm->footer.geom.spt;
    int truncated_sectors = mvhd_check_sectors(offset, num_sectors, total_sectors);
    addr = (int64_t)offset * MVHD_SECTOR_SIZE;
    fseeko64(vhdm->f, addr, SEEK_SET);
    fread(out_buff, transfer_sectors*MVHD_SECTOR_SIZE, 1, vhdm->f);
    return truncated_sectors;
}

int mvhd_sparse_read(MVHDMeta* vhdm, int offset, int num_sectors, void* out_buff) {
    int64_t addr;
    int transfer_sectors = num_sectors;
    int total_sectors = vhdm->footer.geom.cyl * vhdm->footer.geom.heads * vhdm->footer.geom.spt;
    int truncated_sectors = mvhd_check_sectors(offset, num_sectors, total_sectors);
    return truncated_sectors;
}