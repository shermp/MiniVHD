# MiniVHD - Minimalist VHD implementation in C

**MiniVHD** is a minimalist implementation of read/write/creation of VHD files. It is designed to read and write to VHD files at a sector level. It does not enable file access, or provide mounting options. Those features are left to more advanced libraries and/or the OS.

MiniVHD supports the following VHD version 1 types:
- Fixed VHD files
- Dynamic (sparse) VHD files

The following are currently not supported:
- Differential VHD files
- VHDX files of any type

MiniVHD was designed to work with the PCem emulator, which is designed to work with RAW disk images. Thus the usage of MiniVHD is similar how RAW images are accessed in PCem.

## Technical Considerations

Dynamic and differencing VHD files include a sector bitmap which precedes each data block. The purpose of this sector bitmap is to mark which sectors are 'dirty' (for dynamic disks), or sectors which differ from the parent image (differencing disks).

MiniVHD does NOT currently implement meaningful support for the sector bitmap. It mimics what Windows 10 appears to do, which is mark all sectors 'dirty' at block allocation. This is one of the main reasons for differencing images not being supported.

## Basic Usage

MiniVHD defines a meta structure which contains essential VHD parameters, as well as the raw footer and header. It does not provide any file management capabilities. It is your responsiblity to open, track, and close the VHD file.

### Preliminary

Include the *minivhd.h* header
```
#include "minivhd.h"
```

### Open Existing VHD

Open a file for read/write
```
FILE *f = fopen("path/to/image.vhd", "r+");
VHDMeta vhdm;
VHDError err;
/* Check if file is a VHD */
if (vhd_file_is_vhd(f))
{
    /* Parse the VHD metadata into the VHDMeta struct */
    vhd_read_file(f, &vhdm);

    /* Check the VHD for possible errors */
    err = vhd_check_validity(&vhdm);
    // check error status
}
// do stuff
/* Read 8 sectors from VHD, starting at sector 2048 */
uint8_t buff[4096]; // 8 * 512
vhd_read_sectors(&vhdm, f, 2048, 8, buff);

/* Write 255 to sectors just read */
memset(buff, 255, 4096);
vhd_write_sectors(&vhdm, f, 2048, 8, buff);
// Do stuff...
// More stuff...
/* Close VHD when done */
vhd_close(&vhdm);
fclose(f);
```

### Create New VHD

Create a new VHD for read/write
```
FILE *f = fopen("path/to/image.vhd", "w+");
VHDMeta vhdm;

/* Set the desired VHD type */
VHDType img_type = VHD_DYNAMIC;

/* Create a 2048MB VHD */
vhd_create_file_sz(f, &vhdm, 2048, img_type);
// Do stuff...
// More stuff...

/* Close when done */
vhd_close(&vhdm);
fclose(f);
```
