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
    bbox_t bb = fnt_draw_text(x, y, d->text, 32, H_LEFT | V_MIDDLE);
    bb = bb_add_spacing(bb, d->padding);

    set_draw_mode(DRAW_ADD);
    fill_rectangle_bb(bb, 0x33);

    if (state == W_FOCUSED)
        draw_rectangle_rbb(bb_add_spacing(bb, 5), 4, 0xFF);

    // WIN95 style shading
    set_draw_mode(DRAW_SET);
    draw_vline(bb.left - 1, bb.top - 1, bb.bottom + 1, 0xDD);
    draw_hline(bb.left - 1, bb.right + 1, bb.top - 1, 0xDD);
    draw_vline(bb.right + 1, bb.top, bb.bottom + 1, 0x11);
    draw_hline(bb.left, bb.right + 1, bb.bottom + 1, 0x11);
    // Invert button when encoder is pressed down
    if (state == W_FOCUSED && event_flags & 1) {
        set_draw_mode(DRAW_INV);
        fill_rectangle_bb(bb_add_spacing(bb, 1), 0xFF);
    }
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

void draw_grid_view(const Widget *w, w_state_t state, unsigned event_flags) {
    const GridViewData *d = (const GridViewData *)w->data;
    for (int col = 0; col < d->n_cols; col++) {
        int x = w->x + col * d->col_advance;
        for (int row = 0; row < d->n_rows; row++) {
            int y = w->y + row * d->row_advance;
            static char cell_buff[32];
            cell_buff[0] = '\0';
            d->format_cell(row, col, cell_buff, sizeof(cell_buff));
            cell_buff[sizeof(cell_buff) - 1] = '\0';
            fnt_draw_text(x, y, cell_buff, sizeof(cell_buff), H_RIGHT | V_BASELINE);
        }
    }
}
