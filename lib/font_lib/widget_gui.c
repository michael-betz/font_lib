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
        -6, start_y - 8, 8, start_y + slide_count * 8, 5, mode == MODE_SLIDE ? 0xFF : 0x30);
    for (int i = 0; i < slide_count; i++) {
        if (i == cur_slide)
            fill_ellipse(3, start_y + (i * 8), 2, 2, 0xF, mode == MODE_SLIDE ? 0xFF : 0x30);
        else
            draw_ellipse(3, start_y + (i * 8), 1, 1, 0xF, mode == MODE_SLIDE ? 0x80 : 0x30);
    }
}

// TODO: smart (formatted) labels using escape characters
void draw_static_label(const Widget *w, w_state_t state, unsigned event_flags) {
    const LblData *d = (const LblData *)w->data;
    // state is ignored because it's never focused
    fnt_draw_text(w->x, w->y, d->text, 32, d->align);
}

void draw_dyn_label(const Widget *w, w_state_t state, unsigned event_flags) {
    const DynLblData *d = (const DynLblData *)w->data;
    char buffer[32] = {0};
    d->format_value(buffer);  // e.g. formats "24.5 °C" into buffer
    fnt_draw_text(w->x, w->y, buffer, sizeof(buffer), d->align);
}

void draw_button(const Widget *w, w_state_t state, unsigned event_flags) {
    const LblData *d = (const LblData *)w->data;
    int y = w->y - 1;
    // Move button down when encoder is pressed down
    if (state == W_FOCUSED && event_flags & 1) {
        y += 1;
    }
    bbox_t bb = fnt_draw_text(w->x + 10, y, d->text, 32, H_LEFT | V_MIDDLE);

    if (state == W_FOCUSED) {
        set_draw_mode(DRAW_INV);
        fill_rectangle_rbb(bb_add_spacing(bb, 4), 4, 0xFF);
    } else {
        set_draw_mode(DRAW_ADD);
        fill_rectangle_rbb(bb_add_spacing(bb, 4), 4, 0x44);
    }

    set_draw_mode(DRAW_SET);
}

void draw_check_box(const Widget *w, w_state_t state, unsigned event_flags) {
    const CheckBoxData *d = (const CheckBoxData *)w->data;
    int y = w->y - 1;
    // Move checkobox down when encoder is pressed down
    if (state == W_FOCUSED && event_flags & 1)
        y += 1;
    draw_rectangle_c(w->x, y, 8, 8, 0xFF);
    if (*d->is_enabled)
        fill_rectangle_c(w->x, y, 5, 5, 0xFF);
    bbox_t bb = fnt_draw_text(w->x + 10, w->y, d->text, 32, H_LEFT | V_MIDDLE);
    bb.left -= 14;
    if (state == W_FOCUSED)
        draw_rectangle_rbb(bb_add_spacing(bb, 4), 4, 0x88);
}
void event_check_box(const Widget *w, uint32_t ev) {
    const CheckBoxData *d = (const CheckBoxData *)w->data;
    if (ev & EV_ENC_S)
        *d->is_enabled = !(*d->is_enabled);
}

void draw_setting(const Widget *w, w_state_t state, unsigned event_flags) {
    const SettingData *d = (const SettingData *)w->data;

    // Draw the text inside
    bbox_t bb = fnt_draw_printf(w->x + 4, w->y + 2, H_LEFT | V_TOP, "%s: %d", d->label, *d->value);

    // Draw a nice rounded bounding box
    if (state == W_FOCUSED)
        draw_rectangle_rbb(bb_add_spacing(bb, 4), 4, 0x88);
    else if (state == W_EDITING) {
        set_draw_mode(DRAW_INV);
        fill_rectangle_rbb(bb_add_spacing(bb, 4), 4, 0xFF);
        set_draw_mode(DRAW_SET);
    }
}

void event_setting(const Widget *w, uint32_t ev) {
    const SettingData *d = (const SettingData *)w->data;
    // Increment / decrement value by step
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
