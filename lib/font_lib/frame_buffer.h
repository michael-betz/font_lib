#pragma once
#include <stdint.h>

// These defines need to be set by the user in the build flags (Makefile / platformio.ini)

// Width of the framebuffer in [pixels]
// #define FB_WIDTH 128

// Height of the framebuffer in [pixels]
// #define FB_HEIGHT 64

// Number of bits per pixel. Choose this according to the display you want to use. One of:
//   * 8: SDL simulator
//   * 4: use with SSD1322 based OLEDs with 16 shades
//	 * 1: use with SSD1306 based OLEDs with monochrome pixels
// #define FB_BPP 8

#define FB_SIZE ((FB_WIDTH * FB_HEIGHT * FB_BPP + 7) / 8)

typedef enum {
    DRAW_SET,
    DRAW_ADD,
    DRAW_SUB,
    DRAW_INV,
} t_draw_mode;

extern uint8_t framebuffer[FB_SIZE];

// output a single pixel with a dynamically selected draw mode.
// Used by font and graphics functions
extern void (*pixel_ptr)(int, int, uint8_t);

// Select a drawing mode
void set_draw_mode(t_draw_mode mode);

// Outside this rectangle, pixels will not be modified
void set_draw_region(int x0, int y0, int x1, int y1);

// removes a previously set draw-region restriction
void set_draw_region_full();

// Note that for the below functions, value has always 8 bit range, independent of the FB_BPP
// setting

// Set a pixel to value. But only within the draw-region.
void set_pixel(int x, int y, uint8_t value);

// Additively increase the brightness value of a pixel
void add_pixel(int x, int y, uint8_t value);

// Decrease the brightness value of a pixel
void subtract_pixel(int x, int y, uint8_t value);

// Invert the brightness value of the pixel by interpolating between bg and (255 - bg)
void invert_pixel(int x, int y, uint8_t value);

// return a pixel value.
uint8_t get_pixel(int x, int y);

// Set all pixels to a shade. Ignores draw region.
void fill(uint8_t shade);

// Optimization potential: set several pixels at once (memset)
static inline void draw_hline(int x0, int x1, int y, uint8_t value) {
    for (int x = x0; x <= x1; x++)
        pixel_ptr(x, y, value);
}

static inline void draw_vline(int x, int y0, int y1, uint8_t value) {
    for (int y = y0; y <= y1; y++)
        pixel_ptr(x, y, value);
}
