#include <stdlib.h>
#include <string.h>
#include "vhdutil.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#include <error.h>
#endif

int vhd_utf_convert(VHDUtfType toUTF, void* in_str, char** out_str) 
{
#ifdef _WIN32
        int out_buff_len = 0;
        size_t out_type = 0;
        if (toUTF == VHD_UTF_8) {
                out_buff_len = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)in_str, -1, NULL, 0, NULL, NULL);
                out_type = sizeof(CHAR);
        } else {
                out_buff_len = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)in_str, -1, NULL, 0);
                out_type = sizeof(WCHAR);
        }
        char *out = *out_str;
        out = calloc(out_buff_len, out_type);
        if (out == NULL) {
                return -1;
        }
        if (toUTF == VHD_UTF_8) {
                WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)in_str, -1, (LPSTR)out, out_buff_len, NULL, NULL);
        } else {
                MultiByteToWideChar(CP_UTF8, 0, (LPCCH)in_str, -1, (LPWSTR)out, out_buff_len);
        }
#else
        iconv_t cd = (toUTF == VHD_UTF_8) ? iconv_open("UTF-8", "UTF-16LE") : iconv_open("UTF-16LE", "UTF-8");
        /* Iconv doesn't have an easy way of performing a 'dry run'. So we allocate a buffer that is sure to be
           large enough. */
        char *in = (char*)in_str;
        size_t in_len = (toUTF == VHD_UTF_8) ? (vhd_u16_strlen(in) * sizeof(uint16_t)) : strlen(in);
        size_t out_len = (toUTF == VHD_UTF_8) ? ((in_len * 2) + 1) : ((in_len * 4) + 2);
        char *out = *out_str;
        out = calloc(out_len, sizeof *out);
        if (out == NULL) {
                return -1;
        }
        size_t res = iconv(cd, &in, &in_len, &out, &out_len);
        if (res == (size_t)-1) {
                return -1;
        }
#endif
}

size_t vhd_u16_strlen(uint16_t* u16_str) {
        size_t count = 0;
        while(*u16_str) {
                count++;
                u16_str += 1;
        }
        return count;
}