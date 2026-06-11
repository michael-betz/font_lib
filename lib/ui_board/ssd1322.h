#pragma once
#include <stdbool.h>
#include <stdint.h>

void init_ssd1322(void);

// set OLED brightness (0 = off, 1 - 16 = on)
void set_brightness(uint8_t val);

// invert the display
void set_inverted(bool val);

// send a certain rectangular window of the framebuffer to the display.
// Non blocking, this just sets up the transmisison, call send_row() to transmit image data.
// x1, y1, x2, y2: the rectangle to update in [pixels]
// x1, y1, x2 and y2 are all inclusive!
// note that ssd1322 works with columns of 4 pixels horizontally
// so the lower 2 bits of x1 and x2 will be truncated
// data in 4 bits / pixel, 2 pixels / byte
// before calling this, make sure the SPI peripheral is IDLE with ui_spi_is_busy() == false
void send_window_4(int x1, int y1, int x2, int y2);

// like above but instead of a window, send the complete framebuffer
#define send_fb() send_window_4(0, 0, FB_WIDTH - 1, FB_HEIGHT - 1)

// After setting up a window transfer above,
// call this in a loop to send 1 row at a time.
// Only call this when ui_spi_is_busy() returns false
// When all rows were sent, returns true.
bool send_row();
