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
} LblData;

void draw_static_label(const Widget *w, w_state_t state, unsigned event_flags);

#define WIDGET_LABEL(_x, _y, _text, _align)                                                        \
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

#define WIDGET_DYNLBL(_x, _y, _cb_format_value, _align)                                            \
    &(Widget) {                                                                                    \
        .draw = draw_dyn_label, .event = NULL, .x = (_x), .y = (_y), .selectable = false,          \
        .data = &(const DynLblData) {                                                              \
            .format_value = (_cb_format_value), .align = (_align)                                  \
        }                                                                                          \
    }

// A button
void draw_button(const Widget *w, w_state_t state, unsigned event_flags);
#define WIDGET_BUTTON(_x, _y, _text, _event_cb)                                                    \
    &(Widget) {                                                                                    \
        .draw = draw_button, .event = _event_cb, .x = (_x), .y = (_y), .selectable = true,         \
        .data = &(const LblData) {                                                                 \
            .text = (_text)                                                                        \
        }                                                                                          \
    }

// A rectangular check-box with a label which can be enabled / disabled
typedef struct {
    const char *text;
    bool *is_enabled;
} CheckBoxData;

void draw_check_box(const Widget *w, w_state_t state, unsigned event_flags);
void event_check_box(const Widget *w, uint32_t ev);

#define WIDGET_CHECK_BOX(_x, _y, _text, _is_enabled)                                               \
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

#define WIDGET_SETTING(_x, _y, _label, _value_ptr, _min, _max, _step)                              \
    &(Widget) {                                                                                    \
        .draw = draw_setting, .event = event_setting, .x = (_x), .y = (_y), .selectable = true,    \
        .editable = true, .data = &(const SettingData) {                                           \
            .label = (_label), .value = (_value_ptr), .min = (_min), .max = (_max),                \
            .step = (_step)                                                                        \
        }                                                                                          \
    }

// Table view with a label and multiple rows of data
typedef struct {
    void (*format_int)(char *buffer, int val);  // Format val as string and write in buffer
    const int n_rows;
    const int row_advance;  // line spacing between 2 rows
    // Headline label
    const char *top_label;
    // array of n_rows elements, labels on the left, can be NULL
    const char *const *left_labels;
    // array of n_rows elements, values to print.
    int *values;
    const font_header_t *font_left_label;  // can be NULL
    const font_header_t *font_value;       // can be NULL
} TableViewData;

void draw_table_view(const Widget *w, w_state_t state, unsigned event_flags);

#define WIDGET_TABLE_VIEW(_x,                                                                      \
                          _y,                                                                      \
                          _format_int,                                                             \
                          _n_rows,                                                                 \
                          _row_advance,                                                            \
                          _top_label,                                                              \
                          _left_labels,                                                            \
                          _values,                                                                 \
                          _font_left_label,                                                        \
                          _font_value)                                                             \
    &(Widget) {                                                                                    \
        .draw = draw_table_view, .event = NULL, .x = (_x), .y = (_y), .selectable = false,         \
        .editable = false, .data = &(const TableViewData) {                                        \
            .format_int = _format_int, .n_rows = _n_rows, .row_advance = _row_advance,             \
            .top_label = _top_label, .left_labels = _left_labels, .values = _values,               \
            .font_left_label = _font_left_label, .font_value = _font_value                         \
        }                                                                                          \
    }
