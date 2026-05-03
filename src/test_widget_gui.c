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

// --- Label Widget ---
void draw_label(widget_t *self, bool is_focused, bool is_editing) {
    // Cast the generic data pointer to our specific label state
    char *text = (char *)self->data;

    // Use the font pointer to swap fonts before drawing!
    fnt_init_from_header(&f_fixed);

    // Draw the text using the parameters
    int center_y = self->bounds.top + ((self->bounds.bottom - self->bounds.top) / 2);
    int center_x = self->bounds.left + ((self->bounds.right - self->bounds.left) / 2);

    fnt_draw_text(center_x, center_y, text, 64, H_MIDDLE | V_MIDDLE);
}

// --- Page 1: Settings ---
static widget_t settings_widgets[] = {
    // id, bounds, can_focus, draw_cb, event_cb, data pointer
    {1, {10, 200, 20, 32}, false, draw_label, NULL, "Hello"},
    {2, {10, 200, 36, 48}, false, draw_label, NULL, "World"},
};

// --- Page 2: Network ---
static widget_t network_widgets[] = {
    // id, bounds, can_focus, draw_cb, event_cb, data pointer
    {4, {10, 246, 20, 32}, false, draw_label, NULL, "IP: 192.168.1.50"},
    {5, {10, 100, 36, 48}, false, draw_label, NULL, "Reconnect"},
};

// --- The Pages Array ---
static page_t my_pages[] = {
    {"Settings", settings_widgets, 2, 0},  // 3 widgets, start with index 0 focused
    {"Network", network_widgets, 2, 0},
};

// --- The Global GUI ---
static gui_t gui = {
    .pages = my_pages,
    .num_pages = 2,
    .active_page = 0,
    .state = STATE_NAV_TABS  // Start by letting the user choose a tab
};

void test_widget_gui(void) {
    static unsigned frame = 0;
    if (frame == 0)
        set_gui(&gui);
    widget_gui_update();
    frame++;
}
