#pragma once
#include "font.h"
#include "graphics.h"
#include <stdbool.h>
#include <stdint.h>

// Minimalistic GUI to be used with a single encoder and back button
// Takes care of the navigation:
//   * selecting a tab
//   * selecting a particular widget on a tab
//   * interacting with the widget
//
// Widgets are kept very general and are implemented with an on_event() and on_draw() callback

// ---------------------------------------------------
//  Event codes
// ---------------------------------------------------
// these are returned by get_event_flags() / given as parameter to on_event()
// Instantaneous value (1 = pushed)
#define EV_ENC (1 << 0)   // encoder button
#define EV_BACK (1 << 1)  // back button
// On push
#define EV_ENC_P (1 << 4)
#define EV_BACK_P (1 << 5)
// Short push and release
#define EV_ENC_S (1 << 8)
#define EV_BACK_S (1 << 9)
// Long push and release
#define EV_ENC_L (1 << 12)
#define EV_BACK_L (1 << 13)
// Encoder ticks. Alternative to get_encoder_ticks()
#define EV_ROT_CCW (1 << 16)
#define EV_ROT_CW (1 << 17)

// ---------------------------------------------------
//  GUI data-types
// ---------------------------------------------------
// The visual state of a widget
typedef enum {
    W_NORMAL = 0,  // Unselected
    W_FOCUSED,     // Selected, ready to be edited
    W_EDITING      // Actively being manipulated
} w_state_t;

// Forward declaration needed for the const function pointers
struct Widget;

typedef struct Widget {
    void (*draw)(const struct Widget *w, w_state_t state, unsigned event_flags);
    void (*event)(const struct Widget *w, uint32_t ev);  // NULL if not interactive
    int x, y;
    bool selectable;   // Set to false for labels without interaction
    bool editable;     // Set to false for buttons / check-boxes with direct action
    const void *data;  // Pointer to const ROM widget-specific data
} Widget;

typedef struct {
    const Widget *const *widgets;  // ROM array of pointers to ROM Widgets
    uint8_t count;
} Screen;

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

// ---------------------------------------------------
//  GUI functions
// ---------------------------------------------------
// Initialize the GUI with an array of screens (slides)
void gui_init(const Screen *const *screens, uint8_t num_screens);

// Render the current state to the framebuffer
void gui_draw(bool force_draw);

// External interface. This needs to be implemented somewhere else...
// returns the instantaneous state of the encoder and back button (in the 2 LSBs)
// the other bits are used to indicate events. See the EV_ flags above.
unsigned get_event_flags(void);
