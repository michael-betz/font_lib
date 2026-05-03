#pragma once
#include "font.h"
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
//  Navigation Objects
// ---------------------------------------------------
// Base Widget
typedef struct widget_s widget_t;
struct widget_s {
    uint8_t id;
    bbox_t bounds;   // The bounding box (we use this for drawing focus borders!)
    bool can_focus;  // Some static widgets don't need focus
    void (*draw)(widget_t *self, bool is_focused, bool is_editing);
    // Returns true if the widget consumed the event, false if the GUI should handle it
    bool (*on_event)(widget_t *self, unsigned events, bool is_editing);
    void *data;  // Pointer to the actual string, integer, etc.
};

// A Page (content of a tab)
typedef struct {
    const char *tab_name;  // The text to display in the top Tab Bar
    widget_t *widgets;     // Array of widgets on this page
    uint8_t num_widgets;
    uint8_t focused_index;  // Remember what the user was doing on this tab!
} page_t;

// 3. The Main GUI Context
typedef enum {
    STATE_NAV_TABS,     // Turning encoder switches tabs
    STATE_NAV_WIDGETS,  // Turning encoder switches focus between widgets
    STATE_EDIT_WIDGET   // Turning encoder changes a widget's value (e.g., volume slider)
} gui_state_t;

typedef struct {
    page_t *pages;
    uint8_t num_pages;
    uint8_t active_page;  // Which tab is currently visible
    gui_state_t state;
} gui_t;

// Point to the GUI to operate on
void set_gui(gui_t *val);

// Call this in the main loop to draw a new frame
void widget_gui_update(void);

// External interface. This needs to be implemented somewhere else...
// returns the instantaneous state of the encoder and back button (in the 2 LSBs)
// the other bits are used to indicate events. See the EV_ flags above.
unsigned get_event_flags(void);
