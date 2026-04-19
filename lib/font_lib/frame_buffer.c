#include "frame_buffer.h"
#include <stdio.h>
#include <string.h>

// Support 1 bit (monochrome), 4 bit (greyscale) and 8 bit
uint8_t framebuffer[FB_SIZE];

// min is inclusive. max is not inclusive. like in numpy slices.
static int x_min = 0, x_max = FB_WIDTH, y_min = 0, y_max = FB_HEIGHT;

static void limit(int *a, int *b, int lim) {
    if (*a < 0)
        *a = 0;
    if (*a > lim)
        *a = lim;
    if (*b < 0)
        *b = 0;
    if (*b > lim)
        *b = lim;
    if (*a > *b) {
        int c = *a;
        *a = *b;
        *b = c;
    }
}

void set_draw_region(int x0, int y0, int x1, int y1) {
    limit(&x0, &x1, FB_WIDTH);
    limit(&y0, &y1, FB_HEIGHT);
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

void add_pixel(int x, int y, uint8_t value) {
    // printf("(%d, %d): %d\n", x, y, value);
    if (x < x_min || x >= x_max || y < y_min || y >= y_max)
        return;
#if FB_BPP == 8
    framebuffer[x + y * FB_WIDTH] |= value;
#elif FB_BPP == 4
    uint8_t *tmp = &framebuffer[x / 2 + y * FB_WIDTH / 2];
    if (x & 1)
        *tmp |= value >> 4;
    else
        *tmp |= value & 0xF0;
#elif FB_BPP == 1
    uint8_t *tmp = &framebuffer[x + (y / 8) * FB_WIDTH];
    if (value & 0x80)
        *tmp |= 1 << (y & 7);
#endif
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
    // Addressing is compatible with SSD1306
    uint8_t *tmp = &framebuffer[x + (y / 8) * FB_WIDTH];
    uint8_t mask = 1 << (y & 7);
    if (value & 0x80)
        *tmp |= mask;
    else
        *tmp &= ~mask;
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
    return (tmp >> (y & 7)) & 0x80 ? 0xFF : 0x00;
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

// void fast_h_line(int x, int y, int w, uint8_t value) {
//     if (w == 0)
//         return;
//     // printf("fast_h_line(%2d, %2d, %2d, %2d)\n", x, y, w, value);

// #if FB_BPP == 8
//     uint8_t *tmp = &framebuffer[x + y * FB_WIDTH];
//     memset(tmp, value, w);
// #elif FB_BPP == 4
//     uint8_t *tmp = &framebuffer[x / 2 + y * FB_WIDTH / 2];
//     // partial byte: set the right pixel, which is in the LSB
//     if (x & 1) {
//         *tmp++ = (*tmp & 0xF0) | (value >> 4);
//         w--;
//     }
//     // set double-pixels in the full bytes in the middle
//     unsigned n_full_bytes = w / 2;
//     memset(tmp, value, n_full_bytes);
//     w -= n_full_bytes * 2;
//     tmp += n_full_bytes;
//     // partial byte: set the left pixel, which is in the MSB
//     if (w > 0)
//         *tmp = (value & 0xF0) | (*tmp & 0x0F);
// #endif
// }

// static void vLine(unsigned x, unsigned y, unsigned h, uint8_t shade) {
//     for (unsigned i = 0; i < h; i++)
//         setPixel(x, y + i, shade);
// }

// void fillRect(int x0, int y0, int x1, int y1, uint8_t shade) {
//     shade &= 0x0F;

//     limit(&x0, &x1, DISPLAY_WIDTH - 1);
//     limit(&y0, &y1, DISPLAY_HEIGHT - 1);
//     unsigned w = x1 - x0 + 1;

//     update_window(x0, y0, x1, y1);

//     // printf("fillRect(%2d, %2d, %2d, %2d, %2d)\n", x0, x1, y0, y1, shade);

//     for (int row = y0; row <= y1; row++)
//         hLine(x0, row, w, shade);
// }

// void rect(int x0, int y0, int x1, int y1, uint8_t shade) {
//     limit(&x0, &x1, DISPLAY_WIDTH - 1);
//     limit(&y0, &y1, DISPLAY_HEIGHT - 1);
//     unsigned w = x1 - x0;
//     unsigned h = y1 - y0;

//     update_window(x0, y0, x1, y1);

//     // printf("    rect(%2d, %2d, %2d, %2d, %2d)\n", x0, x1, y0, y1, shade);

//     hLine(x0, y0, w, shade);
//     hLine(x0, y0 + h, w, shade);
//     vLine(x0, y0, h + 1, shade);
//     vLine(x0 + w, y0, h + 1, shade);
// }
// bool send_partial_fb(void) {
//     static bool is_done = true;
//     if (is_done) {
//         // Check if the framebuffer was touched by the user at all
//         if (g_x_min <= g_x_max && g_y_min <= g_y_max) {
//             // If yes, start a new row-by-row transfer
//             is_done = send_window_4(g_x_min, g_y_min, g_x_max, g_y_max);
//         }
//     } else {
//         // A row-by-row transfer is still in progress. Transfer the next row.
//         is_done = send_window_4(-1, -1, -1, -1);  // arguments are ignored
//     }
//     // if all rows were sent, mark the framebuffer as not modified
//     if (is_done) {
//         g_x_min = DISPLAY_WIDTH - 1;
//         g_x_max = 0;
//         g_y_min = DISPLAY_HEIGHT - 1;
//         g_y_max = 0;
//     }
//     return is_done;
// }

// void send_fb(void) { write_vram(g_frameBuff); }
