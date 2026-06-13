#pragma once
#include "font.h"
#include "widget_gui.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// -------------------------
//  Widgets
// -------------------------
// Static label which never changes
typedef struct {
    const char *text;
    fnt_align_t align;
    int padding;
} LblData;

void draw_static_label(const Widget *w, w_state_t state, unsigned event_flags);

#define W_LABEL(_x, _y, _text, _align)                                                             \
    &(Widget) {                                                                                    \
        .draw = draw_static_label, .event = NULL, .x = (_x), .y = (_y), .selectable = false,       \
        .data = &(const LblData) {                                                                 \
            .text = (_text), .align = (_align)                                                     \
        }                                                                                          \
    }

// Dynamic label which can be updated with a callback
typedef struct {
    void (*format_value)(char *buffer);  // Callback to get the string
    fnt_align_t align;
} DynLblData;

void draw_dyn_label(const Widget *w, w_state_t state, unsigned event_flags);

#define W_DYNLBL(_x, _y, _cb_format_value, _align)                                                 \
    &(Widget) {                                                                                    \
        .draw = draw_dyn_label, .event = NULL, .x = (_x), .y = (_y), .selectable = false,          \
        .data = &(const DynLblData) {                                                              \
            .format_value = (_cb_format_value), .align = (_align)                                  \
        }                                                                                          \
    }

// A button
void draw_button(const Widget *w, w_state_t state, unsigned event_flags);
#define W_BUTTON(_x, _y, _padding, _text, _event_cb)                                               \
    &(Widget) {                                                                                    \
        .draw = draw_button, .event = _event_cb, .x = (_x), .y = (_y), .selectable = true,         \
        .data = &(const LblData) {                                                                 \
            .text = (_text), .padding = _padding                                                   \
        }                                                                                          \
    }

// A rectangular check-box with a label which can be enabled / disabled
typedef struct {
    const char *text;
    bool *is_enabled;
} CheckBoxData;

void draw_check_box(const Widget *w, w_state_t state, unsigned event_flags);
void event_check_box(const Widget *w, uint32_t ev);

#define W_CHECK_BOX(_x, _y, _text, _is_enabled)                                                    \
    &(Widget) {                                                                                    \
        .draw = draw_check_box, .event = event_check_box, .x = (_x), .y = (_y),                    \
        .selectable = true, .data = &(const CheckBoxData) {                                        \
            .text = (_text), .is_enabled = (_is_enabled)                                           \
        }                                                                                          \
    }

// A value setting which can increment / decrement a value with a step and min / max limits
typedef struct {
    const char *label;
    int *value;  // Points to RAM, but the SettingData struct itself is in ROM
    int min, max, step;
} SettingData;

void draw_setting(const Widget *w, w_state_t state, unsigned event_flags);

void event_setting(const Widget *w, uint32_t ev);

#define W_SETTING(_x, _y, _label, _value_ptr, _min, _max, _step)                                   \
    &(Widget) {                                                                                    \
        .draw = draw_setting, .event = event_setting, .x = (_x), .y = (_y), .selectable = true,    \
        .editable = true, .data = &(const SettingData) {                                           \
            .label = (_label), .value = (_value_ptr), .min = (_min), .max = (_max),                \
            .step = (_step)                                                                        \
        }                                                                                          \
    }

// A grid of cells. The format_cell() callback is called to get the content of each cell.
// TODO: make this scrollable!
typedef struct {
    void (*format_cell)(int row,
                        int col,
                        char *buffer,
                        const int buffer_size);  // Write the content of a cell into buffer
    const int n_rows;
    const int n_cols;
    const int row_advance;  // pixels between 2 rows
    const int col_advance;  // pixels between 2 columns
} GridViewData;

void draw_grid_view(const Widget *w, w_state_t state, unsigned event_flags);

#define W_GRID_VIEW(_x, _y, _format_cell, _n_rows, _n_cols, _row_advance, _col_advance)            \
    &(Widget) {                                                                                    \
        .draw = draw_grid_view, .event = NULL, .x = (_x), .y = (_y), .selectable = false,          \
        .editable = false, .data = &(const GridViewData) {                                         \
            .format_cell = _format_cell, .n_rows = _n_rows, .n_cols = _n_cols,                     \
            .row_advance = _row_advance, .col_advance = _col_advance,                              \
        }                                                                                          \
    }

// A vertical scroll-bar
typedef struct {
    const int height;
    const int slider_height;
    int *position;
    const int min_position;
    const int max_position;
} VScrollData;

void draw_v_scroll(const Widget *w, w_state_t state, unsigned event_flags);
void event_v_scroll(const Widget *w, uint32_t ev);

#define W_V_SCROLL(_x, _y, _height, _slider_height, _position, _min_position, _max_position)       \
    &(Widget) {                                                                                    \
        .draw = draw_v_scroll, .event = event_v_scroll, .x = (_x), .y = (_y), .selectable = true,  \
        .editable = true, .data = &(const VScrollData) {                                           \
            .height = _height, .slider_height = _slider_height, .position = _position,             \
            .min_position = _min_position, .max_position = _max_position                           \
        }                                                                                          \
    }
