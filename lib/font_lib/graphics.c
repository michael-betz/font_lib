#include "graphics.h"
#include "frame_buffer.h"
#include <stdbool.h>
#include <stdint.h>

void draw_line_aa(int x0, int y0, int x1, int y1) {
#if FB_BPP == 1
    // No point in doing anti aliasing if there is no grey-scale support.
    draw_line(x0, y0, x1, y1);
    return;
#endif

    // dx_ = x1 - x0,  dx = abs(dx_),  sx = sign(dx_)
    int dx = x1 - x0, sx = dx < 0 ? -1 : 1;
    dx *= sx;
    int dy = y1 - y0, sy = dy < 0 ? -1 : 1;
    dy *= sy;

    if (!dx && !dy) {
        add_pixel(x0, y0, 0xFF);
        return;
    }

    int xMajor = dx > dy;
    int count = xMajor ? dx : dy;

    // Pre-calculate adjacent pixel axis offsets
    int dx_adj = xMajor ? 0 : sx;
    int dy_adj = xMajor ? sy : 0;

    // calculate the slope in 16.16 fixed-point
    uint32_t errAdj = ((uint32_t)(xMajor ? dy : dx) << 16) / count;
    uint32_t errAcc = 0;

    // Draw start pixel
    add_pixel(x0, y0, 0xFF);

    while (--count) {
        errAcc += errAdj;
        int step = errAcc >> 16;
        errAcc &= 0xFFFF;  // Mask to keep only the fractional part

        // Step along main axis always, step along cross axis if overflowed
        x0 += xMajor ? sx : step * sx;
        y0 += xMajor ? step * sy : sy;

        // need to invert the weight to get pixel intensity
        uint8_t weight = errAcc >> 8;
        add_pixel(x0, y0, weight ^ 0xFF);
        add_pixel(x0 + dx_adj, y0 + dy_adj, weight);
    }

    // Draw end pixel
    add_pixel(x1, y1, 0xFF);
}

void draw_line(int x0, int y0, int x1, int y1) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        add_pixel(x0, y0, 255);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = err << 1;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Helper function for ellipses. Draws 4 pixels in 4 quadrants, selected by q.
#define PLOT4(ox, oy, w)                                                                           \
    if (q & (1 << 0))                                                                              \
        add_pixel(xc + (ox), yc + (oy), (w));                                                      \
    if (q & (1 << 1))                                                                              \
        add_pixel(xc - (ox), yc + (oy), (w));                                                      \
    if (q & (1 << 2))                                                                              \
        add_pixel(xc + (ox), yc - (oy), (w));                                                      \
    if (q & (1 << 3))                                                                              \
        add_pixel(xc - (ox), yc - (oy), (w));

// Monochrome ellipse.
void draw_ellipse(int xc, int yc, int rx, int ry, unsigned q) {
    if (rx == 0 && ry == 0 && q) {
        add_pixel(xc, yc, 0xFF);
        return;
    }
    if (rx == 0 || ry == 0)
        return;

    int32_t rx2 = rx * rx;
    int32_t ry2 = ry * ry;
    int32_t tworx2 = rx2 << 1;
    int32_t twory2 = ry2 << 1;

    int32_t x = 0;
    int32_t y = ry;
    int32_t px = 0;
    int32_t py = tworx2 * y;

    int32_t err = ry2 - (rx2 * y) + (rx2 >> 2);

    while (px <= py) {
        PLOT4(x, y, 0xFF);
        x++;
        px += twory2;
        if (err < 0) {
            err += ry2 + px;
        } else {
            y--;
            py -= tworx2;
            err += ry2 + px - py;
        }
    }

    err = ry2 * x * (x + 1) + (ry2 >> 2) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;

    while (y >= 0) {
        PLOT4(x, y, 0xFF);
        y--;
        py -= tworx2;
        if (err > 0) {
            err += rx2 - py;
        } else {
            x++;
            px += twory2;
            err += rx2 - py + px;
        }
    }
}

// Anti aliased ellipse. Use midpoint algorithm and keep track of internal error.
// q is a 4-bit field selecting the quadrant. Set it to 0xF for the full ellipse.
// Needs to do 1 division / output pixel

void draw_ellipse_aa(int xc, int yc, int rx, int ry, unsigned q) {
    if (rx == 0 && ry == 0) {
        add_pixel(xc, yc, 0xFF);
        return;
    }
    if (rx == 0 || ry == 0)
        return;

#if FB_BPP == 1
    // No point in doing anti aliasing if there is no grey-scale support.
    draw_ellipse(xc, yc, rx, ry, q);
    return;
#endif

    int32_t a2 = rx * rx;
    int32_t b2 = ry * ry;
    int32_t x = 0;
    int32_t y = ry;
    int32_t px = 0;
    int32_t py = 2 * a2 * y;
    int32_t e = 0;

    while (px <= py) {
        int w = (e * 0xFF) / py;
        PLOT4(x, y, 0xFF - w);
        PLOT4(x, y - 1, w);

        x++;
        e += px + b2;
        px += 2 * b2;

        if (e >= py - a2) {
            y--;
            e -= py - a2;
            py -= 2 * a2;
        }
    }
    while (y >= 0) {
        int w = (e * 0xFF) / px;
        PLOT4(x, y, 0xFF - w);
        PLOT4(x - 1, y, w);

        if (y == 0) {
            break;
        }

        y--;
        e -= py - a2;
        py -= 2 * a2;

        if (e < 0) {
            x++;
            e += px + b2;
            px += 2 * b2;
        }
    }
}
