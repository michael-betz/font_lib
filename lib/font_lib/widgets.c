#include "widgets.h"
#include "font.h"
#include "frame_buffer.h"
#include "graphics.h"
#include "widget_gui.h"
#include <sys/param.h>

#define C_FOC 0x88  // intensity value of cursor rectangle

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
    bbox_t bb;
    if (state == W_FOCUSED && event_flags & 1) {
        // When button is pushed, offset the text (but not the BB)
        bb = fnt_draw_text(x + 1, y + 1, d->text, 32, d->align);
        bb.left -= 1;
        bb.right -= 1;
        bb.top -= 1;
        bb.bottom -= 1;
    } else {
        bb = fnt_draw_text(x, y, d->text, 32, d->align);
    }
    bb = bb_add_spacing(bb, d->padding);

    // Is width set? Then produce a fixed width button
    if (d->width > 0) {
        bb.left = x - d->width / 2;
        bb.right = x + d->width / 2;
    }

    set_draw_mode(DRAW_ADD);
    fill_rectangle_bb(bb, 0x33);

    if (state == W_FOCUSED)
        draw_rectangle_rbb(bb_add_spacing(bb, 4), 4, C_FOC);

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
        draw_rectangle_rbb(bb_add_spacing(bb, 4), 4, C_FOC);
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
        draw_rectangle_rbb(bb_add_spacing(bb, 4), 4, C_FOC);
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

void draw_v_scroll(const Widget *w, w_state_t state, unsigned event_flags) {
    const VScrollData *d = (const VScrollData *)w->data;
    const int width = 4;

    bbox_t bb = {w->x, w->x + width, w->y, w->y + d->height};

    draw_rectangle_bb(bb, state == W_EDITING ? 0xA0 : 0x40);

    // Draw a nice rounded bounding box if focused
    if (state == W_FOCUSED || state == W_EDITING)
        draw_rectangle_rbb(bb_add_spacing(bb, 3), 4, C_FOC);

    const int n_steps = d->max_position - d->min_position;
    const int dy = (d->height - d->slider_height) * 256 / n_steps;
    const int y0 = ((*d->position - d->min_position) * dy + 128) / 256;

    fill_rectangle(w->x + 1,
                   w->y + y0 + 1,
                   w->x + width - 1,
                   w->y + y0 + d->slider_height - 1,
                   state == W_EDITING ? 0xFF : 0x40);
}

void event_v_scroll(const Widget *w, uint32_t ev) {
    const VScrollData *d = (const VScrollData *)w->data;

    // Increment / decrement value by step
    if (ev & EV_ROT_CW)
        *d->position += 1;
    if (ev & EV_ROT_CCW)
        *d->position -= 1;

    // Clamp values
    if (*d->position > d->max_position)
        *d->position = d->max_position;
    if (*d->position < d->min_position)
        *d->position = d->min_position;
}

// -----------------------------
//  Non interactive widgets
// -----------------------------

void draw_grid_view(const Widget *w, w_state_t state, unsigned event_flags) {
    const GridViewData *d = (const GridViewData *)w->data;
    int scroll_offset = 0;
    if (d->scroll_offset != NULL)
        scroll_offset = *d->scroll_offset;

    if (scroll_offset < 0)
        scroll_offset = 0;

    set_draw_mode(DRAW_ADD);

    for (int col = 0; col < d->n_cols; col++) {
        int x = w->x + col * d->col_advance;
        for (int r = 0; r < d->n_rows; r++) {
            int y = w->y + r * d->row_advance;
            if (y >= FB_HEIGHT)  // stop drawing when row is outside of framebuffer
                break;

            static char cell_buff[16];
            cell_buff[0] = '\0';

            const int row = r == 0 ? 0 : r + scroll_offset;  // frozen row 0
            // const int row = r + scroll_offset;  // row 0 scrolls with the other rows
            if (row >= d->n_rows)
                break;

            d->format_cell(row, col, cell_buff, sizeof(cell_buff));
            cell_buff[sizeof(cell_buff) - 1] = '\0';

            fnt_draw_text(x, y, cell_buff, sizeof(cell_buff), H_RIGHT | V_BASELINE);
        }
    }
}
