#include "ssd1322.h"
#include "frame_buffer.h"
#include "hardware_interface.h"
#include <stdbool.h>
#include <stdint.h>

// Initialization for SSD1322 based OLED display
// the dc_vector contains the state of the DC pin for each init-byte. LSB first.
static const uint64_t dc_vector = 0x1d528000a952aa4d;
// clang-format off
static const uint8_t init[] = {
    0xFD, 0x12,     // Unlock OLED driver IC
    0xAE,           // Display OFF (blank)
    0x15, 0x1C, 0x5B,  // Set column address to 1C, 5B
    0x75, 0x00, 0x3F,  // Set row address to 00, 3F
    0xB3, 0x91,     // set clock to 80 fps
    0xCA, 0x3F,     // Multiplex ratio, 1/64, 64 COMS enabled
    0xA2, 0x00,     // Set offset, the display map starting line is COM0
    0xA1, 0x00,     // Set start line position
    0xA0, 0x14, 0x11,   // Set remap, horiz address increment, disable colum address remap,
                     //  enable nibble remap, scan from com[N-1] to COM0, disable COM split odd even
    0xB5, 0x00,     // Disable GPIO inputs
    0xAB, 0x01,     // Select external VDD
    0xB4, 0xA0, 0xB5,   // Display enhancement A, 0xA0: external VSL, 0xA2: internal VSL, 0xB5: normal, 0xFD: enhanced low GS
    0xC1, 0x7F,     // Contrast current, 256 steps, default is 0x7F
    0xC7, 0x08,     // Master contrast current (brightness), 16 steps, default is 0x0F
    // Load custom gamma table
    0xB8, 0, 1, 4, 8, 15, 23, 33, 45, 59, 74, 92, 111, 132, 155, 180,
    0xB1, 0xF4,     // Reset period / first pre-charge period Length
    0xD1, 0xa2, 0x20,    // Display enhancement B
    0xBB, 0x17,     // Pre-charge voltage
    0xB6, 0x08,     // Second pre-charge period = 8 clks
    0xBE, 0x04,     // VCOMH: Set Common Pins Deselect Voltage Level as 0.8 * VCC
    0xA6,           // Normal display
    0xA9,           // Disable partial display mode
    0xAF            // Display ON
};
// clang-format on

// Write a 8 bit register
static void send_data(uint8_t val) { ui_spi_rx_tx(val); }

static void send_cmd(uint8_t val) {
    ui_set_dc(0);
    ui_spi_rx_tx(val);
    ui_set_dc(1);
}

void init_ssd1322(void) {
    ui_set_cs_n(SELECT_OLED);
    ui_set_dc(1);
    for (unsigned i = 0; i < sizeof(init); i++) {
        if ((dc_vector >> i) & 1)
            send_cmd(init[i]);
        else
            send_data(init[i]);
    }
    send_cmd(0x00);  // enable custom gamma table
    ui_set_cs_n(SELECT_NONE);
}

void set_brightness(uint8_t val) {
    while (ui_spi_is_busy())
        ;

    ui_set_cs_n(SELECT_OLED);
    if (val == 0) {
        send_cmd(0xAE);  // display off
    } else {
        send_cmd(0xAF);  // display on
        send_cmd(0xC7);  // set brightness (0 - 15)
        send_data(val - 1);
    }
    ui_set_cs_n(SELECT_NONE);
}

void set_inverted(bool val) {
    while (ui_spi_is_busy())
        ;

    ui_set_cs_n(SELECT_OLED);
    send_cmd(val ? 0xA7 : 0xA6);
    ui_set_cs_n(SELECT_NONE);
}

typedef struct {
    int x1;  // region of interest
    int y1;
    int x2;
    int y2;
    int n_rows;   // how many rows are left to transmit
    int n_bytes;  // how many bytes per row
} t_send_state;

static t_send_state tx_state = {0};

void send_window_4(int x1, int y1, int x2, int y2) {
    // truncate the 2 LSBs for X because 1 addressable column contains 4 pixels
    x1 >>= 2;
    x2 >>= 2;

    // Which region of the screen is redrawn
    // D("send_window_4(%3d, %3d, %3d, %3d)\n", x1 << 2, y1, x2 << 2, y2);

    // Start a new transfer ... after a quick sanity check
    if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0 || x1 > x2 || y1 > y2) {
        // D("window invalid. Returning.\n");
        return;
    }

    tx_state.y1 = y1;  // Store the state
    tx_state.y2 = y2;
    tx_state.x1 = x1;
    tx_state.x2 = x2;
    tx_state.n_rows = y2 - y1 + 1;
    tx_state.n_bytes = (x2 - x1 + 1) * 2;  // [bytes]

    // Setup the SSD1322 addressing window
    ui_set_cs_n(SELECT_OLED);
    send_cmd(0x15);  // Set column address range
    send_data(0x1C + x1);
    send_data(0x1C + x2);

    send_cmd(0x75);  // Set row address range
    send_data(y1);
    send_data(y2);
    send_cmd(0x5C);  // write VRAM
}

bool send_row() {
    // Check if we are done
    if (tx_state.n_rows <= 0) {
        ui_set_cs_n(SELECT_NONE);
        return true;
    }

    // Send a single row to the OLED
    uint8_t *p = &framebuffer[tx_state.y1 * FB_WIDTH / 2 + tx_state.x1 * 2];
    ui_set_cs_n(SELECT_OLED);
    ui_spi_tx_chunk(p, tx_state.n_bytes);  // initiate a new DMA transfer
    tx_state.n_rows--;
    tx_state.y1++;

    return false;
}
