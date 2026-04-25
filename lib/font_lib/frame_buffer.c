#include "frame_buffer.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Support 1 bit (monochrome), 4 bit (greyscale) and 8 bit
uint8_t framebuffer[FB_SIZE];

// min is inclusive. max is not inclusive. like in numpy slices.
static int x_min = 0, x_max = FB_WIDTH, y_min = 0, y_max = FB_HEIGHT;

// Allows to output pixels with a dynamically selected drawing mode
void (*pixel_ptr)(int, int, uint8_t) = set_pixel;

void set_draw_mode(t_draw_mode mode) {
    if (mode == DRAW_SUB)
        pixel_ptr = subtract_pixel;
    else if (mode == DRAW_ADD)
        pixel_ptr = add_pixel;
    else if (mode == DRAW_INV)
        pixel_ptr = invert_pixel;
    else
        pixel_ptr = set_pixel;
}

static inline void limit(int *a, int *b, int lim_min, int lim_max) {
    if (*a < lim_min)
        *a = lim_min;
    if (*a > lim_max)
        *a = lim_max;
    if (*b < lim_min)
        *b = lim_min;
    if (*b > lim_max)
        *b = lim_max;
    if (*a > *b) {
        int c = *a;
        *a = *b;
        *b = c;
    }
}

void set_draw_region(int x0, int y0, int x1, int y1) {
    limit(&x0, &x1, 0, FB_WIDTH);
    limit(&y0, &y1, 0, FB_HEIGHT);
    x_min = x0;
    x_max = x1;
    y_min = y0;
    y_max = y1;
}

void set_draw_region_full() {
    x_min = 0;
    x_max = FB_WIDTH;
    y_min = 0;
    y_max = FB_HEIGHT;
}

// Saturated math addition / subtraction
static inline uint8_t qadd8(uint8_t a, uint8_t b) {
    unsigned int t = a + b;
    if (t > 0xFF)
        return 0xFF;
    return (uint8_t)(t);
}
static inline uint8_t qsub8(uint8_t a, uint8_t b) {
    int t = a - b;
    if (t < 0)
        return 0;
    return (uint8_t)(t);
}
static inline uint8_t qadd4(uint8_t a, uint8_t b) {
    unsigned int t = a + b;
    if (t > 0xF)
        return 0xF;
    return (uint8_t)(t);
}

void set_pixel(int x, int y, uint8_t value) {
    if (x < x_min || x >= x_max || y < y_min || y >= y_max)
        return;
#if FB_BPP == 8
    framebuffer[x + y * FB_WIDTH] = value;
#elif FB_BPP == 4
    uint8_t *tmp = &framebuffer[x / 2 + y * FB_WIDTH / 2];
    if (x & 1)
        *tmp = (*tmp & 0xF0) | (value >> 4);
    else
        *tmp = (value & 0xF0) | (*tmp & 0x0F);
#elif FB_BPP == 1
    // Addressing is compatible with SSD1306 memory layout
    uint8_t *tmp = &framebuffer[x + (y / 8) * FB_WIDTH];
    uint8_t mask = 1 << (y & 7);
    if (value & 0x80)
        *tmp |= mask;
    else
        *tmp &= ~mask;
#endif
}

void add_pixel(int x, int y, uint8_t value) {
    // D("(%d, %d): %d\n", x, y, value);
    if (x < x_min || x >= x_max || y < y_min || y >= y_max)
        return;
#if FB_BPP == 8
    uint8_t *tmp = &framebuffer[x + y * FB_WIDTH];
    *tmp = qadd8(*tmp, value);
#elif FB_BPP == 4
    uint8_t *tmp = &framebuffer[x / 2 + y * FB_WIDTH / 2];
    if (x & 1)
        *tmp = (*tmp & 0xF0) | qadd4(*tmp & 0xF, value >> 4);
    else
        *tmp = qadd4(*tmp >> 4, value >> 4) << 4 | (*tmp & 0x0F);
#elif FB_BPP == 1
    uint8_t *tmp = &framebuffer[x + (y / 8) * FB_WIDTH];
    if (value & 0x80)
        *tmp |= 1 << (y & 7);
#endif
}

void subtract_pixel(int x, int y, uint8_t value) {
    // D("(%d, %d): %d\n", x, y, value);
    if (x < x_min || x >= x_max || y < y_min || y >= y_max)
        return;
#if FB_BPP == 8
    uint8_t *tmp = &framebuffer[x + y * FB_WIDTH];
    *tmp = qsub8(*tmp, value);
#elif FB_BPP == 4
    uint8_t *tmp = &framebuffer[x / 2 + y * FB_WIDTH / 2];
    if (x & 1)
        *tmp = (*tmp & 0xF0) | qsub8(*tmp & 0xF, value >> 4);
    else
        *tmp = qsub8(*tmp >> 4, value >> 4) << 4 | (*tmp & 0x0F);
#elif FB_BPP == 1
    uint8_t *tmp = &framebuffer[x + (y / 8) * FB_WIDTH];
    if (value & 0x80)
        *tmp &= ~(1 << (y & 7));
#endif
}

// Alpha blending between background and inverted background:
// (bg * (255 - val) + (255 - bg) * val) / 255
void invert_pixel(int x, int y, uint8_t value) {
    if (x < x_min || x >= x_max || y < y_min || y >= y_max)
        return;
#if FB_BPP == 8
    uint8_t *tmp = &framebuffer[x + y * FB_WIDTH];
    *tmp += (int)(value) - (2 * (int)(*tmp) * (int)(value) + 127) / 255;
#elif FB_BPP == 4
    uint8_t *tmp = &framebuffer[x / 2 + y * FB_WIDTH / 2];
    int backg = (x & 1) ? *tmp & 0x0F : *tmp >> 4;
    backg |= backg << 4;
    int new_val = backg + value - ((2 * backg * value + 127) >> 8);
    if (x & 1)
        *tmp = (*tmp & 0xF0) | (new_val >> 4);
    else
        *tmp = (new_val & 0xF0) | (*tmp & 0x0F);
#elif FB_BPP == 1
    if (value & 0x80)
        framebuffer[x + (y / 8) * FB_WIDTH] ^= 1 << (y & 7);
#endif
}

uint8_t get_pixel(int x, int y) {
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT)
        return 0;

#if FB_BPP == 8
    return framebuffer[x + y * FB_WIDTH];
#elif FB_BPP == 4
    uint8_t tmp = framebuffer[x / 2 + y * FB_WIDTH / 2];
    return (x & 1) ? (tmp << 4) | (tmp & 0x0F) : (tmp & 0xF0) | (tmp >> 4);
#elif FB_BPP == 1
    uint8_t tmp = framebuffer[x + (y / 8) * FB_WIDTH];
    return (tmp >> (y & 7)) & 1 ? 0xFF : 0x00;
#endif
}

// set all pixels to a shade
void fill(uint8_t value) {
#if FB_BPP == 1
    value = (value >= 0x80) ? 0xFF : 0;
#elif FB_BPP == 4
    // Only take the 4 MSBs but apply them for both pixels
    value &= 0xF0;
    value |= value >> 4;
#endif
    memset(framebuffer, value, FB_SIZE);
}
