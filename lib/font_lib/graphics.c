#include "graphics.h"
#include "frame_buffer.h"
#include <stdbool.h>
#include <stdint.h>

// ----------------------------
//  Lines
// ----------------------------
static inline void draw_line_mono(int x0, int y0, int x1, int y1) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        pixel_ptr(x0, y0, 255);

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

static inline void draw_line_aa(int x0, int y0, int x1, int y1) {
    // dx_ = x1 - x0,  dx = abs(dx_),  sx = sign(dx_)
    int dx = x1 - x0, sx = dx < 0 ? -1 : 1;
    dx *= sx;
    int dy = y1 - y0, sy = dy < 0 ? -1 : 1;
    dy *= sy;

    if (!dx && !dy) {
        pixel_ptr(x0, y0, 0xFF);
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
    pixel_ptr(x0, y0, 0xFF);

    while (--count) {
        errAcc += errAdj;
        int step = errAcc >> 16;
        errAcc &= 0xFFFF;  // Mask to keep only the fractional part

        // Step along main axis always, step along cross axis if overflowed
        x0 += xMajor ? sx : step * sx;
        y0 += xMajor ? step * sy : sy;

        // need to invert the weight to get pixel intensity
        uint8_t weight = errAcc >> 8;
        pixel_ptr(x0, y0, weight ^ 0xFF);
        pixel_ptr(x0 + dx_adj, y0 + dy_adj, weight);
    }

    // Draw end pixel
    pixel_ptr(x1, y1, 0xFF);
}

void draw_line(int x0, int y0, int x1, int y1) {
    if (y0 == y1) {
        draw_hline(x0, x1, y0, 0xFF);  // Horizontal line
        return;
    }
    if (x0 == x1) {
        draw_vline(x0, y0, y1, 0xFF);  // Vertical line
        return;
    }
#if FB_BPP == 1
    // No point in doing anti aliasing if there is no grey-scale support.
    draw_line_mono(x0, y0, x1, y1);
#else
    draw_line_aa(x0, y0, x1, y1);
#endif
}

// ----------------------------
//  Rectangles
// ----------------------------
void draw_rectangle(int x0, int y0, int x1, int y1, uint8_t value) {
    draw_hline(x0, x1, y0, value);
    draw_hline(x0, x1, y1, value);
    draw_vline(x0, y0 + 1, y1 - 1, value);
    draw_vline(x1, y0 + 1, y1 - 1, value);
}

void draw_rectangle_c(int xc, int yc, int w, int h, uint8_t value) {
    draw_rectangle(xc - w / 2, yc - h / 2, xc + w / 2, yc + h / 2, value);
}

void fill_rectangle(int x0, int y0, int x1, int y1, uint8_t value) {
    for (int y = y0; y <= y1; y++)
        draw_hline(x0, x1, y, value);
}

void fill_rectangle_c(int xc, int yc, int w, int h, uint8_t value) {
    fill_rectangle(xc - w / 2, yc - h / 2, xc + w / 2, yc + h / 2, value);
}

void fill_rectangle_r(int x0, int y0, int x1, int y1, int r, uint8_t value) {
    int xc_left = x0 + r;
    int xc_right = x1 - r;
    int yc_top = y0 + r;
    int yc_bot = y1 - r;

    if (xc_left == xc_right && yc_top == yc_bot) {
        fill_ellipse(xc_left, yc_top, r, r, 15, value);
    } else if (yc_top == yc_bot) {
        fill_ellipse(xc_left, yc_top, r, r, 10, value);
        fill_ellipse(xc_right, yc_top, r, r, 5, value);
        if (xc_right > xc_left + 1) {
            fill_rectangle(xc_left + 1, y0, xc_right - 1, y1, value);
        }
    } else if (xc_left == xc_right) {
        fill_ellipse(xc_left, yc_top, r, r, 12, value);
        fill_ellipse(xc_left, yc_bot, r, r, 3, value);
        if (yc_bot > yc_top + 1) {
            fill_rectangle(x0, yc_top + 1, x1, yc_bot - 1, value);
        }
    } else {
        fill_ellipse(xc_left, yc_top, r, r, 8, value);
        fill_ellipse(xc_right, yc_top, r, r, 4, value);
        fill_ellipse(xc_left, yc_bot, r, r, 2, value);
        fill_ellipse(xc_right, yc_bot, r, r, 1, value);

        if (xc_right > xc_left + 1) {
            fill_rectangle(xc_left + 1, y0, xc_right - 1, yc_top, value);
            fill_rectangle(xc_left + 1, yc_bot, xc_right - 1, y1, value);
        }
        if (yc_bot > yc_top + 1) {
            fill_rectangle(x0, yc_top + 1, x1, yc_bot - 1, value);
        }
    }
}

void draw_rectangle_r(int x0, int y0, int x1, int y1, int r, uint8_t value) {
    int xc_left = x0 + r;
    int xc_right = x1 - r;
    int yc_top = y0 + r;
    int yc_bot = y1 - r;

    if (xc_left == xc_right && yc_top == yc_bot) {
        draw_ellipse(xc_left, yc_top, r, r, 15, value);
    } else if (yc_top == yc_bot) {
        draw_ellipse(xc_left, yc_top, r, r, 10, value);
        draw_ellipse(xc_right, yc_top, r, r, 5, value);
        if (xc_right > xc_left + 1) {
            draw_hline(xc_left + 1, xc_right - 1, y0, value);
            draw_hline(xc_left + 1, xc_right - 1, y1, value);
        }
    } else if (xc_left == xc_right) {
        draw_ellipse(xc_left, yc_top, r, r, 12, value);
        draw_ellipse(xc_left, yc_bot, r, r, 3, value);
        if (yc_bot > yc_top + 1) {
            draw_vline(x0, yc_top + 1, yc_bot - 1, value);
            draw_vline(x1, yc_top + 1, yc_bot - 1, value);
        }
    } else {
        draw_ellipse(xc_left, yc_top, r, r, 8, value);
        draw_ellipse(xc_right, yc_top, r, r, 4, value);
        draw_ellipse(xc_left, yc_bot, r, r, 2, value);
        draw_ellipse(xc_right, yc_bot, r, r, 1, value);

        if (xc_right > xc_left + 1) {
            draw_hline(xc_left + 1, xc_right - 1, y0, value);
            draw_hline(xc_left + 1, xc_right - 1, y1, value);
        }
        if (yc_bot > yc_top + 1) {
            draw_vline(x0, yc_top + 1, yc_bot - 1, value);
            draw_vline(x1, yc_top + 1, yc_bot - 1, value);
        }
    }
}

void draw_rectangle_rc(int xc, int yc, int w, int h, int r, uint8_t value) {
    draw_rectangle_r(xc - w / 2, yc - h / 2, xc + w / 2, yc + h / 2, r, value);
}

void fill_rectangle_rc(int xc, int yc, int w, int h, int r, uint8_t value) {
    fill_rectangle_r(xc - w / 2, yc - h / 2, xc + w / 2, yc + h / 2, r, value);
}

// ----------------------------
//  Ellipses
// ----------------------------
// Helper function for drawing empty ellipses
// Quadrant map: Bit 0=BR, 1=BL, 2=TR, 3=TL
static inline void plot4(int xc, int yc, int ox, int oy, uint8_t w, unsigned q) {
    if (w == 0 || q == 0)
        return;

    // Resolve vertical axis overlaps: Right (1, 4) overrides Left (2, 8)
    if (ox == 0) {
        if (q & 1)
            q &= ~2;
        if (q & 4)
            q &= ~8;
    }
    // Resolve horizontal axis overlaps: Bottom (1, 2) overrides Top (4, 8)
    if (oy == 0) {
        if (q & 1)
            q &= ~4;
        if (q & 2)
            q &= ~8;
    }

    if (q & 1)
        pixel_ptr(xc + ox, yc + oy, w);
    if (q & 2)
        pixel_ptr(xc - ox, yc + oy, w);
    if (q & 4)
        pixel_ptr(xc + ox, yc - oy, w);
    if (q & 8)
        pixel_ptr(xc - ox, yc - oy, w);
}

// Helper function for drawing filled ellipses
static inline void fill_row(int xc, int yc, int x_ext, int oy, uint8_t value, unsigned q) {
    if (x_ext < 0 || q == 0)
        return;

    // Resolve horizontal axis overlaps: Bottom (1, 2) overrides Top (4, 8)
    if (oy == 0) {
        if (q & 1)
            q &= ~4;
        if (q & 2)
            q &= ~8;
    }

    if (q & 3) {  // Bottom half
        int x0 = (q & 2) ? (xc - x_ext) : xc;
        int x1 = (q & 1) ? (xc + x_ext) : xc;
        draw_hline(x0, x1, yc + oy, value);
    }
    if (q & 12) {  // Top half
        int x0 = (q & 8) ? (xc - x_ext) : xc;
        int x1 = (q & 4) ? (xc + x_ext) : xc;
        draw_hline(x0, x1, yc - oy, value);
    }
}

// Monochrome ellipses. Filled or not filled. By quadrant q.
static inline void
ellipse_ll_mono(int xc, int yc, int rx, int ry, unsigned q, uint8_t value, bool do_fill) {
    int32_t rx2 = rx * rx, ry2 = ry * ry;
    int32_t tworx2 = rx2 << 1, twory2 = ry2 << 1;
    int32_t x = 0, y = ry;
    int32_t px = 0, py = tworx2 * y;
    int32_t err = ry2 - (rx2 * y) + (rx2 >> 2);

    while (px <= py) {
        if (!do_fill)
            plot4(xc, yc, x, y, value, q);
        int cur_x = x;
        x++;
        px += twory2;
        if (err < 0) {
            err += ry2 + px;
        } else {
            if (do_fill)
                fill_row(xc, yc, cur_x, y, value, q);
            y--;
            py -= tworx2;
            err += ry2 + px - py;
        }
    }

    err = ry2 * x * (x + 1) + (ry2 >> 2) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;

    while (y >= 0) {
        if (do_fill)
            fill_row(xc, yc, x, y, value, q);
        else
            plot4(xc, yc, x, y, value, q);

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

// Anti aliased ellipses. Filled or not filled. By quadrant q.
static inline void
ellipse_ll_aa(int xc, int yc, int rx, int ry, unsigned q, uint8_t value, bool do_fill) {
    int32_t a2 = rx * rx, b2 = ry * ry;
    int32_t x = 0, y = ry;
    int32_t px = 0, py = 2 * a2 * y;
    int32_t e = 0;
    int last_y = -1;

    while (px <= py) {
        int w = (e * value) / py;
        plot4(xc, yc, x, y, value - w, q);  // Outer pixel (Used by BOTH)
        if (!do_fill)
            plot4(xc, yc, x, y - 1, w, q);  // Inner pixel (Outline only)

        int cur_x = x;
        x++;
        e += px + b2;
        px += 2 * b2;

        if (e >= py - a2) {
            if (do_fill && last_y != y - 1) {
                fill_row(xc, yc, cur_x, y - 1, value, q);  // Solid core (Fill only)
                last_y = y - 1;
            }
            y--;
            e -= py - a2;
            py -= 2 * a2;
        }
    }

    while (y >= 0) {
        int w = (e * value) / px;
        plot4(xc, yc, x, y, value - w, q);  // Outer pixel (Used by BOTH)

        if (!do_fill) {
            plot4(xc, yc, x - 1, y, w, q);  // Inner pixel (Outline only)
        } else if (x > 0 && last_y != y) {
            fill_row(xc, yc, x - 1, y, value, q);  // Solid core (Fill only)
            last_y = y;
        }

        if (y == 0)
            break;

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

void draw_ellipse(int xc, int yc, int rx, int ry, unsigned q, uint8_t value) {
    if (rx == 0 && ry == 0) {
        if (q)
            plot4(xc, yc, 0, 0, value, q);
        return;
    }
    if (rx == 0 || ry == 0)
        return;

#if FB_BPP == 1
    ellipse_ll_mono(xc, yc, rx, ry, q, value, false);
#else
    ellipse_ll_aa(xc, yc, rx, ry, q, value, false);
#endif
}

void fill_ellipse(int xc, int yc, int rx, int ry, unsigned q, uint8_t value) {
    if (rx == 0 && ry == 0) {
        if (q)
            plot4(xc, yc, 0, 0, value, q);
        return;
    }
    if (rx == 0 || ry == 0)
        return;

#if FB_BPP == 1
    ellipse_ll_mono(xc, yc, rx, ry, q, value, true);
#else
    ellipse_ll_aa(xc, yc, rx, ry, q, value, true);
#endif
}
