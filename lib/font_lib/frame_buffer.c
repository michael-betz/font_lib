#include "frame_buffer.h"
#include <stdio.h>
#include <string.h>

// Support 1 bit (monochrome), 4 bit (greyscale) and 8 bit
static uint8_t framebuffer[FB_SIZE];

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

void add_pixel(int x, int y, uint8_t value) {
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
    uint8_t *tmp = &framebuffer[x / 8 + y * FB_WIDTH / 8];
    *tmp |= (value & 0x80) >> (x & 7);
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
    uint8_t *tmp = &framebuffer[x / 8 + y * FB_WIDTH / 8];
    uint8_t mask = 0x80 >> (x & 7);
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
    uint8_t tmp = framebuffer[x / 8 + y * FB_WIDTH / 8];
    return (tmp << (x & 7)) & 0x80 ? 0xFF : 0x00;
#endif
}

// set all pixels to a shade
void fill(uint8_t shade) {
#if FB_BPP == 1
    shade = (shade >= 0x80) ? 0xFF : 0;
#elif FB_BPP == 4
    // Only take the 4 MSBs but apply them for both pixels
    shade &= 0xF0;
    shade |= shade >> 4;
#endif
    memset(framebuffer, shade, FB_SIZE);
}

// // Draw one horizontal line with a certain shade (fast, no checks)
// static void hLine(unsigned x, unsigned y, unsigned w, uint8_t shade) {
//     if (w == 0)
//         return;
//     // printf("hLine(%2d, %2d, %2d, %2d)\n", x, y, w, shade);

//     uint8_t *p = &g_frameBuff[x / 2 + y * (DISPLAY_WIDTH / 2)];

//     if (x & 0x01) {
//         *p = (*p & 0xF0) | shade;  // set lower nibble only
//         p++;
//         x++;
//         w--;
//     }

//     unsigned len = w / 2;
//     memset(p, (shade << 4) | shade, len);  // set bytes / words

//     if (((x + w) & 0x01)) {
//         p += len;
//         *p = (*p & 0x0F) | (shade << 4);  // set upper nibble only
//     }
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

// // --------------------
// //  Anti-aliased line
// // --------------------
// // hardcoded parameters for 4 bit greyscale mode
// // changed arithmetic to picorv32 native 32 bit
// #define INTENSITY_BITS 4
// #define BASE_COLOR 0x0F
// #define N_BITS 16  // number of bits of `ErrorAdj` and `ErrorAcc`

// // number of bits by which to shift ErrorAcc to get intensity level
// #define INTENSITY_SHIFT (N_BITS - INTENSITY_BITS)
// #define INTENSITY_MASK ((1 << INTENSITY_BITS) - 1)

// // draw white anti-aliased line from (x0, y0) to (x1, y1)
// void drawLine(int x0, int y0, int x1, int y1) {
//     uint16_t ErrorAdj, ErrorAcc;
//     unsigned ErrorAccTemp, Weighting;
//     int DeltaX, DeltaY, Temp, XDir;

//     // Make sure the line runs top to bottom
//     if (y0 > y1) {
//         Temp = y0;
//         y0 = y1;
//         y1 = Temp;
//         Temp = x0;
//         x0 = x1;
//         x1 = Temp;
//     }

//     update_window((x0 > x1) ? x1 : x0, y0, (x0 > x1) ? x0 : x1, y1);

//     // Draw the initial pixel, which is always exactly intersected by
//     // the line and so needs no weighting
//     setPixel(x0, y0, BASE_COLOR);

//     if ((DeltaX = x1 - x0) >= 0) {
//         XDir = 1;
//     } else {
//         XDir = -1;
//         DeltaX = -DeltaX;  // make DeltaX positive
//     }
//     // Special-case horizontal, vertical, and diagonal lines, which
//     // require no weighting because they go right through the center of
//     // every pixel
//     if ((DeltaY = y1 - y0) == 0) {
//         // Horizontal line
//         while (DeltaX-- != 0) {
//             x0 += XDir;
//             setPixel(x0, y0, BASE_COLOR);
//         }
//         return;
//     }
//     if (DeltaX == 0) {
//         // Vertical line
//         do {
//             y0++;
//             setPixel(x0, y0, BASE_COLOR);
//         } while (--DeltaY != 0);
//         return;
//     }
//     if (DeltaX == DeltaY) {
//         // Diagonal line
//         do {
//             x0 += XDir;
//             y0++;
//             setPixel(x0, y0, BASE_COLOR);
//         } while (--DeltaY != 0);
//         return;
//     }
//     // line is not horizontal, diagonal, or vertical
//     ErrorAcc = 0;
//     // Is this an X-major or Y-major line?
//     if (DeltaY > DeltaX) {
//         // Y-major line; calculate N_BITS-bit fixed-point fractional part of a
//         // pixel that X advances each time Y advances 1 pixel, truncating the
//         // result so that we won't overrun the endpoint along the X axis
//         ErrorAdj = ((uint32_t)DeltaX << N_BITS) / (uint32_t)DeltaY;
//         // Draw all pixels other than the first and last
//         while (--DeltaY) {
//             ErrorAccTemp = ErrorAcc;  // remember current accumulated error
//             ErrorAcc += ErrorAdj;     // calculate error for next pixel
//             if (ErrorAcc <= ErrorAccTemp)
//                 x0 += XDir;
//             y0++;  // Y-major, so always advance Y
//             Weighting = ErrorAcc >> INTENSITY_SHIFT;
//             setPixel(x0, y0, Weighting ^ INTENSITY_MASK);
//             setPixel(x0 + XDir, y0, Weighting);
//         }
//         // the final pixel, which is always exactly intersected by the line
//         setPixel(x1, y1, BASE_COLOR);
//         return;
//     }
//     // It's an X-major line;
//     ErrorAdj = ((uint32_t)DeltaY << N_BITS) / (uint32_t)DeltaX;
//     // Draw all pixels other than the first and last
//     while (--DeltaX) {
//         ErrorAccTemp = ErrorAcc;  // remember current accumulated error
//         ErrorAcc += ErrorAdj;     // calculate error for next pixel
//         if (ErrorAcc <= ErrorAccTemp)
//             y0++;
//         x0 += XDir;  // X-major, so always advance X
//         Weighting = ErrorAcc >> INTENSITY_SHIFT;
//         setPixel(x0, y0, Weighting ^ INTENSITY_MASK);
//         setPixel(x0, y0 + 1, Weighting);
//     }
//     setPixel(x1, y1, BASE_COLOR);
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
