#include <string.h>
#include "bswap.h"
#include "vhdstrenc.h"

/* UTF-16 constants */
static const uint32_t LEAD_OFFSET = 0xD800 - (0x10000 >> 10);
static const uint32_t SURROGATE_OFFSET = 0x10000 - (0xD800 << 10) - 0xDC00;

/***** UTF-8 Decoder *****/

// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

static const uint8_t utf8d[] = 
{
        // The first part of the table maps bytes to character classes that
        // to reduce the size of the transition table and create bitmasks.
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

        // The second part is a transition table that maps a combination
        // of a state of the automaton and a character class to a state.
        0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
        12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
        12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
        12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
        12,36,12,12,12,12,12,12,12,12,12,12, 
};

static uint32_t inline
vhd_utf8_decode(uint32_t* state, uint32_t* codep, uint32_t byte) 
{
        uint32_t type = utf8d[byte];

        *codep = (*state != UTF8_ACCEPT) ?
                (byte & 0x3fu) | (*codep << 6) :
                (0xff >> type) & (byte);

        *state = utf8d[256 + *state + type];
        return *state;
}

/***************************************************************/

static int vhd_utf8_encode(uint32_t codepoint, uint8_t* utf8_buff)
{
        int num_bytes = 0;
        if (codepoint < 0x80) {
                utf8_buff[0] = codepoint;
                utf8_buff[1] = utf8_buff[2] = utf8_buff[3] = 0;
                num_bytes = 1;
        } else if (codepoint < 0x0800) {
                utf8_buff[0] = codepoint >> 6 & 0x1F | 0xC0;
                utf8_buff[1] = codepoint >> 0 & 0x3F | 0x80;
                utf8_buff[2] = utf8_buff[3] = 0;
                num_bytes = 2;
        } else if (codepoint < 0x010000) {
                utf8_buff[0] = codepoint >> 12 & 0x0F | 0xE0;
                utf8_buff[1] = codepoint >> 6  & 0x3F | 0x80;
                utf8_buff[2] = codepoint >> 0  & 0x3F | 0x80;
                utf8_buff[3] = 0;
                num_bytes = 3;
        } else if (codepoint < 0x110000) {
                utf8_buff[0] = codepoint >> 18 & 0x07 | 0xF0;
                utf8_buff[1] = codepoint >> 12 & 0x3F | 0x80;
                utf8_buff[2] = codepoint >> 6  & 0x3F | 0x80;
                utf8_buff[3] = codepoint >> 0  & 0x3F | 0x80;
                num_bytes = 4;
        }
        return num_bytes;
}

// computations
void vhd_utf16_to_utf8(uint16_t* u16_str, VHDUTF16Endian endian, void* dest, size_t u8_len) {
        uint16_t (*handle_endian)(uint16_t);
        if (endian == VHD_U16_BE) {
                handle_endian = be16_to_cpu;
        } else {
                handle_endian = le16_to_cpu;
        }
        uint32_t codepoint = 0;
        uint16_t curr_c, next_c;
        uint8_t mb_buff[4];
        int c = 0;
        int num_u8_bytes;
        uint8_t* dest_str = (uint8_t*)dest;
        while((*handle_endian)(u16_str[c]) && u8_len > 1)
        {
                curr_c = (*handle_endian)(u16_str[c]);
                next_c = (*handle_endian)(u16_str[c + 1]);
                /* Get codepoint below surrogate pairs */
                if (curr_c <= 0xD7FF) {
                        codepoint = (uint32_t)curr_c;
                /* Check high surrogate */
                } else if (curr_c <= 0xDBFF) {
                        /* Does the low surrogate exist? */
                        if (next_c >= 0xDC00 && next_c <= 0xDFFF) {
                                codepoint = (curr_c << 10) + next_c + SURROGATE_OFFSET;
                                c++;
                        } else {
                                /* Unpaired surrogate. Replace it with a valid unicode codepoint */
                                codepoint = 0xFFFD;
                        }
                /* Check for standalone low surrogate */
                } else if (curr_c <= 0xDFFF) {
                        codepoint = 0xFFFD;
                } else if (curr_c <= 0xFFFF) {
                        codepoint = curr_c;
                } else {
                        codepoint = 0xFFFD;
                }
                num_u8_bytes = vhd_utf8_encode(codepoint, mb_buff);
                if (num_u8_bytes && num_u8_bytes < u8_len) {
                        memcpy(dest_str, mb_buff, num_u8_bytes);
                        dest_str += num_u8_bytes;
                } else {
                        break;
                }
                c++;
        }
        /* Make sure to null terminate the UTF-8 string */
        *dest_str = '\0';
}

void vhd_utf8_to_utf16be(void* u8_str, uint16_t* u16_str, size_t u16_len) {
        uint32_t codepoint = 0;
        uint32_t state = UTF8_ACCEPT;
        uint32_t res;
        uint8_t *src_str = (uint8_t*)u8_str;
        while(*src_str && u16_len > 1)
        {
                res = vhd_utf8_decode(&state, &codepoint, *src_str);
                if (res == UTF8_REJECT) {
                        break;
                } else if (res == UTF8_ACCEPT) {
                        if (codepoint <= 0xFFFF) {
                                *u16_str = cpu_to_be16((uint16_t)codepoint);
                                u16_str += 1;
                                u16_len -= 1;
                        } else {
                                if (u16_len > 2) {
                                        uint16_t lead = LEAD_OFFSET + (codepoint >> 10);
                                        uint16_t trail = 0xDC00 + (codepoint & 0x3FF);
                                        u16_str[0] = cpu_to_be16(lead);
                                        u16_str[1] = cpu_to_be16(trail);
                                        u16_str += 2;
                                        u16_len -= 2;
                                } else {
                                        break;
                                }
                        }
                }
                src_str += 1;
        }
        *u16_str = 0;
}

void vhd_utf16be_to_host(uint16_t* u16be_str)
{
        while (*u16be_str)
        {
                *u16be_str = be16_to_cpu(*u16be_str);
                u16be_str += 1;
        }
}

size_t vhd_u16_strlen(uint16_t* u16_str) {
        size_t count = 0;
        while(*u16_str) {
                count++;
                u16_str += 1;
        }
        return count;
}