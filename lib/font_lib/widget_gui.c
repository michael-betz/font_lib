#include "widget_gui.h"
#include "frame_buffer.h"
#include <stddef.h>

static gui_t *gui = NULL;

void set_gui(gui_t *val) { gui = val; }

void widget_gui_update(void) {
    // 1. Fetch hardware state
    unsigned events = get_event_flags();

    // Nothing happened, just return
    if (events == 0 || gui == NULL)
        return;

    page_t *current_page = &gui->pages[gui->active_page];
    widget_t *active_widget = &current_page->widgets[current_page->focused_index];

    bool is_editing = (gui->state == STATE_EDIT_WIDGET);

    // 2. Give the focused widget the first chance to consume the event
    bool consumed = false;
    if (gui->state != STATE_NAV_TABS && active_widget->on_event != NULL) {
        consumed = active_widget->on_event(active_widget, events, is_editing);
    }

    // 3. If the widget didn't eat the event, handle GUI navigation
    if (!consumed) {

        // --- Handle BACK button ---
        if (events & EV_BACK_S) {
            if (gui->state == STATE_EDIT_WIDGET) {
                // Cancel edit, back to widget list
                gui->state = STATE_NAV_WIDGETS;
            } else if (gui->state == STATE_NAV_WIDGETS) {
                // Cancel widget list, back to tabs
                gui->state = STATE_NAV_TABS;
            }
        }

        // --- Handle ENCODER CLICK ---
        else if (events & EV_ENC_S) {
            if (gui->state == STATE_NAV_TABS) {
                gui->state = STATE_NAV_WIDGETS;  // Enter the tab
            } else if (gui->state == STATE_NAV_WIDGETS && active_widget->can_focus) {
                gui->state = STATE_EDIT_WIDGET;  // Enter edit mode for this widget
            } else if (gui->state == STATE_EDIT_WIDGET) {
                gui->state = STATE_NAV_WIDGETS;  // Confirm edit, drop back to widget list
            }
        }

        // --- Handle ENCODER ROTATION ---
        else if (events & (EV_ROT_CCW | EV_ROT_CW)) {
            if (gui->state == STATE_NAV_TABS) {
                // Scroll Tabs
                int new_page = gui->active_page + (events & EV_ROT_CCW ? -1 : 1);
                if (new_page < 0)
                    new_page = gui->num_pages - 1;
                if (new_page >= gui->num_pages)
                    new_page = 0;
                gui->active_page = new_page;
            } else if (gui->state == STATE_NAV_WIDGETS) {
                // Scroll Widgets (skip non-focusable ones)
                int new_idx = current_page->focused_index;

                int dir = events & EV_ROT_CCW ? -1 : 1;
                int steps = 1;

                while (steps > 0) {
                    new_idx += dir;
                    // Clamp to page boundaries
                    if (new_idx < 0)
                        new_idx = 0;
                    if (new_idx >= current_page->num_widgets)
                        new_idx = current_page->num_widgets - 1;

                    if (current_page->widgets[new_idx].can_focus) {
                        steps--;
                    } else if (new_idx == 0 || new_idx == current_page->num_widgets - 1) {
                        break;  // Hit the wall, stop trying to find focusable widgets
                    }
                }
                current_page->focused_index = new_idx;
            }
        }
    }

    // 4. Render the screen (same as before)
    fill(0x20);
    // draw_tab_bar(&gui);
    for (unsigned i = 0; i < current_page->num_widgets; i++) {
        widget_t *w = &current_page->widgets[i];
        w->draw(w, i == current_page->focused_index, is_editing);
    }
}

// -----------------------
//  Widgets
// -----------------------

bool button_on_event(widget_t *self, unsigned events, int ticks, bool is_editing) {
    if (events & EV_ENC_S) {
        // Trigger the button's action!
        // ... (e.g., call a callback stored in self->data) ...

        return true;  // We consumed the click! The GUI shouldn't do anything else.
    }
    return false;  // We don't care about rotation or other events
}

bool spinbox_on_event(widget_t *self, unsigned events, int ticks, bool is_editing) {
    if (is_editing && ticks != 0) {
        int *value = (int *)self->data;
        *value += ticks;

        // Add min/max clamping here...

        return true;  // We consumed the rotation to change the value
    }

    // Notice we do NOT consume EV_ENC_S here!
    // We return false, which tells the GUI context "I don't know what to do
    // with this click, you handle it." (The GUI will use it to enter/exit edit mode).
    return false;
}
