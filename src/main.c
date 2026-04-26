#include "font.h"
#include "frame_buffer.h"
#include "graphics.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <fixed.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <vollkorn.h>

#define ZOOM 4

SDL_Renderer *rr = NULL;
SDL_Window *window = NULL;

// To show the display pixels in blue
SDL_Texture *layer_a;

bool send_frame_buffer() {
    // Show pixel values in blue in background layer
    SDL_SetRenderTarget(rr, layer_a);
    for (int y = 0; y < FB_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            uint8_t p = get_pixel(x, y);
            SDL_SetRenderDrawColor(rr, p, p, p, 0xFF);
            SDL_RenderDrawPoint(rr, x, y);
        }
    }
}

static void init_sdl() {
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "could not initialize SDL2: %s\n", SDL_GetError());
        return;
    }

    if (SDL_CreateWindowAndRenderer(FB_WIDTH * ZOOM, FB_HEIGHT * ZOOM, 0, &window, &rr)) {
        fprintf(stderr, "could not create window: %s\n", SDL_GetError());
        return;
    };

    layer_a = SDL_CreateTexture(
        rr, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, FB_WIDTH, FB_HEIGHT);

    SDL_SetTextureBlendMode(layer_a, SDL_BLENDMODE_NONE);
}

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

int main(int argc, char *args[]) {
    init_sdl();

    bool is_running = true;
    unsigned frame = 0, align = A_CENTER, radius = 30;

    char test_str[256] = "Hello World!\nType to edit :)\n";
    int text_cursor = strnlen(test_str, sizeof(test_str));

    init_from_header(&f_vollkorn);
    print_font_info();

    while (is_running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                is_running = false;
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_LEFT) {
                    align = A_RIGHT;
                } else if (e.key.keysym.sym == SDLK_RIGHT) {
                    align = A_LEFT;
                } else if (e.key.keysym.sym == SDLK_UP) {
                    align = A_CENTER;
                    if (radius < 100)
                        radius++;
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    if (radius > 0)
                        radius--;
                } else if (e.key.keysym.sym == SDLK_BACKSPACE && text_cursor > 0) {
                    text_cursor--;
                    test_str[text_cursor] = '\0';
                } else if (e.key.keysym.sym == SDLK_RETURN &&
                           text_cursor < (sizeof(test_str) - 1)) {
                    test_str[text_cursor++] = '\n';
                    test_str[text_cursor] = '\0';
                }
                break;
            case SDL_TEXTINPUT:
                // Add new text onto the end of our test string
                if (text_cursor < (sizeof(test_str) - 1)) {
                    test_str[text_cursor++] = e.text.text[0];
                    test_str[text_cursor] = '\0';
                }
                break;
            }
        }

        fill(0x20);

        // Draw in a big anti-aliased font
        set_draw_mode(DRAW_ADD);
        init_from_header(&f_vollkorn);
        push_str(FB_WIDTH / 2, 50, test_str, sizeof(test_str), align);

        // Draw the bounding box
        int left = 0, right = 0, bottom = 0, top = 0;
        fnt_get_bb(test_str, sizeof(test_str), align, &left, &right, &top, &bottom);
        draw_rectangle(
            FB_WIDTH / 2 + left - 1, 50 + top - 1, FB_WIDTH / 2 + right, 50 + bottom, 0x44);

        // Draw in a small pixel font
        init_from_header(&f_fixed);
        push_str(FB_WIDTH / 2, 150, test_str, sizeof(test_str), align);

        // Draw the bounding box
        fnt_get_bb(test_str, sizeof(test_str), align, &left, &right, &top, &bottom);
        draw_rectangle(
            FB_WIDTH / 2 + left - 1, 150 + top - 1, FB_WIDTH / 2 + right, 150 + bottom, 0x44);

        // Draw a large rectangle with rounded corners
        set_draw_mode(DRAW_INV);
        fill_rectangle_rc(FB_WIDTH / 2,
                          FB_HEIGHT / 2,
                          (sin(frame / 100.0) + 1) * 150 + 60,
                          (sin(frame / 150.0) + 1) * 50 + 60,
                          radius,
                          0xFF);

        test_pattern();

        // Copy font_lib frame buffer to layer_a
        send_frame_buffer();

        // Compose the layers
        SDL_SetRenderTarget(rr, NULL);  // default backbuffer
        SDL_RenderSetScale(rr, ZOOM, ZOOM);
        SDL_RenderClear(rr);
        SDL_RenderCopy(rr, layer_a, NULL, NULL);
        SDL_RenderPresent(rr);

        SDL_Delay(30);
        frame++;
    }

    SDL_DestroyRenderer(rr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
