#include "widget_gui.h"
#include "font.h"
#include "frame_buffer.h"
#include "graphics.h"
#include <stddef.h>

extern const font_header_t f_fixed;

static gui_t *g_gui = NULL;

void set_gui(gui_t *val) { g_gui = val; }

static void draw_tab_bar(gui_t *gui) {
    set_draw_mode(DRAW_ADD);
    draw_line(0, 15, FB_WIDTH, 15);
    bbox_t bb = {0};
    for (int i = 0; i < gui->num_pages; i++) {
        fnt_init_from_header(&f_fixed);
        bb = fnt_draw_text(bb.right + 5, 1, gui->pages[i].tab_name, 8, H_LEFT | V_TOP);
        if (i == gui->active_page) {
            set_draw_mode(DRAW_INV);
            fill_rectangle_bb(bb, 0xFF);
            set_draw_mode(DRAW_ADD);
        }
    }
}

void widget_gui_update(bool full_redraw) {
    if (g_gui == NULL) {
        D("No GUI set!!\n");
        return;
    }

    page_t *current_page = &g_gui->pages[g_gui->active_page];
    widget_t *active_widget = &current_page->widgets[current_page->focused_index];

    // Fetch event-flags from hardware
    unsigned events = get_event_flags();

    // Give the focused widget the first chance to consume the event
    bool consumed = false;
    if (g_gui->state != NAV_TABS && active_widget->on_event != NULL) {
        consumed = active_widget->on_event(active_widget, events, g_gui->state == EDIT_WIDGET);
    }

    // If the widget didn't eat the event, handle GUI navigation
    if (!consumed) {
        // --- Handle BACK button ---
        if (events & EV_BACK_S) {
            if (g_gui->state == EDIT_WIDGET)
                // Cancel edit, back to widget list
                g_gui->state = NAV_WIDGETS;
            else if (g_gui->state == NAV_WIDGETS)
                // Cancel widget list, back to tabs
                g_gui->state = NAV_TABS;
            else if (g_gui->state == NAV_TABS) {
                // Back to page zero
                g_gui->active_page = 0;
                full_redraw = true;
            }
        } else if (events & EV_ENC_S) {
            // --- Handle ENCODER CLICK ---
            if (g_gui->state == NAV_TABS) {
                g_gui->state = NAV_WIDGETS;  // Enter the tab
            } else if (g_gui->state == NAV_WIDGETS && active_widget->can_focus) {
                g_gui->state = EDIT_WIDGET;  // Enter edit mode for this widget
            } else if (g_gui->state == EDIT_WIDGET) {
                g_gui->state = NAV_WIDGETS;  // Confirm edit, drop back to widget list
            }
        } else if (events & (EV_ROT_CCW | EV_ROT_CW)) {
            // --- Handle ENCODER ROTATION ---
            if (g_gui->state == NAV_TABS) {
                // Scroll Tabs
                int new_page = g_gui->active_page + (events & EV_ROT_CCW ? -1 : 1);
                if (new_page < 0)
                    new_page = g_gui->num_pages - 1;
                if (new_page >= g_gui->num_pages)
                    new_page = 0;
                g_gui->active_page = new_page;
                full_redraw = true;
            } else if (g_gui->state == NAV_WIDGETS) {
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
                        break;  // Hit the end, stop trying to find focusable widgets
                    }
                }
                current_page->focused_index = new_idx;
            }
        }
        // D("gui_state: %d,  active_page: %d,  focused_index: %d\n",
        //   g_gui->state,
        //   g_gui->active_page,
        //   current_page->focused_index);
    }

    // Render the screen (same as before)
    if (full_redraw) {
        fill(0);
        draw_tab_bar(g_gui);
    }
    for (unsigned i = 0; i < current_page->num_widgets; i++) {
        widget_t *w = &current_page->widgets[i];
        w->draw(w, i == current_page->focused_index, g_gui->state == EDIT_WIDGET, full_redraw);
    }
}

// -----------------------
//  Widgets
// -----------------------
// --- Static label Widget ---
void draw_simple_static_label(widget_t *self, bool is_focused, bool is_editing, bool full_redraw) {
    // Only draw once when the screen changes
    if (!full_redraw)
        return;

    // Cast the generic data pointer to our specific label state
    char *text = (char *)self->data;

    fnt_init_from_header(&f_fixed);
    // set_draw_region(self->bounds.left, self->bounds.top, self->bounds.right,
    // self->bounds.bottom);

    // Draw the text using the parameters
    int center_y = self->bounds.top + ((self->bounds.bottom - self->bounds.top) / 2);
    int center_x = self->bounds.left + ((self->bounds.right - self->bounds.left) / 2);

    set_draw_mode(DRAW_ADD);
    fnt_draw_text(center_x, center_y, text, 64, H_MIDDLE | V_MIDDLE);
}

// bool button_on_event(widget_t *self, unsigned events, int ticks, bool is_editing) {
//     if (events & EV_ENC_S) {
//         // Trigger the button's action!
//         // ... (e.g., call a callback stored in self->data) ...

//         return true;  // We consumed the click! The GUI shouldn't do anything else.
//     }
//     return false;  // We don't care about rotation or other events
// }

// bool spinbox_on_event(widget_t *self, unsigned events, int ticks, bool is_editing) {
//     if (is_editing && ticks != 0) {
//         int *value = (int *)self->data;
//         *value += ticks;

//         // Add min/max clamping here...

//         return true;  // We consumed the rotation to change the value
//     }

//     // Notice we do NOT consume EV_ENC_S here!
//     // We return false, which tells the GUI context "I don't know what to do
//     // with this click, you handle it." (The GUI will use it to enter/exit edit mode).
//     return false;
// }
