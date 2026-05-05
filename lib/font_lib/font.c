#include "font.h"
#include "frame_buffer.h"
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Font descriptor
// The struct and its sub-tables can be in static memory (when loading from a .h)
// or in dynamic memory (when loading a binary .fnt file)
static const font_header_t *fntHeader = NULL;

#ifdef FNT_SUPPORT
// These are only used with init_from_file()
// If fntFile is not NULL, the tables below need to be freed dynamically!
static FILE *fntFile = NULL;
static char *fontFileName = NULL;
static int fontFileNameLen = 0;
#endif

// glyph cursor
static int cursor_x = 0, cursor_y = 0;

// decodes one UTF8 character at a time, keeping internal state
// returns a valid unicode character once complete or 0
static unsigned utf8_dec(char c) {
    // Unicode return value                   UTF-8 encoded chars
    // 00000000 00000000 00000000 0xxxxxxx -> 0xxxxxxx
    // 00000000 00000000 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx
    // 00000000 00000000 zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx
    // 00000000 000wwwzz zzzzyyyy yyxxxxxx -> 11110www 10zzzzzz 10yyyyyy
    // 10xxxxxx
    static unsigned readN = 0, result = 0;

    if ((c & 0x80) == 0) {
        // 1 byte character, nothing to decode
        readN = 0;  // reset state
        return c;
    }

    if (readN == 0) {
        result = 0;

        // first byte of several, initialize N bytes decode
        if ((c & 0xE0) == 0xC0) {
            readN = 1;  // 1 more byte to decode
            result |= (c & 0x1F) << 6;
            return 0;
        } else if ((c & 0xF0) == 0xE0) {
            readN = 2;
            result |= (c & 0x0F) << 12;
            return 0;
        } else if ((c & 0xF8) == 0xF0) {
            readN = 3;
            result |= (c & 0x07) << 18;
            return 0;
        } else {  // shouldn't happen?
            return 0;
        }
    }

    switch (readN) {
    case 1:
        result |= c & 0x3F;
        readN = 0;
        return result;

    case 2:
        result |= (c & 0x3F) << 6;
        break;

    case 3:
        result |= (c & 0x3F) << 12;
        break;

    default:
        readN = 1;
    }
    readN--;

    return 0;
}

static int binary_search(unsigned target, uint32_t *arr, int length) {
    int left = 0;
    int right = length - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        // Check if target is present at mid
        if (arr[mid] == target) {
            return mid;
        }

        // If target greater, ignore left half
        if (arr[mid] < target) {
            left = mid + 1;
        }
        // If target is smaller, ignore right half
        else {
            right = mid - 1;
        }
    }

    // Target is not present in array
    return -1;
}

// Returns NULL on error
static const glyph_description_t *get_glyph_description(unsigned glyph_index) {
    if (glyph_index >= fntHeader->n_glyphs) {
        D("invalid glyph index :( %d\n", glyph_index);
        return NULL;
    }

    if (fntHeader->flags & FLAG_MONOSPACE) {
        glyph_index = 0;
    }

    return &fntHeader->glyph_description_table[glyph_index];
}

// finds the glyph_index for unicode character `codepoint`
// returns -1 on failure
static int find_glyph_index(unsigned codepoint) {
    int glyph_index = -1;

    // check if the character is in the ascii map
    if (codepoint >= fntHeader->map_start &&
        (codepoint - fntHeader->map_start) < fntHeader->map_n) {
        glyph_index = codepoint - fntHeader->map_start;
    } else if (fntHeader->map_table != NULL) {
        // otherwise binary search in fntHeader->map_table
        glyph_index = binary_search(
            codepoint, (uint32_t *)fntHeader->map_table, fntHeader->n_glyphs - fntHeader->map_n);
        if (glyph_index > -1)
            glyph_index += fntHeader->map_n;
    }

    if (glyph_index < 0 || glyph_index >= fntHeader->n_glyphs) {
        // D("glyph not found: %d\n", glyph_index);
        return -1;
    }

    return glyph_index;
}

void fnt_print_info() {
#if (DEBUG == 1)
    if (fntHeader == NULL) {
        D("No font file loaded\n");
        return;
    }

    D("fontName: %s\n", fntHeader->name);
    D("n_glyphs: %d\n", fntHeader->n_glyphs);
    D("map_start: %d\n", fntHeader->map_start);
    D("map_n: %d\n", fntHeader->map_n);
    D("linespace: %d\n", fntHeader->linespace);
    D("flags: %x\n", fntHeader->flags);
    unsigned bpp = 1 << ((fntHeader->flags >> 1) & 3);
    D("bpp: %d\n", bpp);
#endif
}

