#include "widget_gui.h"
#include "font.h"
#include "frame_buffer.h"
#include "graphics.h"
#include <stddef.h>
#include <stdio.h>

// GUI State Machine
typedef enum { MODE_SLIDE, MODE_FOCUS, MODE_EDIT } gui_mode_t;

// pointer to a ROM array of ROM Screen pointers
static const Screen *const *slides;
static uint8_t slide_count = 0;
static uint8_t cur_slide = 0;
static uint8_t cur_focus = 0;
static gui_mode_t mode = MODE_SLIDE;

void gui_init(const Screen *const *screens, uint8_t num_screens) {
    slides = screens;
    slide_count = num_screens;
    cur_slide = 0;
    mode = MODE_SLIDE;
}

// Helper to find the next interactive widget on a screen
static void step_focus(int dir) {
    const Screen *s = slides[cur_slide];
    if (!s || s->count == 0)
        return;
    for (int i = 0; i < s->count; i++) {
        cur_focus = (cur_focus + dir + s->count) % s->count;
        if (s->widgets[cur_focus]->selectable)
            return;  // Found one
        if (dir == 0)
            dir = 1;
    }
    printf("step_focus(%d): no selectable widget found on slide %d\n", dir, cur_slide);
}

void gui_draw(bool force_draw) {
    if (slide_count == 0)
        return;

    unsigned ev = get_event_flags();

    if (ev == 0 && force_draw == false)
        return;

    const Screen *s = slides[cur_slide];
    const Widget *w = (s->count > 0) ? s->widgets[cur_focus] : NULL;

    // Handle BACK button globally
    if (ev & EV_BACK_S) {
        if (mode == MODE_EDIT) {
            mode = MODE_FOCUS;
            printf("MODE_FOCUS\n");
        } else if (mode == MODE_FOCUS) {
            mode = MODE_SLIDE;
            printf("MODE_SLIDE\n");
        }
    }

    switch (mode) {
    case MODE_SLIDE:
        // Rotate changes slides.
        if (ev & EV_ROT_CW)
            cur_slide = (cur_slide + 1) % slide_count;
        if (ev & EV_ROT_CCW)
            cur_slide = (cur_slide - 1 + slide_count) % slide_count;
        // Push enters focus mode (if slide has selectable widgets).
        if (ev & EV_ENC_S) {
            cur_focus = 0;
            step_focus(0);  // Find first selectable
            if (s->widgets[cur_focus]->selectable) {
                mode = MODE_FOCUS;
                printf("MODE_FOCUS\n");
            }
        }
        break;

    case MODE_FOCUS:
        // Rotate moves between widgets.
        if (ev & EV_ROT_CW)
            step_focus(1);
        if (ev & EV_ROT_CCW)
            step_focus(-1);
        // Push enters edit mode (if widget has an event handler).
        if ((ev & EV_ENC_S) && w && w->event) {
            mode = MODE_EDIT;
            printf("MODE_EDIT\n");
        }
        break;

    case MODE_EDIT:
        // Push confirms edit and returns to focus.
        if (ev & EV_ENC_S) {
            mode = MODE_FOCUS;
            printf("MODE_FOCUS\n");
        }
        // Otherwise, route events to the widget.
        else if (w && w->event)
            w->event(w, ev);
        break;
    }

    // Draw all widgets on the screen
    fill_rectangle(0, 0, 255, 63, 0);
    s = slides[cur_slide];
    for (uint8_t i = 0; i < s->count; i++) {
        w = s->widgets[i];
        if (!w->draw)
            continue;

        w_state_t state = W_NORMAL;
        if (mode != MODE_SLIDE && i == cur_focus) {
            state = (mode == MODE_EDIT) ? W_EDITING : W_FOCUSED;
        }
        w->draw(w, state);
    }

    // Optional: Draw a tiny slide indicator at the left (e.g. dots)
    int dot_w = slide_count * 8;
    int start_y = 32 - (dot_w / 2);
    for (int i = 0; i < slide_count; i++) {
        if (i == cur_slide)
            fill_ellipse(4, start_y + (i * 8), 2, 2, 0xF, 0xFF);
        else
            draw_ellipse(4, start_y + (i * 8), 2, 2, 0xF, 0x80);
    }
}

// TODO: smart (formatted) labels using escape characters
void draw_static_label(const Widget *w, w_state_t state) {
    const LblData *d = (const LblData *)w->data;
    // state is ignored because it's never focused
    fnt_draw_text(w->x, w->y, d->text, 32, d->align);
}

void draw_dyn_label(const Widget *w, w_state_t state) {
    const DynLblData *d = (const DynLblData *)w->data;
    char buffer[32] = {0};
    d->format_value(buffer);  // e.g. formats "24.5 °C" into buffer
    fnt_draw_text(w->x, w->y, buffer, sizeof(buffer), d->align);
}

void draw_setting(const Widget *w, w_state_t state) {
    const SettingData *d = (const SettingData *)w->data;

    uint8_t color = (state == W_EDITING) ? 0xFF : (state == W_FOCUSED) ? 0x88 : 0x44;

    // Draw a nice rounded bounding box
    draw_rectangle_r(w->x, w->y, w->x + 100, w->y + 16, 3, color);

    // Draw the text inside
    fnt_draw_printf(w->x + 4, w->y + 2, H_LEFT | V_TOP, "%s: %d", d->label, *d->value);
}

void event_setting(const Widget *w, uint32_t ev) {
    const SettingData *d = (const SettingData *)w->data;

    // We are modifying the RAM value that the ROM pointer points to.
    // This is entirely valid standard C!
    if (ev & EV_ROT_CW)
        *d->value += d->step;
    if (ev & EV_ROT_CCW)
        *d->value -= d->step;

    // Clamp values
    if (*d->value > d->max)
        *d->value = d->max;
    if (*d->value < d->min)
        *d->value = d->min;
}
