#include "font.h"
#include "frame_buffer.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define ZOOM 2

SDL_Renderer *rr = NULL;
SDL_Window *window = NULL;

// To show the display pixels in blue (persistent between frames)
SDL_Texture *layer_a;

// x1, y1, x2, y2: the rectangle to update in [pixels]
// x1, y1, x2 and y2 are all inclusive!
// note that ssd1322 works with columns of 4 pixels horizontally
// so the lower 2 bits of x1 and x2 will be truncated
// data in 4 bits / pixel, 2 pixels / byte
bool send_window_4(unsigned x1, unsigned y1, unsigned x2, unsigned y2, uint8_t *data) {
    // printf("send_window_4(%3d, %3d, %3d, %3d)\n", x1, y1, x2, y2);

    // truncate the 2 LSBs
    x1 = x1 & ~3;
    x2 = (x2 & ~3);

    // Show pixel values in blue in background layer
    SDL_SetRenderTarget(rr, layer_a);
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            uint8_t p = get_pixel(x, y);
            SDL_SetRenderDrawColor(rr, 0, 0, (p << 4) | p, 0xFF);
            SDL_RenderDrawPoint(rr, x, y);
        }
    }

    SDL_RenderDrawRect(rr, &rect);

    return true;
}

static void init_sdl() {
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "could not initialize SDL2: %s\n", SDL_GetError());
        return;
    }

    if (SDL_CreateWindowAndRenderer(DISPLAY_WIDTH * ZOOM, DISPLAY_HEIGHT * ZOOM, 0, &window, &rr)) {
        fprintf(stderr, "could not create window: %s\n", SDL_GetError());
        return;
    };

    layer_a = SDL_CreateTexture(
        rr, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    SDL_SetTextureBlendMode(layer_a, SDL_BLENDMODE_NONE);
}

int main(int argc, char *args[]) {
    init_sdl();

    while (is_running) {
        int encoder_value = 0;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                is_running = false;
                break;
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    encoder_value = -1;
                    break;
                case SDLK_RIGHT:
                    encoder_value = 1;
                    break;
                }
                break;
            }
        }

        screen_handler();

        // Compose the textures
        SDL_SetRenderTarget(rr, NULL);  // default backbuffer
        SDL_RenderSetScale(rr, ZOOM, ZOOM);
        SDL_RenderClear(rr);
        SDL_RenderCopy(rr, layer_a, NULL, NULL);
        SDL_RenderPresent(rr);

        SDL_Delay(5);
    }

    SDL_DestroyRenderer(rr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
