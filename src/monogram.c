#include <stdint.h>
#include <stdio.h>
#include <font.h>
// -----------------------------------
//  monogram
// -----------------------------------
// font_converter.py --add-ascii --add-range 0xb0 monogram-extended.ttf --height 16 --bpp 1
// Copyright Vinicius Menezio 2024
// monospace pixel typeface
// Creative Commons Zero v1.0 Universal
// https://creativecommons.org/publicdomain/zero/1.0/

static const uint8_t glyphs_monogram[388] = {
250,182,128, 87,212,175,168, 35,232,226,248,128,140, 68, 68, 70, 32,100,164,249, 73,160,224,106,
164,149, 88, 37, 93, 82,  0, 33, 62, 66,  0, 88,248,192,  8, 68, 68, 66,  0,116,103, 92,197,192,
 35,  8, 66, 19,224,116, 66, 34, 35,224,116, 66, 96,197,192, 74, 99,240,132, 32,252, 60, 16,197,
192,116, 33,232,197,192,248, 66, 34, 16,128,116, 98,232,197,192,116, 98,240,197,192,204, 80, 88,
 27, 32,193,128,248, 62,193,130,108,  0,116, 66, 34,  0,128,116,235, 89,193,192,116, 99, 31,198,
 32,244, 99,232,199,192,116, 97,  8, 69,192,244, 99, 24,199,192,252, 33,232, 67,224,252, 33,232,
 66,  0,116, 97,120,197,192,140, 99,248,198, 32,249,  8, 66, 19,224,  8, 66, 24,197,192,140,169,
138, 74, 32,132, 33,  8, 67,224,142,235, 24,198, 32,140,115, 89,198, 32,116, 99, 24,197,192,244,
 99,232, 66,  0,116, 99, 24,197,195,244, 99,232,198, 32,116, 96,224,197,192,249,  8, 66, 16,128,
140, 99, 24,197,192,140, 99, 21, 40,128,140, 99, 26,238, 32,140, 84, 69, 70, 32,140, 84, 66, 16,
128,248, 68, 68, 67,224,234,172,132, 16, 65,  4, 32,213, 92, 34,162,248,144,124, 99, 23,128,132,
 61, 24,199,192,116, 97, 23,  0,  8, 95, 24,197,224,116,127,  7,  0, 50, 81,228, 33,  0,124, 99,
 23,133,192,132, 61, 24,198, 32, 32, 24, 66, 19,224,  8,  6, 16,132, 49,112,132, 35, 46, 74, 32,
194, 16,132, 32,224,245,107, 90,128,244, 99, 24,128,116, 99, 23,  0,244, 99, 31, 66,  0,124, 99,
 23,132, 32,182, 97,  8,  0,124, 28, 31,  0, 66, 60,132, 32,224,140, 99, 23,128,140, 98,162,  0,
140,107, 85,  0,138,136,168,128,140, 99, 23,133,192,248,136,143,128, 41, 68,136,254,137, 20,160,
 77,128,105,150
};

