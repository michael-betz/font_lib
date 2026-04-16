#pragma once

// Graphics primitives

// Draw a monochrome line from (x0, y0) to (x1, y1)
void draw_line(int x0, int y0, int x1, int y1);

// same as draw_line() but anti-aliased if FB_BPP > 1
void draw_line_aa(int x0, int y0, int x1, int y1);

// Draw a monochrome ellipse centered at (xc, yc) with radius (rx, ry)
// The 4 bits in q define which quadrant to draw, set to 0xF to get the full ellipse
//     0b0001: top right
//     0b0010: top left
//     0b0100: bottom right
//     0b1000: bottom left
void draw_ellipse(int xc, int yc, int rx, int ry, unsigned q);

// same as draw_ellipse() but anti-aliased if FB_BPP > 1
void draw_ellipse_aa(int xc, int yc, int rx, int ry, unsigned q);
