#include <stdint.h>
typedef enum {
    VHD_U16_BE,
    VHD_U16_LE
} VHDUTF16Endian;

void vhd_utf16_to_utf8(uint16_t* u16_str, VHDUTF16Endian endian, void* dest, size_t u8_len);
void vhd_utf8_to_utf16be(void* u8_str, uint16_t* u16_str, size_t u16_len);
void vhd_utf16be_to_host(uint16_t* u16be_str);
size_t vhd_u16_strlen(uint16_t* u16_str);