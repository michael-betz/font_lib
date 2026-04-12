#pragma once
#include <stdint.h>

#ifndef FB_WIDTH
#define FB_WIDTH 128
#endif

#ifndef FB_HEIGHT
#define FB_HEIGHT 64
#endif

#ifndef FB_BPP
#define FB_BPP 4
#endif

#define FB_SIZE (FB_WIDTH * FB_HEIGHT * FB_BPP / 8)

// In 1 BPP mode: 1 = white, 0 = black
// In 4 BPP mode: 0 to 15 = shades of grey
// In 32 bit mode: a color to fade with pix_val (*)
void set_draw_color(unsigned val);

typedef enum { DRAW_SET, DRAW_ADD, DRAW_INVERT, DRAW_ALPHA } t_draw_mode;

// Select a draw mode
void set_draw_mode(t_draw_mode val);

// Outside this rectangle, pixels will not be modified
void set_draw_region(int x0, int y0, int x1, int y1);

// used by font.c to draw a pixel
void draw_pixel(int x, int y, uint8_t val);

// return a pixel value
uint8_t get_pixel(unsigned x, unsigned y);

// which framebuffer layer to use (for alpha compositing)
void set_draw_layer(int val);
