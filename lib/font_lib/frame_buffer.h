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

// Outside this rectangle, pixels will not be modified
void set_draw_region(int x0, int y0, int x1, int y1);

// Note that for the below functions, value has always 8 bit range, independent of the FB_BPP setting

// Additively increase the brightness value of a pixel. Used by font.c to draw glyphs
void add_pixel(int x, int y, uint8_t value);

// set a pixel to value.
void set_pixel(int x, int y, uint8_t value);

// return a pixel value.
uint8_t get_pixel(int x, int y);

// set all pixels to a shade
void fill(uint8_t shade);
