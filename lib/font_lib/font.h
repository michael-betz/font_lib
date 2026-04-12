#ifndef FONT_H
#define FONT_H

#include <stdbool.h>
#include <stdint.h>

enum {
    // Flag is set if outline glyphs are available in the font,
    // which doubles the number of glyphs
    FLAG_HAS_OUTLINE = 1 << 0,

    // Pixel format_B/_A: 00 = 1 bit, 01 = 8 bit monochrome, 10 = 4 bit monochrome
    FLAG_PIX_FORMAT_A = 1 << 1,
    FLAG_PIX_FORMAT_B = 1 << 2,

    // In monospace-mode all glyphs have the same size and avance_width.
    // If this is given, the glyph_dsc_t table only has a single entry.
    FLAG_MONOSPACE = 1 << 3,
};

typedef struct {
    uint8_t width;         // bitmap width [pixels]
    uint8_t height;        // bitmap height [pixels]
    int8_t lsb;            // left side bearing
    int8_t tsb;            // top side bearing
    int8_t advance;        // cursor advance width
    uint32_t start_index;  // offset to first byte of bitmap (relative to start
                           // of glyph description table)
} glyph_description_t;

typedef struct {
    uint32_t magic;
    uint16_t n_glyphs;  // number of glyphs in this font file

    // simple ascci mapping parameters
    uint16_t map_start;  // the first glyph maps to this codepoint
    uint16_t map_n;      // how many glyphs map to incremental codepoints

    // When loading from a header file, these are pointers to the tables in memory
    // When loading from a .fnt file, these are the file-offsets to the tables.
    uint32_t *map_table;  // equal to glyph_description_table when not present
    glyph_description_t *glyph_description_table;
    uint8_t *glyph_data_table;

    uint16_t linespace;
    // to vertically center the digits, add this to tsb
    int8_t yshift;
    // See FLAG_* enum above
    uint8_t flags;
    char *name;
} font_header_t;

// Text alignment and horizontal anchor point
#define A_LEFT 0
#define A_CENTER 1
#define A_RIGHT 2

// Load a <filePrefix>.fnt file. Uses malloc(). Returns true on success.
bool init_from_file(const char *filePrefix);

bool init_from_header(const font_header_t *header);

// draws a string of length `n` into `layer`
// to center it use x_a = 0 and
// colors cOutline and cFill
void push_str(int x_a,         // x-offset of the anchor point in pixels
              int y_a,         // y-offset in pixels
              const char *c,   // the UTF8 string to draw (can be zero terminated)
              unsigned n,      // length of the string
              unsigned align,  // Anchor point. One of A_LEFT, A_CENTER, A_RIGHT
              bool is_outline  // draw the outline glyphs if True, otherwise the
                               // fill glyphs
);

// This needs to be implemented by the framebuffer:
void draw_pixel(int xPixel, int yPixel, uint8_t pix_val);

#endif
