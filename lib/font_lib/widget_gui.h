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

typedef struct Widget {
    void (*draw)(struct Widget *w, w_state_t state);
    void (*event)(struct Widget *w, uint32_t ev);  // NULL if not interactive
    int x, y;
    bool selectable;  // Set to false for pure displays (labels)
    void *data;       // Pointer to widget-specific data
} Widget;

typedef struct {
    Widget **widgets;
    uint8_t count;
} Screen;

// -------------------------
//  Widgets
// -------------------------
typedef struct {
    const char *text;
    fnt_align_t align;
} LblData;
void draw_static_label(Widget *w, w_state_t state);

typedef struct {
    void (*format_value)(char *buffer);  // Callback to get the string
    fnt_align_t align;
} DynLblData;
void draw_dyn_label(Widget *w, w_state_t state);

typedef struct {
    const char *label;
    int *value;
    int min, max, step;
} SettingData;
void draw_setting(Widget *w, w_state_t state);
void event_setting(Widget *w, uint32_t ev);

// In widget_label.h
#define WIDGET_LABEL(_x, _y, _text, _align)                                                        \
    {                                                                                              \
        .draw = draw_static_label, .event = NULL, .x = (_x), .y = (_y), .selectable = false,       \
        .data = &(LblData) {                                                                       \
            .text = (_text), .align = (_align)                                                     \
        }                                                                                          \
    }

#define WIDGET_DYNLBL(_x, _y, _cb_format_value, _align)                                            \
    {                                                                                              \
        .draw = draw_dyn_label, .event = NULL, .x = (_x), .y = (_y), .selectable = false,          \
        .data = &(DynLblData) {                                                                    \
            .format_value = (_cb_format_value), .align = (_align)                                  \
        }                                                                                          \
    }

#define WIDGET_SETTING(_x, _y, _label, _value_ptr, _min, _max, _step)                              \
    {                                                                                              \
        .draw = draw_setting, .event = event_setting, .x = (_x), .y = (_y), .selectable = true,    \
        .data = &(SettingData) {                                                                   \
            .label = (_label), .value = (_value_ptr), .min = (_min), .max = (_max),                \
            .step = (_step)                                                                        \
        }                                                                                          \
    }

// ---------------------------------------------------
//  GUI functions
// ---------------------------------------------------
// Initialize the GUI with an array of screens (slides)
void gui_init(Screen **screens, uint8_t num_screens);

// Render the current state to the framebuffer
void gui_draw(bool force_draw);

// External interface. This needs to be implemented somewhere else...
// returns the instantaneous state of the encoder and back button (in the 2 LSBs)
// the other bits are used to indicate events. See the EV_ flags above.
unsigned get_event_flags(void);
