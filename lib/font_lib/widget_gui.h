#pragma once
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
    bbox_t bb;         // the bounding box of the widget, erased on redraw
    bool selectable;   // Set to false for labels without interaction
    bool editable;     // Set to false for buttons / check-boxes with direct action
    const void *data;  // Pointer to const ROM widget-specific data
} Widget;

typedef struct {
    const Widget *const *widgets;  // ROM array of pointers to ROM Widgets
    uint8_t count;
} Screen;

// ---------------------------------------------------
//  GUI functions
// ---------------------------------------------------
// Initialize the GUI with an array of screens (slides)
void gui_init(const Screen *const *screens, uint8_t num_screens);

// Render the current state to the framebuffer
void gui_draw(void);

// External interface. This needs to be implemented somewhere else...
// returns the instantaneous state of the encoder and back button (in the 2 LSBs)
// the other bits are used to indicate events. See the EV_ flags above.
unsigned get_event_flags(void);
