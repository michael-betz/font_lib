#include "font.h"
#include "frame_buffer.h"
#include "graphics.h"
#include <SDL2/SDL.h>
#include <fixed.h>
#include <stdio.h>
#include <vollkorn.h>

static unsigned frame = 0, align = H_MIDDLE, radius = 30;

static char test_str[256] = "Hello World!\nType to edit :)";
static int text_cursor = 28;

static void test_pattern() {
    for (int i = 0; i <= 5; i++) {
        int x = 10 + i * 15;

        set_draw_mode(DRAW_SUB);
        draw_line(x, 5, x, 150);
        for (int y = 10; y <= 150; y += 15) {
            draw_line(3, y, 6, y);
            draw_line(93, y, 96, y);
        }

        set_draw_mode(DRAW_ADD);
        draw_rectangle(x, 10, x + i, 10 + 3, 0xA0);
        fill_rectangle(x, 25, x + i, 25 + 3, 0xA0);

        draw_rectangle_c(x, 40, i, 3, 0xA0);
        fill_rectangle_c(x, 55, i, 3, 0xA0);

        draw_ellipse(x, 70, i, i, 0xF, 0xA0);
        fill_ellipse(x, 85, i, i, 0xF, 0xA0);

        fill_ellipse(x, 100, i, i, 0x1, 0xA0);
        fill_ellipse(x, 115, i, i, 0x2, 0xA0);
        fill_ellipse(x, 130, i, i, 0x4, 0xA0);
        fill_ellipse(x, 145, i, i, 0x8, 0xA0);
    }
}

void test_graphics(void) {
    fill(0x20);

    fnt_init_from_header(&f_vollkorn);
    if (frame == 0)
        fnt_print_info();

    // Keep the text anchor point at a fixed position
    const int txt_x = FB_WIDTH / 2, txt_y = 50, txt_y2 = 150;

    // Draw in a big anti-aliased font
    set_draw_mode(DRAW_ADD);
    fnt_init_from_header(&f_vollkorn);
    bbox_t bb = fnt_draw_text(txt_x, txt_y, test_str, sizeof(test_str), align);
    // Draw the bounding box
    draw_rectangle(bb.left - 1, bb.top - 1, bb.right + 1, bb.bottom + 1, 0x44);

    // Draw in a small pixel font
    fnt_init_from_header(&f_fixed);
    bb = fnt_draw_printf(txt_x, txt_y2, align, test_str, frame);
    // Draw the bounding box
    draw_rectangle(bb.left - 1, bb.top - 1, bb.right + 1, bb.bottom + 1, 0x44);

    // Draw anchor points of the 2 texts
    set_draw_mode(DRAW_INV);
    draw_line(txt_x - 3, txt_y, txt_x + 3, txt_y);
    draw_line(txt_x, txt_y - 3, txt_x, txt_y + 3);
    draw_line(txt_x - 3, txt_y2, txt_x + 3, txt_y2);
    draw_line(txt_x, txt_y2 - 3, txt_x, txt_y2 + 3);

    // Draw a large rectangle with rounded corners
    // set_draw_mode(DRAW_INV);
    // fill_rectangle_rc(FB_WIDTH / 2,
    //                   FB_HEIGHT / 2,
    //                   (sin(frame / 100.0) + 1) * 150 + 60,
    //                   (sin(frame / 150.0) + 1) * 50 + 60,
    //                   radius,
    //                   0xFF);

    test_pattern();
    frame++;
}

void on_event_graphics(SDL_Event *e) {
    switch (e->type) {
    case SDL_KEYDOWN:
        // Cursor keys = horizontal alignment
        if (e->key.keysym.sym == SDLK_RIGHT) {
            printf("H_RIGHT\n");
            align = (align & ~0xF) | H_RIGHT;
        } else if (e->key.keysym.sym == SDLK_LEFT) {
            printf("H_LEFT\n");
            align = (align & ~0xF) | H_LEFT;
        } else if (e->key.keysym.sym == SDLK_UP) {
            printf("H_MIDDLE\n");
            align = (align & ~0xF) | H_MIDDLE;
            if (radius < 100)
                radius++;
        } else if (e->key.keysym.sym == SDLK_DOWN) {
            if (radius > 0)
                radius--;
        } else if (e->key.keysym.sym == '1') {
            // QAZX = vertical alignment
            printf("V_TOP\n");
            align = (align & ~0xF0) | V_TOP;
        } else if (e->key.keysym.sym == '2') {
            printf("V_MIDDLE\n");
            align = (align & ~0xF0) | V_MIDDLE;
        } else if (e->key.keysym.sym == '3') {
            printf("V_BASELINE\n");
            align = (align & ~0xF0) | V_BASELINE;
        } else if (e->key.keysym.sym == '4') {
            printf("V_BOTTOM\n");
            align = (align & ~0xF0) | V_BOTTOM;
        } else if (e->key.keysym.sym == SDLK_BACKSPACE && text_cursor > 0) {
            text_cursor--;
            test_str[text_cursor] = '\0';
        } else if (e->key.keysym.sym == SDLK_RETURN && text_cursor < (sizeof(test_str) - 1)) {
            test_str[text_cursor++] = '\n';
            test_str[text_cursor] = '\0';
        }
        break;
    case SDL_TEXTINPUT:
        // Add new text onto the end of our test string
        if (text_cursor < (sizeof(test_str) - 1)) {
            test_str[text_cursor++] = e->text.text[0];
            test_str[text_cursor] = '\0';
        }
        break;
    }
}
