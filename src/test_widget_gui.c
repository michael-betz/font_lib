#include "font.h"
#include "frame_buffer.h"
#include "graphics.h"
#include "widget_gui.h"
#include <SDL2/SDL.h>
#include <stdio.h>

extern const font_header_t f_fixed;

unsigned event_flags = 0;

void on_event_widget_gui(SDL_Event *e) {
    if (e->type != SDL_KEYDOWN)
        return;
    // left / right = encoder rotation
    if (e->key.keysym.sym == SDLK_RIGHT) {
        event_flags |= EV_ROT_CW;
    } else if (e->key.keysym.sym == SDLK_LEFT) {
        event_flags |= EV_ROT_CCW;
    } else if (e->key.keysym.sym == SDLK_UP) {
        event_flags |= EV_ENC_S;
    } else if (e->key.keysym.sym == SDLK_DOWN) {
        event_flags |= EV_BACK_S;
    }
}

unsigned get_event_flags(void) {
    unsigned tmp = event_flags;
    event_flags = 0;
    return tmp;
}

// --- Page 1: Settings ---
static widget_t settings_widgets[] = {
    // id, bounds, can_focus, draw_cb, event_cb, data pointer
    {1, {10, 32, 20, 32}, false, draw_simple_static_label, NULL, "Hello"},
    {2, {64, 128, 20, 32}, false, draw_simple_static_label, NULL, "World"},
};

// --- Page 2: Network ---
static widget_t network_widgets[] = {
    // id, bounds, can_focus, draw_cb, event_cb, data pointer
    {4, {10, 246, 20, 32}, false, draw_simple_static_label, NULL, "IP: 192.168.1.50"},
    {5, {10, 100, 36, 48}, false, draw_simple_static_label, NULL, "Reconnect"},
};

// --- The Pages Array ---
static page_t my_pages[] = {
    // tab_name, widgets*, num_widgets, focused_index
    {"Settings", settings_widgets, 2, 0},
    {"Network", network_widgets, 2, 0},
};

// --- The Global GUI ---
static gui_t gui = {
    .pages = my_pages,
    .num_pages = 2,
    .active_page = 0,
    .state = NAV_TABS,
};

void test_widget_gui(void) {
    static unsigned frame = 0;
    if (frame == 0) {
        set_gui(&gui);
        fnt_init_from_header(&f_fixed);
    }
    widget_gui_update(frame == 0);
    frame++;
}
