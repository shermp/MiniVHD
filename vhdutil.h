#include <stdint.h>
typedef enum {
    VHD_UTF_8,
    VHD_UTF_16_LE
} VHDUtfType;

/* Convert string between UTF-8 and UTF-16LE
   
   toUTF:    determines what encoding out_str will be
   in_str:   String to convert. String shall be null terminated UTF-16LE or UTF-8, and is expected to be the 
             opposite encoding from toUTF
   out_str:  Pointer to output string pointer. Encoding shall be determined by toUTF, and is null terminated.
             It is the callers repsponsibility to free the memory allocated by this function. */
int vhd_utf_convert(VHDUtfType toUTF, void* in_str, char** out_str);
/* Count the number of "characters" in a null terminated UTF-16 string. Not a true character count, as surrogate
   pairs are counted as two "characters". Appears to be the Windows definition of a character for filepath 
   purposes. */
size_t vhd_u16_strlen(uint16_t* u16_str);
/* Cross platform function to open a (potentially) non-ascii filepath */
FILE* vhd_fopen(char* utf8_path, char* mode);