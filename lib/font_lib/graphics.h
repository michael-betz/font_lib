#pragma once
#include <stdint.h>

// Graphics primitives

// Draw a line from (x0, y0) to (x1, y1)
// anti-aliased if FB_BPP > 1
void draw_line(int x0, int y0, int x1, int y1);

// Draw a rectangle from (x0, y0) to (x1, y1)
void draw_rectangle(int x0, int y0, int x1, int y1, uint8_t value);
void fill_rectangle(int x0, int y0, int x1, int y1, uint8_t value);

// Draw a rectangle centered at xc, yc with width w and height h
void draw_rectangle_c(int xc, int yc, int w, int h, uint8_t value);
void fill_rectangle_c(int xc, int yc, int w, int h, uint8_t value);

// Draw a rectangle from (x0, y0) to (x1, y1) with corner radius r
void draw_rectangle_r(int x0, int y0, int x1, int y1, int r, uint8_t value);
void fill_rectangle_r(int x0, int y0, int x1, int y1, int r, uint8_t value);

void draw_rectangle_rc(int xc, int yc, int w, int h, int r, uint8_t value);
void fill_rectangle_rc(int xc, int yc, int w, int h, int r, uint8_t value);

// Draw an ellipse outline centered at (xc, yc) with radius (rx, ry)
// anti-aliased if FB_BPP > 1
// The 4 bits in q define which quadrant to draw, set to 0xF to get the full ellipse
// Quadrant map: Bit 0=BR, 1=BL, 2=TR, 3=TL
void draw_ellipse(int xc, int yc, int rx, int ry, unsigned q, uint8_t val);
void fill_ellipse(int xc, int yc, int rx, int ry, unsigned q, uint8_t val);
