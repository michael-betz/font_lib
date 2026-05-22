#include "font.h"
#include "fonts.h"
#include "frame_buffer.h"
#include "graphics.h"
#include "widget_gui.h"
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
static void get_temp_cb(char *buf) { sprintf(buf, "Temp: %d C", frame % 150); }

static void button_cb(const Widget *w, uint32_t ev) {
    if (ev & EV_ENC_S)
        printf("BUTTON event!!!\n");
}

static bool state = false;
static const Widget *const slide1_widgets[] = {
    WIDGET_LABEL(128,
                 0,
                 "\x11"
                 "Test1\x10",
                 H_MIDDLE | V_TOP),
    WIDGET_DYNLBL(32, 20, get_temp_cb, H_LEFT | V_TOP),
    WIDGET_CHECK_BOX(16, 48, "Power enabled", &state),
    WIDGET_BUTTON(130, 48, "Push me!", button_cb),
};

static const Screen slide1 = {slide1_widgets, 4};

// ------------------
//  Second slide
// ------------------
int fan_speed = 50, heater = 50;
static const Widget *const slide2_widgets[] = {
    WIDGET_LABEL(128,
                 2,
                 "\x11"
                 "Fan Controls\x10",
                 H_MIDDLE | V_TOP),
    WIDGET_SETTING(32, 48, "Speed", &fan_speed, 0, 100, 10),
    WIDGET_SETTING(128, 48, "Heater", &heater, -50, 50, 5),
};
static const Screen slide2 = {slide2_widgets, 3};

static const Screen *my_slides[] = {&slide1, &slide2};

void test_widget_gui(void) {
    if (frame == 0) {
        draw_rectangle(0, 0, 256, 64, 0x88);
        fnt_init_from_header(&f_fixed);
        static const font_header_t *fnt_table[2] = {&f_fixed, &f_vollkorn};
        fnt_set_table(fnt_table, 2);
        gui_init(my_slides, 2);
    }
    set_draw_mode(DRAW_SET);
    fill_rectangle(0, 0, 255, 63, 0);
    gui_draw(frame == 0 || true);
    frame++;
}