#ifdef FNT_SUPPORT
void freeFont() {
    // If no file was loaded, no need to free anything
    if (fntFile == NULL)
        return;

    free(fntHeader->glyph_description_table);
    fntHeader->glyph_description_table = NULL;

    free(fntHeader->map_table);
    fntHeader->map_table = NULL;

    free((void *)fntHeader->name);
    fntHeader->name = NULL;

    free(fntHeader);
    fntHeader = NULL;

    free(fontFileName);
    fontFileName = NULL;
    fontFileNameLen = 0;

    fclose(fntFile);
    fntFile = NULL;
}

static bool load_helper(void **target, int len, const char *name) {
    if (len > 0) {
        *target = malloc(len);
        if (*target == NULL) {
            D("Couldn't allocate %s :( %d\n", name, len);
            return false;
        }

        int n_read = fread(*target, 1, len, fntFile);
        if (n_read != len) {
            D("Failed to read %s: %s\n", name, strerror(errno));
            return false;
        }
    }
    return true;
}

bool init_from_file(const char *fileName) {
    freeFont();

    fontFileNameLen = strlen(fileName) + 1;
    fontFileName = malloc(fontFileNameLen);
    if (fontFileName == NULL)
        return false;
    memcpy(fontFileName, fileName, fontFileNameLen);

    D("loading %s\n", fontFileName);

    fntFile = fopen(fileName, "r");
    if (fntFile == NULL) {
        D("Failed to open %s: %s\n", fileName, strerror(errno));
        goto error_out;
    }

    fntHeader = malloc(sizeof(font_header_t));
    if (fntHeader == NULL) {
        D("Couldn't allocate fntHeader\n");
        goto error_out;
    }

    int n_read = fread(&fntHeader, sizeof(font_header_t), 1, fntFile);
    if (n_read != 1) {
        D("Failed to read fntHeader: %s\n", strerror(errno));
        goto error_out;
    }

    if (fntHeader->magic != 0x005A54BE) {
        D("Wrong magic in .fnt file %x\n", (unsigned)fntHeader->magic);
        goto error_out;
    }

    if (!load_helper((void **)&fntHeader->name,
                     fntHeader->map_table_offset - sizeof(font_header_t),
                     "font name"))
        goto error_out;

    if (!load_helper((void **)&fntHeader->map_table,
                     fntHeader->glyph_description_offset - fntHeader->map_table_offset,
                     "unicode mapping table"))
        goto error_out;

    if (!load_helper((void **)&fntHeader->glyph_description_table,
                     fntHeader->glyph_data_offset - fntHeader->glyph_description_offset,
                     "glyph description table"))
        goto error_out;

    return true;

error_out:
    freeFont();
    return false;
}

uint8_t *get_bitmap_buff_from_file(const glyph_description_t *desc) {
    // Find the beginning and length of the glyph blob
    unsigned data_start = (unsigned)fntHeader->glyph_data_table + desc->start_index;

    unsigned pitch = 0;
    if (pix_mode == 0) {  // 1 bit per pixel
        pitch = (desc->width + 7) / 8;
    } else if (pix_mode == 1) {  // 1 byte per pixel
        pitch = desc->width;
    } else {
        D("Don't know this pixel mode: %d\n", pix_mode);
        return;
    }
    unsigned len = pitch * desc->height;

    if (len <= 0)
        return;

    if (fseek(fntFile, data_start, SEEK_SET) == -1) {
        D("glyph seek failed :( %s\n", strerror(errno));
        return;
    }

    uint8_t *buff = malloc(len);
    if (buff == NULL) {
        D("glyph overflow :( %s\n", strerror(errno));
        return NULL;
    }

    int n_read = fread(buff, 1, len, fntFile);
    if (n_read != len) {
        D("glyph read failed :( %s\n", strerror(errno));
        free(buff);
        return NULL;
    }

    return buff;
}
#endif  // FNT_SUPPORT

void fnt_init_from_header(const font_header_t *header) {
#ifdef FNT_SUPPORT
    freeFont();
#endif

    fntHeader = header;
}

