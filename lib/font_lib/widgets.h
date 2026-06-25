#pragma once
#include "font.h"
#include "widget_gui.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// -------------------------
//  Widgets
// -------------------------
// A label, static or dynamic
typedef struct {
    const char *text;                    // static string, used when callback is NULL
    void (*format_value)(char *buffer);  // Callback to get the string
    fnt_align_t align;
} LblData;

void draw_label(const Widget *w, w_state_t state, unsigned event_flags);

// Static label, only redraw when screens change. Set _format_value callback to NULL
// Dynamic label, redrawn on every frame, test its text from _format_value, ignores _text
// To erase background over a fixed size, set _x0, _y0, _x1 and _y1. Then _align detrmines
// the alignment of the text within this bounding box
// Alternatively, set _x0 and _y0 to the anchor point of the text and _x1 and _y1 to -1, then
// the area to erase is determined from the text size
#define W_LABEL(_x0, _y0, _x1, _y1, _text, _format_value, _align)                                  \
    &(Widget) {                                                                                    \
        .draw = draw_label, .event = NULL, .selectable = false, .bb.left = _x0, .bb.top = _y0,     \
        .bb.right = _x1, .bb.bottom = _y1, .data = &(const LblData) {                              \
            .text = (_text), .format_value = _format_value, .align = (_align)                      \
        }                                                                                          \
    }

// A button
void draw_button(const Widget *w, w_state_t state, unsigned event_flags);
#define W_BUTTON(_x0, _y0, _x1, _y1, _text, _event_cb)                                             \
    &(Widget) {                                                                                    \
        .draw = draw_button, .event = _event_cb, .bb.left = _x0, .bb.top = _y0, .bb.right = _x1,   \
        .bb.bottom = _y1, .selectable = true, .data = &(const LblData) {                           \
            .text = (_text), .format_value = NULL, .align = H_MIDDLE | V_MIDDLE                    \
        }                                                                                          \
    }

// // A rectangular check-box with a label which can be enabled / disabled
typedef struct {
    const char *text;
    bool *is_enabled;
} CheckBoxData;

void draw_check_box(const Widget *w, w_state_t state, unsigned event_flags);
void event_check_box(const Widget *w, uint32_t ev);

#define W_CHECK_BOX(_x, _y, _text, _is_enabled)                                                    \
    &(Widget) {                                                                                    \
        .draw = draw_check_box, .event = event_check_box, .bb.left = (_x), .bb.bottom = (_y),      \
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
        .draw = draw_setting, .event = event_setting, .bb.left = (_x), .bb.bottom = (_y),          \
        .selectable = true, .editable = true, .data = &(const SettingData) {                       \
            .label = (_label), .value = (_value_ptr), .min = (_min), .max = (_max),                \
            .step = (_step)                                                                        \
        }                                                                                          \
    }

// // A grid of cells. The format_cell() callback is called to get the content of each cell.
typedef struct {
    bool (*format_cell)(int row,
                        int col,
                        char *buffer,
                        const int buffer_size);  // Write the content of a cell into buffer
    const int n_rows;
    const int n_cols;
    const int row_advance;  // pixels between 2 rows
    const int col_advance;  // pixels between 2 columns
    int *scroll_offset;     // to make it scroll, set to a position value from a W_V_SCROLL() or
} GridViewData;

void draw_grid_view(const Widget *w, w_state_t state, unsigned event_flags);

#define W_GRID_VIEW(                                                                               \
    _x, _y, _format_cell, _n_rows, _n_cols, _row_advance, _col_advance, _scroll_offset)            \
    &(Widget) {                                                                                    \
        .draw = draw_grid_view, .event = NULL, .bb.left = (_x), .bb.bottom = (_y),                 \
        .selectable = false, .editable = false, .data = &(const GridViewData) {                    \
            .format_cell = _format_cell, .n_rows = _n_rows, .n_cols = _n_cols,                     \
            .row_advance = _row_advance, .col_advance = _col_advance,                              \
            .scroll_offset = _scroll_offset                                                        \
        }                                                                                          \
    }

// A vertical scroll-bar
typedef struct {
    const int slider_height;
    int *position;
    const int min_position;
    const int max_position;
} VScrollData;

void draw_v_scroll(const Widget *w, w_state_t state, unsigned event_flags);
void event_v_scroll(const Widget *w, uint32_t ev);

#define W_V_SCROLL(_x0, _y0, _y1, _slider_height, _position, _min_position, _max_position)         \
    &(Widget) {                                                                                    \
        .draw = draw_v_scroll, .event = event_v_scroll, .bb.left = (_x0), .bb.top = (_y0),         \
        .bb.bottom = _y1, .selectable = true, .editable = true, .data = &(const VScrollData) {     \
            .slider_height = _slider_height, .position = _position, .min_position = _min_position, \
            .max_position = _max_position                                                          \
        }                                                                                          \
    }

// // A trend-line plot
// typedef struct {
//     const int width;
//     const int height;
//     const int n_lines;
//     const int interval_ms;
//     const int y_min;
//     const int y_max;
//     int *data;  // array of size data[n_lines] with the latest data-point
// } TrendViewData;

// void draw_trend_view(const Widget *w, w_state_t state, unsigned event_flags);

// #define W_TREND_VIEW(_x, _y, _width, _height, _n_lines, _interval_ms, _y_min, _y_max, _data) \
//     &(Widget) { \
//         .draw = draw_trend_view, .event = NULL, .x = (_x), .y = (_y), .selectable = false, \
//         .editable = false, .data = &(const TrendViewData) { \
//             .width = _width, .height = _height, .n_lines = _n_lines, .interval_ms = _interval_ms,
//             \
//             .y_min = _y_min, .y_max = _y_max, .data = _data, \
//         } \
//     }
