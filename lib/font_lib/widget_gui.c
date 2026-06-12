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
    if (ev & (EV_BACK_S | EV_ENC_L)) {
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
        // Push enters edit mode (if widget is editable. Button and checkbox isnt).
        if ((ev & EV_ENC_S) && w && w->event) {
            if (w->editable) {
                mode = MODE_EDIT;
                printf("MODE_EDIT\n");
            } else {
                w->event(w, ev);
            }
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
        w->draw(w, state, ev);
    }

    // Draw a tiny slide indicator at the left (e.g. dots)
    // if (mode != MODE_SLIDE)
    //     return;
    int dot_w = slide_count * 8;
    int start_y = 32 - (dot_w / 2);
    draw_rectangle_r(
        -6, start_y - 8, 8, start_y + slide_count * 8, 5, mode == MODE_SLIDE ? 0xFF : 0x20);
    for (int i = 0; i < slide_count; i++) {
        if (i == cur_slide)
            fill_ellipse(3, start_y + (i * 8), 2, 2, 0xF, mode == MODE_SLIDE ? 0xFF : 0x20);
        else
            draw_ellipse(3, start_y + (i * 8), 1, 1, 0xF, mode == MODE_SLIDE ? 0x80 : 0x20);
    }
}
