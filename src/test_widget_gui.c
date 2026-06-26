#include "font.h"
#include "fonts.h"
#include "frame_buffer.h"
#include "graphics.h"
#include "print.h"
#include "widgets.h"
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>

extern const font_header_t f_fixed;

static unsigned event_flags = 0, frame = 0;

void on_event_widget_gui(SDL_Event *e) {
    if (e->type == SDL_KEYUP) {
        if (e->key.keysym.sym == SDLK_DOWN) {
            if (event_flags & EV_ENC) {
                event_flags |= EV_ENC_S;
            }
            event_flags &= ~EV_ENC;
        }
    }

    if (e->type != SDL_KEYDOWN)
        return;

    // left / right = encoder rotation
    if (e->key.keysym.sym == SDLK_RIGHT) {
        event_flags |= EV_ROT_CW;
    } else if (e->key.keysym.sym == SDLK_LEFT) {
        event_flags |= EV_ROT_CCW;
    } else if (e->key.keysym.sym == SDLK_UP) {
        event_flags |= EV_BACK_S;
    } else if (e->key.keysym.sym == SDLK_DOWN) {
        event_flags |= EV_ENC;
    }
}

unsigned get_event_flags(void) {
    unsigned tmp = event_flags;
    event_flags &= EV_ENC;
    return tmp;
}

// ------------------
//  First slide
// ------------------
// Example sensor callback
static void get_temp_cb(char *buf) { sprintf(buf, "\x10Temp: %d °C", frame % 150); }

static void button_cb(const Widget *w, uint32_t ev) {
    if (ev & EV_ENC_S)
        printf("BUTTON event!!!\n");
}

static bool state = false;
static const Widget *const slide1_widgets[] = {
    W_LABEL(64, 2, -1, -1, "\x16Test1", NULL, H_MIDDLE | V_TOP),
    W_LABEL(64, 29, 150, 48, NULL, get_temp_cb, H_RIGHT | V_BOTTOM),
    W_CHECK_BOX(140, 12, "\x15LED", &state),
    W_BUTTON(200, 4, 251, 4 + 13, "\024Red", button_cb),
    W_BUTTON(200, 25, 251, 25 + 13, "\024Green", button_cb),
    W_BUTTON(200, 46, 251, 46 + 13, "\024Blue", button_cb),
};

static const Screen slide1 = {slide1_widgets, 6};

// ------------------
//  Second slide
// ------------------
int fan_speed = 50, heater = 50;
static const Widget *const slide2_widgets[] = {
    W_LABEL(128, 2, -1, -1, "\x16 Fan Controls", NULL, H_MIDDLE | V_TOP),
    W_SETTING(32, 48, "\x11Speed\x15", &fan_speed, 0, 100, 10),
    W_SETTING(128, 48, "\x11Heater\x15", &heater, -50, 50, 5),
};
static const Screen slide2 = {slide2_widgets, 3};

// ------------------
//  Third slide
// ------------------
int p_values[] = {1253, 124, 12, 24, 1};
int t_values[] = {100, 2580, 13, 25, 8};

#define N_HISTORY 200
static int16_t t_history[N_HISTORY] = {0};
static unsigned t_history_wp = 0;

// return true if the cell should be redrawn (dynamic value)
// return false if the cell is a static label
static bool cell(int row, int col, char *buffer, const int buffer_size) {
    const char *const l_labels[] = {"\020Inlet", "\020Outlet", "\020Diff", "\020Mult", "\020Bla"};
    switch (col) {
    case 0:
        if (row > 0)
            strncpy(buffer, l_labels[row - 1], buffer_size);
        return false;
    case 1:
        if (row > 0) {
            dec_dp(p_values[row - 1], 5, 0, buffer);
            return true;
        }
        strncpy(buffer, "\x10P [mbar]\x11", buffer_size);
        return false;
    case 2:
        if (row > 0) {
            dec_dp(t_values[row - 1], 5, 2, buffer);
            return true;
        }
        strncpy(buffer, "\x10T [°C]\x11", buffer_size);
        return false;
    }
}

int scroll_pos = 0;
static const Widget *const slide3_widgets[] = {
    W_GRID_VIEW(20, 14, cell, 6, 3, 15, 54, &scroll_pos),
    W_V_SCROLL(245, 5, 58, 8, &scroll_pos, 0, 3),
};
static const Screen slide3 = {slide3_widgets, 2};

static const Widget *const slide4_widgets[] = {
    W_TREND_VIEW(20, 4, 250, 60, 0, N_HISTORY, t_history, &t_history_wp),
};
static const Screen slide4 = {slide4_widgets, 1};

static const Screen *my_slides[] = {&slide1, &slide2, &slide3, &slide4};

void test_widget_gui(void) {
    if (frame == 0) {
        draw_rectangle(0, 0, 256, 64, 0x88);
        fnt_init_from_header(&f_fixed);
        static const font_header_t *fnt_table[] = {
            &f_fixed,     // 0
            &f_fixed_b,   // 1
            &f_fixed_o,   // 2
            &f_monogram,  // 3
            &f_ubuntu,    // 4
            &f_ubuntu_b,  // 5
            &f_vollkorn,  // 6
        };
        fnt_set_table(fnt_table, 7);
        gui_init(my_slides, 4);
    }
    set_draw_mode(DRAW_SET);
    gui_draw();

    set_draw_mode(DRAW_SET);
    draw_rectangle(-1, -1, 256, 64, 0x80);

    p_values[0] += 1;
    p_values[1] -= 1;
    p_values[2] = p_values[0] - p_values[1];

    for (int i = 0; i < 2; i++)
        t_values[i] += random() & 1 ? 1 : -1;
    t_values[2] = t_values[0] - t_values[1];

    // if (frame < 100) {
    t_history[t_history_wp] = t_values[0];
    t_history_wp = (t_history_wp + 1) % N_HISTORY;
    // }

    frame++;
}
