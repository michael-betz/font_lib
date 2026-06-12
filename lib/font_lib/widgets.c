#include "widgets.h"
#include "font.h"
#include "frame_buffer.h"
#include "graphics.h"
#include "widget_gui.h"
#include <sys/param.h>

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
    int x = w->x, y = w->y;
    // Move button down when encoder is pressed down
    if (state == W_FOCUSED && event_flags & 1) {
        y += 2;
        x += 2;
    }
    bbox_t bb = fnt_draw_text(x, y, d->text, 32, H_LEFT | V_MIDDLE);

    const int padding = 4;
    bb = bb_add_spacing(bb, padding);

    if (state == W_FOCUSED) {
        set_draw_mode(DRAW_INV);
        fill_rectangle_bb(bb, 0xFF);
    } else {
        set_draw_mode(DRAW_ADD);
        fill_rectangle_bb(bb, 0x20);
    }
    set_draw_mode(DRAW_SET);
    if (state == W_FOCUSED && event_flags & 1)
        return;

    draw_hline(bb.left + 2, bb.right + 2, bb.bottom + 2, 0x10);
    draw_vline(bb.right + 2, bb.top + 2, bb.bottom + 2, 0x10);
}

void draw_check_box(const Widget *w, w_state_t state, unsigned event_flags) {
    const CheckBoxData *d = (const CheckBoxData *)w->data;
    int y = w->y;
    // Move checkobox down when encoder is pressed down
    if (state == W_FOCUSED && event_flags & 1)
        y += 1;
    draw_rectangle_c(w->x, y - 1, 8, 8, 0xFF);
    if (*d->is_enabled)
        fill_rectangle_c(w->x, y - 1, 5, 5, 0xFF);
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
    bbox_t bb = fnt_draw_printf(w->x, w->y, H_LEFT | V_MIDDLE, "%s %d", d->label, *d->value);

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

// -----------------------------
//  Non interactive widgets
// -----------------------------

// Table view with a label and multiple rows of data
void draw_table_view(const Widget *w, w_state_t state, unsigned event_flags) {
    const TableViewData *d = (const TableViewData *)w->data;

    // Top label
    bbox_t bbt = fnt_draw_text(w->x, w->y, d->top_label, 32, H_RIGHT | V_BASELINE);

    int left = 0;
    for (int i = 0; i < d->n_rows; i++) {
        // Value
        static char buf[32];
        int y = w->y + (i + 1) * d->row_advance;
        d->format_int(buf, d->values[i]);
        fnt_init_from_header(d->font_value);
        bbox_t bb = fnt_draw_text(w->x, y, buf, 32, H_RIGHT | V_BASELINE);

        if (i == 0)
            left = MIN(bb.left, bbt.left);

        // Left labels
        if (d->left_labels != NULL) {
            fnt_init_from_header(d->font_left_label);
            fnt_draw_text(left, y, d->left_labels[i], 32, H_RIGHT | V_BASELINE);
        }
    }
}
