#include "widgets.h"
#include "font.h"
#include "frame_buffer.h"
#include "graphics.h"
#include "widget_gui.h"
#include <sys/param.h>

#define C_FOC 0x88  // intensity value of cursor rectangle

void draw_label(const Widget *w, w_state_t state, unsigned event_flags) {
    char buffer[32] = {0};
    const LblData *d = (const LblData *)w->data;

    // only redraw static labels on events
    if (event_flags == 0 && d->format_value == NULL)
        return;

    // Determine labels string
    const char *txt = buffer;
    if (d->format_value != NULL)
        d->format_value(buffer);  // e.g. formats "24.5 °C" into buffer
    else
        txt = d->text;

    bbox_t bb = w->bb;

    int txt_x = 0, txt_y = 0;

    // right and bottom borders are optional
    if (bb.right < 0 || bb.bottom < 0) {
        // user gave x and y directly
        txt_x = bb.left;
        txt_y = bb.top;
        bb = fnt_measure_text(txt_x, txt_y, txt, sizeof(buffer), d->align);
    } else {
        // determine text x and y coords from user-given bbox
        if (d->align & H_LEFT)
            txt_x = bb.left;
        else if (d->align & H_MIDDLE)
            txt_x = (bb.left + bb.right) / 2;
        else if (d->align & H_RIGHT)
            txt_x = bb.right;

        if (d->align & V_TOP)
            txt_y = bb.top;
        else if (d->align & V_MIDDLE)
            txt_y = (bb.top + bb.bottom) / 2;
        else if (d->align & V_BOTTOM)
            txt_y = bb.bottom;
    }

    // Erase background
    set_draw_mode(DRAW_SET);
    fill_rectangle_bb(bb, 0);

    // Draw label
    set_draw_mode(DRAW_ADD);
    fnt_draw_text(txt_x, txt_y, txt, sizeof(buffer), d->align);
}

void draw_button(const Widget *w, w_state_t state, unsigned event_flags) {
    const LblData *d = (const LblData *)w->data;

    // only redraw buttons on events
    if (event_flags == 0 && d->format_value == NULL)
        return;

    bbox_t bb = w->bb;

    // Draw text (always centered)
    set_draw_mode(DRAW_ADD);
    int txt_x = (bb.left + bb.right + 1) / 2;
    int txt_y = (bb.top + bb.bottom + 1) / 2;
    // Move text down a bit if encoder is pushed
    if (state == W_FOCUSED && event_flags & EV_ENC) {
        txt_x++;
        txt_y++;
    }
    fnt_draw_text(txt_x, txt_y, d->text, 32, H_MIDDLE | V_MIDDLE);

    // Draw rounded rectangle, if selected
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

    // only redraw the check-box on events
    if (event_flags == 0)
        return;

    int x = w->bb.left;
    int y = w->bb.bottom;

    // Move checkbox down when encoder is pressed down
    int y_ = y;
    if (state == W_FOCUSED && event_flags & 1)
        y_ += 1;

    // Draw the actual check-box
    draw_rectangle_c(x, y_ - 1, 8, 8, 0xFF);
    if (*d->is_enabled)
        fill_rectangle_c(x, y_ - 1, 5, 5, 0xFF);

    // Draw the text label
    bbox_t bb = fnt_draw_text(x + 10, y, d->text, 32, H_LEFT | V_MIDDLE);
    bb.left -= 14;

    // Draw cursor if selected
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

    // only redraw the setting chooser on events
    if (event_flags == 0)
        return;

    // Draw the text inside
    bbox_t bb =
        fnt_draw_printf(w->bb.left, w->bb.bottom, H_LEFT | V_MIDDLE, "%s %d", d->label, *d->value);

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

    // only redraw the scroll bar on events
    if (event_flags == 0)
        return;

    // bbox_t bb = {w->x, w->x + width, w->y, w->y + d->height};
    bbox_t bb = w->bb;
    bb.right = bb.left + width;

    set_draw_mode(DRAW_SET);

    draw_rectangle_bb(bb, state == W_EDITING ? 0xF0 : 0x40);

    // Draw a nice rounded bounding box if focused
    if (state == W_FOCUSED || state == W_EDITING)
        draw_rectangle_rbb(bb_add_spacing(bb, 3), 4, C_FOC);

    const int n_steps = d->max_position - d->min_position;
    const int dy = (bb.bottom - bb.top - d->slider_height) * 256 / n_steps;
    const int y0 = ((*d->position - d->min_position) * dy + 128) / 256;

    fill_rectangle(bb.left + 1,
                   bb.top + y0 + 1,
                   bb.right - 1,
                   bb.top + y0 + d->slider_height - 1,
                   state == W_EDITING ? 0xF0 : 0x40);
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

    for (int col = 0; col < d->n_cols; col++) {
        int x = w->bb.left + col * d->col_advance;
        for (int r = 0; r < d->n_rows; r++) {
            int y = w->bb.top + r * d->row_advance;
            if (y >= FB_HEIGHT)  // stop drawing when row is outside of framebuffer
                break;

            static char cell_buff[16];
            cell_buff[0] = '\0';

            const int row = r == 0 ? 0 : r + scroll_offset;  // frozen row 0
            // const int row = r + scroll_offset;  // row 0 scrolls with the other rows
            if (row >= d->n_rows)
                break;

            bool do_redraw = d->format_cell(row, col, cell_buff, sizeof(cell_buff));
            cell_buff[sizeof(cell_buff) - 1] = '\0';

            // redraw all cells on events
            do_redraw |= event_flags != 0;
            if (!do_redraw)
                continue;

            // Erase and redraw the cell
            set_draw_mode(DRAW_SET);
            fill_rectangle(x, y, x + d->col_advance, y + d->row_advance, 0);

            set_draw_mode(DRAW_ADD);
            fnt_draw_text(x + d->col_advance, y, cell_buff, sizeof(cell_buff), H_RIGHT | V_TOP);
        }
    }
}

