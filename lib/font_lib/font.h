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
    const char *name;
} font_header_t;

// Text alignment and horizontal anchor point. Following Pillow Image Font conventions
// https://pillow.readthedocs.io/en/stable/handbook/text-anchors.html#text-anchors
// Combine one of H_* and one of V_* with |.
// The default (0-value) corresponds to H_LEFT | V_BASELINE
typedef enum {
    H_LEFT = 0,
    H_MIDDLE = 1,
    H_RIGHT = 2,
    V_BASELINE = (0 << 4),
    V_TOP = (1 << 4),
    V_MIDDLE = (2 << 4),
    V_BOTTOM = (3 << 4),
} fnt_align_t;

// Bounding box representing absolute pixel coordinates
typedef struct {
    int left;
    int right;
    int top;
    int bottom;
} fnt_bbox_t;

#ifdef FNT_SUPPORT
// Load a <filePrefix>.fnt file. Uses malloc(). Returns true on success.
bool fnt_init_from_file(const char *filePrefix);
#endif

void fnt_init_from_header(const font_header_t *header);

// Print infos about the loaded font
void fnt_print_info(void);

// draws the chars from `c` at a specific position specified by x_a, y_a and align.
// Returns the bounding box of the drawn pixels.
fnt_bbox_t fnt_draw_text(int x_a,           // x-offset of the horizontal anchor point in pixels
                         int y_a,           // y-offset of the vertical anchor point in pixels
                         const char *c,     // the UTF8 string to draw (can be zero terminated)
                         unsigned n,        // max. length of the string
                         fnt_align_t align  // Anchor point position and alignment.
);

// As above but doesn't draw. Just returns the bounding box of the pixels which would be drawn.
fnt_bbox_t fnt_measure_text(int x_a, int y_a, const char *c, unsigned n, fnt_align_t align);

// Draw characters to the screen like printf. Returns the bounding box.
fnt_bbox_t
fnt_draw_printf(const int x_a, const int y_a, fnt_align_t align, const char *format, ...);

// As above but doesn't draw. Just returns the bounding box of the pixels which would be drawn.
fnt_bbox_t
fnt_measure_printf(const int x_a, const int y_a, fnt_align_t align, const char *format, ...);

#endif