static void
glyphToBuffer(int glyph_index, const glyph_description_t *desc, int offs_x, int offs_y) {
    // how many bits per pixel
    unsigned bpp = 1 << ((fntHeader->flags >> 1) & 3);
    // total size of glyph in [bytes]
    unsigned n_bytes = (desc->width * desc->height * bpp + 7) / 8;

    unsigned start_index = desc->start_index;
    if (fntHeader->flags & FLAG_MONOSPACE)
        start_index = glyph_index * n_bytes;

    const uint8_t *buff = NULL;

#ifdef FNT_SUPPORT
    unsigned end_index = start_index + n_bytes;
    if (fntFile)
        buff = get_bitmap_buff_from_file(start_index, end_index);
    else
        buff = &fntHeader->glyph_data_table[start_index];
#else
    buff = &fntHeader->glyph_data_table[start_index];
#endif

    if (buff == NULL) {
        D("Failed to load glyph image!\n");
        return;
    }

    // Extract the bpp bits for each pixel and draw it
    unsigned msb_mask = ((1 << bpp) - 1) << (8 - bpp);  // set the `bpp` MSBs
    unsigned current_byte = 0, n_bits = 0, tmp_byte = 0;
    for (int y = 0; y < desc->height; y++) {
        for (int x = 0; x < desc->width; x++) {
            if (n_bits < bpp) {
                // Shift in fresh data from the right
                tmp_byte = (tmp_byte << 8) | buff[current_byte++];
                n_bits += 8;
            }
            // Output in_bpp bits from the left and draw this pixel
            unsigned out_val = tmp_byte & msb_mask;
            // fill in the lower bits too, to get white for the max. input value
            unsigned msb_val = out_val >> bpp;
            while (msb_val) {
                out_val |= msb_val;
                msb_val >>= bpp;
            }
            pixel_ptr(x + offs_x, y + offs_y, out_val);

            // drop the bits we have just output
            tmp_byte = (tmp_byte << bpp) & 0xFF;
            n_bits -= bpp;
        }
    }
#ifdef FNT_SUPPORT
    if (fntFile)
        free(buff);
#endif
}

static void push_char(unsigned codepoint,
                      bool do_draw,
                      int *abs_min_x,
                      int *abs_max_x,
                      int *abs_min_y,
                      int *abs_max_y) {
    const glyph_description_t *desc = fntHeader->glyph_description_table;

    int glyph_index = find_glyph_index(codepoint);

    // If glyph not found, fallback to glyph 0 (space) but don't output pixels
    if (glyph_index >= 0) {
        desc = get_glyph_description(glyph_index);
        if (desc == NULL) {
            D("No glyph description found!\n");
            glyph_index = -1;
        }
    }

    // Calculate absolute screen pixel boundaries for the ink of this specific glyph
    int g_left = cursor_x + desc->lsb;
    int g_right = g_left + desc->width;
    int g_top = cursor_y - desc->tsb;
    int g_bottom = g_top + desc->height;

    if (abs_min_x && g_left < *abs_min_x)
        *abs_min_x = g_left;
    if (abs_max_x && g_right > *abs_max_x)
        *abs_max_x = g_right;
    if (abs_min_y && g_top < *abs_min_y)
        *abs_min_y = g_top;
    if (abs_max_y && g_bottom > *abs_max_y)
        *abs_max_y = g_bottom;

    if (do_draw && glyph_index >= 0)
        glyphToBuffer(glyph_index, desc, g_left, g_top);

    cursor_x += desc->advance;
}

// Internal function to get relative pixel bounding box of the rendered text
// This simulates a pen drawing glyphs starting at (0, 0) and tracks the exact pixel boundaries
static void fnt_get_bb(const char *c, unsigned n, bool single_line_mode, bbox_t *out_bbox) {
    int pen_x = 0;
    int pen_y = 0;
    int min_x = INT_MAX;
    int max_x = INT_MIN;
    int min_y = INT_MAX;
    int max_y = INT_MIN;
    bool has_glyphs = false;

    if (c == NULL)
        return;

    while (*c && n > 0) {
        unsigned codepoint = utf8_dec(*c++);
        n--;
        if (codepoint == 0)
            continue;

        if (codepoint == '\n' || codepoint == '\r') {
            if (single_line_mode)
                break;
            pen_x = 0;
            if (codepoint == '\n') {
                pen_y += fntHeader->linespace;
            }
            continue;
        }

        int glyph_index = find_glyph_index(codepoint);
        if (glyph_index < 0)
            glyph_index = 0;

        const glyph_description_t *desc = get_glyph_description(glyph_index);
        if (desc == NULL)
            continue;

        int g_left = pen_x + desc->lsb;
        int g_right = g_left + desc->width;
        int g_top = pen_y - desc->tsb;
        int g_bottom = g_top + desc->height;

        if (g_left < min_x)
            min_x = g_left;
        if (g_right > max_x)
            max_x = g_right;
        if (g_top < min_y)
            min_y = g_top;
        if (g_bottom > max_y)
            max_y = g_bottom;

        pen_x += desc->advance;
        has_glyphs = true;
    }
    utf8_dec('\0');

    if (!has_glyphs) {
        min_x = max_x = min_y = max_y = 0;
    }

    if (out_bbox) {
        out_bbox->left = min_x;
        out_bbox->right = max_x;
        out_bbox->top = min_y;
        out_bbox->bottom = max_y;
    }
}

