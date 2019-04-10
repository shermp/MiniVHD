#include <stdint.h>
typedef enum {
    VHD_UTF_8,
    VHD_UTF_16_LE
} VHDUtfType;

int vhd_utf_convert(VHDUtfType toUTF, void* in_str, void** out_str);
size_t vhd_u16_strlen(uint16_t* u16_str);