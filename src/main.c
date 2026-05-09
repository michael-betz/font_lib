#include "font.h"
#include "fonts.h"
#include "frame_buffer.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define ZOOM 4

extern void on_event_graphics(SDL_Event *e);
extern void on_event_widget_gui(SDL_Event *e);
extern void test_graphics(void);
extern void test_widget_gui(void);

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
    static int test_mode = 0;
    init_sdl();

    bool is_running = true;

    while (is_running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                is_running = false;
                break;
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_F1)
                    test_mode = 0;
                if (e.key.keysym.sym == SDLK_F2)
                    test_mode = 1;
            }
            if (test_mode == 0)
                on_event_graphics(&e);
            else if (test_mode == 1)
                on_event_widget_gui(&e);
        }

        if (test_mode == 0)
            test_graphics();
        else if (test_mode == 1)
            test_widget_gui();

        // Copy font_lib frame buffer to layer_a
        send_frame_buffer();

        // Compose the layers
        SDL_SetRenderTarget(rr, NULL);  // default backbuffer
        SDL_RenderSetScale(rr, ZOOM, ZOOM);
        SDL_RenderClear(rr);
        SDL_RenderCopy(rr, layer_a, NULL, NULL);
        SDL_RenderPresent(rr);

        SDL_Delay(30);
    }

    SDL_DestroyRenderer(rr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
