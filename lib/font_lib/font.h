#ifndef FONT_H
#define FONT_H

#include <stdbool.h>
#include <stdint.h>

enum {
    // Flag is set if outline glyphs are available in the font,
    // which doubles the number of glyphs
    FLAG_HAS_OUTLINE = 1 << 0,

    // Bits / pixel: 0b00 = 1 bit, 0b01 = 2 bit, 0b10 = 4 bit, 0b11 = 8 bit
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

    // simple ASCII mapping parameters
    uint16_t map_start;  // the first glyph maps to this codepoint
    uint16_t map_n;      // how many glyphs map to incremental codepoints

    // When loading from a header file, these are pointers to the tables in memory
    // When loading from a .fnt file, these are the file-offsets to the tables.
#ifdef FNT_SUPPORT
    uint32_t *map_table;  // may be NULL for pure ASCII mapping
    glyph_description_t *glyph_description_table;
    uint8_t *glyph_data_table;
#else
    const uint32_t *map_table;
    const glyph_description_t *glyph_description_table;
    const uint8_t *glyph_data_table;
#endif
    uint16_t linespace;
    uint8_t flags;  // See FLAG_* enum above
    char *name;
} font_header_t;

// Text alignment and horizontal anchor point
typedef enum { A_LEFT, A_CENTER, A_RIGHT, A_RIGHT_REF_LEFT } t_align;

#ifdef FNT_SUPPORT
// Load a <filePrefix>.fnt file. Uses malloc(). Returns true on success.
bool init_from_file(const char *filePrefix);
#endif

void init_from_header(const font_header_t *header);

// Print infos about the loaded font
void print_font_info(void);

// get bounding box of the string `c` with `n` characters.
// Return values (by reference):
// o_left and o_right are the horizontal offsets from the anchor point x_a
// to the left and right edges of the bounding box
// o_top and o_bottom are the vertical offsets from the baseline y_a
// to the top and bottom of the bounding box
void fnt_get_bb(
    const char *c, unsigned n, t_align align, int *o_left, int *o_right, int *o_top, int *o_bottom);

// draws the chars from `c` at a specific position. Returns cursor_x.
int push_str(int x_a,        // x-offset of the anchor point in pixels
             int y_a,        // y-offset of the baseline in pixels
             const char *c,  // the UTF8 string to draw (can be zero terminated)
             unsigned n,     // length of the string
             t_align align   // Anchor point. One of A_LEFT, A_CENTER, A_RIGHT
);

// Draw characters to the screen like printf. Returns cursor_x.
int push_print(int x_a, int y_a, t_align align, const char *format, ...);

// This needs to be implemented by the framebuffer:
void draw_pixel(int xPixel, int yPixel, uint8_t pix_val);

#endif
