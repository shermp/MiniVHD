#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#include <error.h>
#endif
#include "vhdutil.h"

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

FILE* vhd_fopen(char* utf8_path, char* mode) 
{
#ifdef _WIN32
        errno_t err = 0;
        char* u16_path = NULL;
        WCHAR u16_mode[5] = {0};
        /* Convert mode to UTF-16LE. Thankfully it's ASCII, which make the conversion simple...
           Note, we're on Windows, so guaranteed to be little endian, therefore, no byte swapping needed. */
        int mode_len = strlen(mode);
        if (mode_len > 0 && mode_len < 5) {
                for (int i = 0; i < mode_len; i++) {
                        u16_mode[i] = (WCHAR)mode[i];
                }
        }
        /* Convert utf8_path to UTF-16LE */
        vhd_utf_convert(VHD_UTF_16_LE, utf8_path, &u16_path);
        FILE* f = _wfopen((LPCWCH)u16_path, u16_mode);
        if (f == NULL) {_get_errno(&err);}
        free(u16_path);
        /* We want to mimic (_w)fopen() behavior */
        if (err) {_set_errno(err);}
        return f;
#else
        /* Non-Windows OS's speak UTF-8 for fopen() */
        return fopen(utf8_path, mode);
#endif
}