#define N_FRAC 11
#define LERP(a, b, t) ((a) + (((b) - (a)) * (t) >> N_FRAC))

void draw_trend_view(const Widget *w, w_state_t state, unsigned event_flags) {
    const TrendViewData *d = (const TrendViewData *)w->data;

    // static unsigned last_ts = 0;
    // unsigned ts = millis();
    // if (ts - last_ts > d->interval_ms && event_flags == 0)
    //     return;
    // last_ts = ts;
    set_draw_mode(DRAW_SET);
    fill_rectangle_bb(w->bb, 0);

    // Find the min, max and sum-value
    int min_value, max_value, sum_value = 0;
    for (int i = 0; i < d->n_points; i++) {
        if (i == 0)
            min_value = max_value = d->y_data[0];
        else if (d->y_data[i] > max_value)
            max_value = d->y_data[i];
        else if (d->y_data[i] < min_value)
            min_value = d->y_data[i];
        sum_value += d->y_data[i];
    }

    // Find a offset and scale factor to fit the data into the pixels
    // max_value * scale + offset = top
    // min_value * scale + offset = bottom
    // scale = (top - bottom) / (max_value - min_value)
    // offset = bottom - min_value * scale

    // Avoid division by 0
    if (max_value == min_value) {
        min_value--;
        max_value++;
    }

    const int scale = ((w->bb.top - w->bb.bottom) << N_FRAC) / (max_value - min_value);
    const int offset = (w->bb.bottom << N_FRAC) - min_value * scale;

    unsigned rp = 0;
    if (d->wp != NULL)
        rp = *d->wp;

    // Plot some horizontal lines
    draw_hline(w->bb.left, w->bb.right, w->bb.top, 0x40);
    draw_hline(w->bb.left, w->bb.right, (w->bb.top + w->bb.bottom + 1) / 2, 0x40);
    draw_hline(w->bb.left, w->bb.right, w->bb.bottom, 0x40);

    // Plot the lines
    set_draw_mode(DRAW_ADD);
    for (int i = 0; i < d->n_points; i++) {
        static int last_x, last_y;
        int x = LERP(w->bb.left << N_FRAC, w->bb.right << N_FRAC, (i << N_FRAC) / d->n_points);
        int y = scale * (int)(d->y_data[rp]) + offset;
        rp = (rp + 1) % d->n_points;

        // Round and remove fractional part
        x = (x + (1 << N_FRAC) / 2) >> N_FRAC;
        y = (y + (1 << N_FRAC) / 2) >> N_FRAC;

        if (i > 0)
            draw_line(last_x, last_y, x, y);

        last_x = x;
        last_y = y;
    }

    // Print the labels
    fnt_draw_printf(w->bb.left, w->bb.top, H_LEFT | V_TOP, "\020%d", max_value);
    fnt_draw_printf(w->bb.left,
                    (w->bb.top + w->bb.bottom + 1) / 2,
                    H_LEFT | V_MIDDLE,
                    "%d",
                    sum_value / d->n_points);
    fnt_draw_printf(w->bb.left, w->bb.bottom, H_LEFT | V_BOTTOM, "%d", min_value);
}
