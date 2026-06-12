#include "font.h"
#include "fonts.h"
#include "frame_buffer.h"
#include "graphics.h"
#include "print.h"
#include "widgets.h"
#include <SDL2/SDL.h>
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
static void get_temp_cb(char *buf) { sprintf(buf, "\x10Temp:\x11 %d\x10°C", frame % 150); }

static void button_cb(const Widget *w, uint32_t ev) {
    if (ev & EV_ENC_S)
        printf("BUTTON event!!!\n");
}

static bool state = false;
static const Widget *const slide1_widgets[] = {
    WIDGET_LABEL(128, 0, "\x16Test1\x10", H_MIDDLE | V_TOP),
    WIDGET_DYNLBL(32, 20, get_temp_cb, H_LEFT | V_TOP),
    WIDGET_CHECK_BOX(32, 48, "\x12Power enabled", &state),
    WIDGET_BUTTON(148, 48, "\x11Push me!", button_cb),
};

static const Screen slide1 = {slide1_widgets, 4};

// ------------------
//  Second slide
// ------------------
int fan_speed = 50, heater = 50;
static const Widget *const slide2_widgets[] = {
    WIDGET_LABEL(128, 2, "\x16 Fan Controls", H_MIDDLE | V_TOP),
    WIDGET_SETTING(32, 48, "\x11Speed\x15", &fan_speed, 0, 100, 10),
    WIDGET_SETTING(128, 48, "\x11Heater\x15", &heater, -50, 50, 5),
};
static const Screen slide2 = {slide2_widgets, 3};

// ------------------
//  Third slide
// ------------------
const char *const l_labels[] = {"Inlet", "Outlet", "Diff"};
int p_values[] = {1253, 124, 0};
int t_values[] = {2421, 2580, 0};
void format_pressure(char *buffer, int val) { dec_dp(val, 5, 0, buffer); }
void format_temperature(char *buffer, int val) { dec_dp(val, 5, 2, buffer); }

static const Widget *const slide3_widgets[] = {
    WIDGET_TABLE_VIEW(125,
                      14,
                      format_pressure,
                      3,
                      14,
                      "\x10P [mbar]",
                      l_labels,
                      p_values,
                      &f_monogram,
                      &f_fixed_b),
    WIDGET_TABLE_VIEW(
        200, 14, format_temperature, 3, 14, "\x10T [°C]", NULL, t_values, NULL, &f_fixed_b)};
static const Screen slide3 = {slide3_widgets, 2};

static const Screen *my_slides[] = {&slide1, &slide2, &slide3};

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
        gui_init(my_slides, 3);
    }
    set_draw_mode(DRAW_SET);
    fill_rectangle(0, 0, 255, 63, 0);
    gui_draw(frame == 0 || true);
    p_values[0] += 1;
    p_values[1] -= 1;
    p_values[2] = p_values[0] - p_values[1];
    t_values[0] -= 1;
    t_values[1] += 1;
    t_values[2] = t_values[0] - t_values[1];

    frame++;
}
