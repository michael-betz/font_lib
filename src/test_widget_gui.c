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

// --- 1. Slide 1: Sensor Dashboard (Pure Display) ---
// Example sensor callback
static void get_temp_cb(char *buf) { sprintf(buf, "Temp: %d C", frame % 150); }

static void button_cb(const Widget *w, uint32_t ev) {
    if (ev & EV_ENC_S)
        printf("BUTTON event!!!\n");
}

bool state = false;
const Widget *const slide1_widgets[] = {
    &(Widget)WIDGET_LABEL(128, 2, "Engine Bay Status", H_MIDDLE | V_TOP),
    &(Widget)WIDGET_DYNLBL(32, 20, get_temp_cb, H_LEFT | V_TOP),
    &(Widget)WIDGET_CHECK_BOX(16, 48, "Power enabled", &state),
    &(Widget)WIDGET_BUTTON(130, 48, "Push me!", button_cb),
};

const Screen slide1 = {slide1_widgets, 4};

// --- 2. Slide 2: Interactive Settings ---
int fan_speed = 50, heater = 50;
const Widget *const slide2_widgets[] = {
    &(Widget)WIDGET_LABEL(128, 2, "Fan Controls", H_MIDDLE | V_TOP),
    &(Widget)WIDGET_SETTING(32, 20, "Speed", &fan_speed, 0, 100, 10),
    &(Widget)WIDGET_SETTING(128, 20, "Heater", &heater, -50, 50, 5),
};
const Screen slide2 = {slide2_widgets, 3};

// --- 3. Main Array & Execution ---
const Screen *my_slides[] = {&slide1, &slide2};

void test_widget_gui(void) {
    if (frame == 0) {
        draw_rectangle(0, 0, 256, 64, 0x88);
        fnt_init_from_header(&f_fixed);
        gui_init(my_slides, 2);
    }
    set_draw_mode(DRAW_SET);
    fill_rectangle(0, 0, 255, 63, 0);
    gui_draw(frame == 0 || true);
    frame++;
}
