#include "font.h"
#include "frame_buffer.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <spleen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define ZOOM 4

SDL_Renderer *rr = NULL;
SDL_Window *window = NULL;

// To show the display pixels in blue (persistent between frames)
SDL_Texture *layer_a;

bool send_frame_buffer() {
    // Show pixel values in blue in background layer
    SDL_SetRenderTarget(rr, layer_a);
    for (int y = 0; y < FB_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            uint8_t p = get_pixel(x, y);
            SDL_SetRenderDrawColor(rr, 0, 0, p, 0xFF);
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

// extern const font_header_t f_vollkorn;

int main(int argc, char *args[]) {
    init_sdl();

    bool is_running = true;
    unsigned frame = 0;

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

        // Draw to font_lib frame buffer.
        printf("%d\n", frame);

        if (frame == 0) {
            init_from_header(&f_spleen);
            set_draw_mode(DRAW_ADD);
            push_str(FB_WIDTH / 2, FB_HEIGHT / 2 - 8, "Hel(l)o Wo[r]ld", 99, A_CENTER);
            push_str(FB_WIDTH / 2, FB_HEIGHT / 2 + 8, "Q^uatschuQuench!!", 99, A_CENTER);
        }

        // Copy font_lib frame buffer to sdl texture
        send_frame_buffer();

        // Compose the texture
        SDL_SetRenderTarget(rr, NULL);  // default backbuffer
        SDL_RenderSetScale(rr, ZOOM, ZOOM);
        SDL_RenderClear(rr);
        SDL_RenderCopy(rr, layer_a, NULL, NULL);
        SDL_RenderPresent(rr);

        SDL_Delay(1000);
        frame++;
    }

    SDL_DestroyRenderer(rr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