// GLYPH DESCRIPTION
static const glyph_description_t glyph_dsc_monogram[96] = {
    {.width =  0, .height =  0, .lsb =  0, .tsb =  0, .advance =  6, .start_index =  0, },  // U+0020 ' '
    {.width =  1, .height =  7, .lsb =  2, .tsb =  7, .advance =  6, .start_index =  0, },  // U+0021 '!'
    {.width =  3, .height =  3, .lsb =  1, .tsb =  7, .advance =  6, .start_index =  1, },  // U+0022 '"'
    {.width =  5, .height =  6, .lsb =  0, .tsb =  6, .advance =  6, .start_index =  3, },  // U+0023 '#'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index =  7, },  // U+0024 '$'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 12, },  // U+0025 '%'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 17, },  // U+0026 '&'
    {.width =  1, .height =  3, .lsb =  2, .tsb =  7, .advance =  6, .start_index = 22, },  // U+0027 '''
    {.width =  2, .height =  7, .lsb =  2, .tsb =  7, .advance =  6, .start_index = 23, },  // U+0028 '('
    {.width =  2, .height =  7, .lsb =  1, .tsb =  7, .advance =  6, .start_index = 25, },  // U+0029 ')'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  6, .advance =  6, .start_index = 27, },  // U+002A '*'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  6, .advance =  6, .start_index = 31, },  // U+002B '+'
    {.width =  2, .height =  3, .lsb =  1, .tsb =  2, .advance =  6, .start_index = 35, },  // U+002C ','
    {.width =  5, .height =  1, .lsb =  0, .tsb =  4, .advance =  6, .start_index = 36, },  // U+002D '-'
    {.width =  1, .height =  2, .lsb =  2, .tsb =  2, .advance =  6, .start_index = 37, },  // U+002E '.'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 38, },  // U+002F '/'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 43, },  // U+0030 '0'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 48, },  // U+0031 '1'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 53, },  // U+0032 '2'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 58, },  // U+0033 '3'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 63, },  // U+0034 '4'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 68, },  // U+0035 '5'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 73, },  // U+0036 '6'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 78, },  // U+0037 '7'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 83, },  // U+0038 '8'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 88, },  // U+0039 '9'
    {.width =  1, .height =  6, .lsb =  2, .tsb =  6, .advance =  6, .start_index = 93, },  // U+003A ':'
    {.width =  2, .height =  7, .lsb =  1, .tsb =  6, .advance =  6, .start_index = 94, },  // U+003B ';'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  6, .advance =  6, .start_index = 96, },  // U+003C '<'
    {.width =  5, .height =  3, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 100, },  // U+003D '='
    {.width =  5, .height =  5, .lsb =  0, .tsb =  6, .advance =  6, .start_index = 102, },  // U+003E '>'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 106, },  // U+003F '?'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 111, },  // U+0040 '@'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 116, },  // U+0041 'A'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 121, },  // U+0042 'B'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 126, },  // U+0043 'C'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 131, },  // U+0044 'D'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 136, },  // U+0045 'E'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 141, },  // U+0046 'F'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 146, },  // U+0047 'G'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 151, },  // U+0048 'H'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 156, },  // U+0049 'I'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 161, },  // U+004A 'J'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 166, },  // U+004B 'K'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 171, },  // U+004C 'L'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 176, },  // U+004D 'M'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 181, },  // U+004E 'N'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 186, },  // U+004F 'O'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 191, },  // U+0050 'P'
    {.width =  5, .height =  8, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 196, },  // U+0051 'Q'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 201, },  // U+0052 'R'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 206, },  // U+0053 'S'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 211, },  // U+0054 'T'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 216, },  // U+0055 'U'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 221, },  // U+0056 'V'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 226, },  // U+0057 'W'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 231, },  // U+0058 'X'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 236, },  // U+0059 'Y'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 241, },  // U+005A 'Z'
    {.width =  2, .height =  7, .lsb =  2, .tsb =  7, .advance =  6, .start_index = 246, },  // U+005B '['
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 248, },  // U+005C '\'
    {.width =  2, .height =  7, .lsb =  1, .tsb =  7, .advance =  6, .start_index = 253, },  // U+005D ']'
    {.width =  5, .height =  3, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 255, },  // U+005E '^'
    {.width =  5, .height =  1, .lsb =  0, .tsb =  1, .advance =  6, .start_index = 257, },  // U+005F '_'
    {.width =  2, .height =  2, .lsb =  1, .tsb =  8, .advance =  6, .start_index = 258, },  // U+0060 '`'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 259, },  // U+0061 'a'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 263, },  // U+0062 'b'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 268, },  // U+0063 'c'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 272, },  // U+0064 'd'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 277, },  // U+0065 'e'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 281, },  // U+0066 'f'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 286, },  // U+0067 'g'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 291, },  // U+0068 'h'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 296, },  // U+0069 'i'
    {.width =  5, .height =  9, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 301, },  // U+006A 'j'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 307, },  // U+006B 'k'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 312, },  // U+006C 'l'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 317, },  // U+006D 'm'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 321, },  // U+006E 'n'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 325, },  // U+006F 'o'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 329, },  // U+0070 'p'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 334, },  // U+0071 'q'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 339, },  // U+0072 'r'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 343, },  // U+0073 's'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 347, },  // U+0074 't'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 352, },  // U+0075 'u'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 356, },  // U+0076 'v'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 360, },  // U+0077 'w'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 364, },  // U+0078 'x'
    {.width =  5, .height =  7, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 368, },  // U+0079 'y'
    {.width =  5, .height =  5, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 373, },  // U+007A 'z'
    {.width =  3, .height =  7, .lsb =  1, .tsb =  7, .advance =  6, .start_index = 377, },  // U+007B '{'
    {.width =  1, .height =  7, .lsb =  2, .tsb =  7, .advance =  6, .start_index = 380, },  // U+007C '|'
    {.width =  3, .height =  7, .lsb =  1, .tsb =  7, .advance =  6, .start_index = 381, },  // U+007D '}'
    {.width =  5, .height =  2, .lsb =  0, .tsb =  5, .advance =  6, .start_index = 384, },  // U+007E '~'
    {.width =  4, .height =  4, .lsb =  0, .tsb =  7, .advance =  6, .start_index = 386, },  // U+00B0 '°'
};

static const uint32_t code_points_monogram[1] = {
   176
};

const font_header_t f_monogram = {
    .magic = 0x005A54BE,
    .n_glyphs = 96,
    .map_start = 32,
    .map_n = 95,
    .map_table = code_points_monogram,
    .glyph_description_table = glyph_dsc_monogram,
    .glyph_data_table = glyphs_monogram,
    .linespace = 13,
    .flags = 0,
    .name = "monogram"
};