// This is called whenever a new line starts, before drawing the next glyph to the screen
// It sets the cursor to the right horizontal position, taking the user-specified alignment into
// account. x_a is the position of the horizontal anchor point.
static void set_x_cursor(const int x_a, const char *c, const unsigned n, fnt_align_t align) {
    bbox_t bbox = {0};
    fnt_get_bb(c, n, true, &bbox);

    cursor_x = x_a;

    if ((align & 0x0F) == H_LEFT)
        cursor_x = x_a - bbox.left;
    else if ((align & 0x0F) == H_RIGHT)
        cursor_x = x_a - bbox.right;
    else if ((align & 0x0F) == H_MIDDLE)
        cursor_x = x_a - (bbox.left + bbox.right) / 2;
}

static bbox_t
fnt_text(const int x_a, const int y_a, const char *c, unsigned n, fnt_align_t align, bool do_draw) {
    // Default fallback in case of errors or empty strings
    bbox_t result = {x_a, x_a, y_a, y_a};

    if (fntHeader == NULL || c == NULL || n == 0)
        return result;

    int abs_min_x = INT_MAX;
    int abs_max_x = INT_MIN;
    int abs_min_y = INT_MAX;
    int abs_max_y = INT_MIN;

    // Set cursor_y to get the requested vertical alignment
    bbox_t block_bounds = {0};
    fnt_get_bb(c, n, false, &block_bounds);
    cursor_y = y_a;
    if ((align & 0xF0) == V_TOP)
        cursor_y = y_a - block_bounds.top;
    else if ((align & 0xF0) == V_BOTTOM)
        cursor_y = y_a - block_bounds.bottom;
    else if ((align & 0xF0) == V_MIDDLE)
        cursor_y = y_a - (block_bounds.top + block_bounds.bottom) / 2;

    set_x_cursor(x_a, c, n, align);

    while (*c && n > 0) {
        unsigned codepoint = utf8_dec(*c++);
        n--;
        if (codepoint == 0)
            continue;

        if (codepoint == '\n') {
            cursor_y += fntHeader->linespace;
            set_x_cursor(x_a, c, n, align);
            continue;
        }

        if (codepoint == '\r') {
            set_x_cursor(x_a, c, n, align);
            continue;
        }

        // push_char will update the absolute trackers
        push_char(codepoint, do_draw, &abs_min_x, &abs_max_x, &abs_min_y, &abs_max_y);
    }
    utf8_dec('\0');  // reset internal state

    // If the string contained actual printable characters, write the real bounds
    if (abs_min_x != INT_MAX) {
        result.left = abs_min_x;
        result.right = abs_max_x - 1;
        result.top = abs_min_y;
        result.bottom = abs_max_y - 1;
    }

    return result;
}

bbox_t fnt_draw_text(const int x_a, const int y_a, const char *c, unsigned n, fnt_align_t align) {
    return fnt_text(x_a, y_a, c, n, align, true);
}

bbox_t
fnt_measure_text(const int x_a, const int y_a, const char *c, unsigned n, fnt_align_t align) {
    return fnt_text(x_a, y_a, c, n, align, false);
}

static bbox_t
fnt_printf(int x_a, int y_a, fnt_align_t align, bool do_draw, const char *format, va_list argp) {
    static char buff[128];
    vsnprintf(buff, sizeof(buff), format, argp);
    return fnt_text(x_a, y_a, buff, sizeof(buff), align, do_draw);
}

bbox_t fnt_measure_printf(int x_a, int y_a, fnt_align_t align, const char *format, ...) {
    va_list argp;
    va_start(argp, format);
    bbox_t bb = fnt_printf(x_a, y_a, align, false, format, argp);
    va_end(argp);
    return bb;
}

bbox_t fnt_draw_printf(int x_a, int y_a, fnt_align_t align, const char *format, ...) {
    va_list argp;
    va_start(argp, format);
    bbox_t bb = fnt_printf(x_a, y_a, align, true, format, argp);
    va_end(argp);
    return bb;
}
