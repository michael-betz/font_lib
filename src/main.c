#include "font.h"
#include "frame_buffer.h"
#include "graphics.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <fixed.h>
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

int main(int argc, char *args[]) {
    init_sdl();

    bool is_running = true, redraw = true;
    unsigned frame = 0, align = A_CENTER;

    char test_str[256] = "Hello World!\nType to edit :)\n";
    int text_cursor = strnlen(test_str, sizeof(test_str));
    init_from_header(&f_vollkorn);
    // init_from_header(&f_fixed);

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
                } else if (e.key.keysym.sym == SDLK_BACKSPACE && text_cursor > 0) {
                    text_cursor--;
                    test_str[text_cursor] = '\0';
                } else if (e.key.keysym.sym == SDLK_RETURN &&
                           text_cursor < (sizeof(test_str) - 1)) {
                    test_str[text_cursor++] = '\n';
                    test_str[text_cursor] = '\0';
                }
                redraw = true;
                break;
            case SDL_TEXTINPUT:
                // Add new text onto the end of our test string
                if (text_cursor < (sizeof(test_str) - 1)) {
                    test_str[text_cursor++] = e.text.text[0];
                    test_str[text_cursor] = '\0';
                }
                redraw = true;
                break;
            }
        }

        if (redraw) {
            fill(0);
            push_str(FB_WIDTH / 2, FB_HEIGHT / 4, test_str, sizeof(test_str), align);

            draw_line_aa(-8 + frame, 4, FB_WIDTH / 2, FB_HEIGHT);
            draw_line_aa(8 + FB_WIDTH - frame, 4, FB_WIDTH / 3, FB_HEIGHT);
            draw_line_aa(4, -8 + frame, FB_WIDTH, FB_HEIGHT / 2);
            draw_line_aa(4, 8 + FB_HEIGHT - frame, FB_WIDTH, FB_HEIGHT / 3);

            draw_ellipse_aa(FB_WIDTH / 2 - 8, FB_HEIGHT / 2, frame, 48, 0b1010);
            draw_ellipse_aa(FB_WIDTH / 2, FB_HEIGHT / 2, frame / 2, frame / 2, 0b0011);
            draw_ellipse_aa(FB_WIDTH / 2, FB_HEIGHT / 2, frame - 8, frame - 8, 0b1100);
            draw_ellipse_aa(FB_WIDTH / 2 + 8, FB_HEIGHT / 2, frame, 48, 0b0101);

            // Copy font_lib frame buffer to layer_a
            send_frame_buffer();

            // Compose the layers
            SDL_SetRenderTarget(rr, NULL);  // default backbuffer
            SDL_RenderSetScale(rr, ZOOM, ZOOM);
            SDL_RenderClear(rr);
            SDL_RenderCopy(rr, layer_a, NULL, NULL);
            SDL_RenderPresent(rr);
        }

        SDL_Delay(100);
        // redraw = false;
        frame++;
    }

    SDL_DestroyRenderer(rr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
