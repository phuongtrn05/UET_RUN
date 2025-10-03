#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    if (!TTF_Init()) {
        std::cout << "TTF_Init failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("2048 UET", 800, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    if (!renderer) {
        std::cout << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* logo = IMG_LoadTexture(renderer, "Assets/uet.png");
    if (!logo) {
        std::cout << "Load image failed: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    float logoW, logoH;
    SDL_GetTextureSize(logo, &logoW, &logoH);

    SDL_FRect logoRect = {200, 100, 400, 400};

    bool running = true;
    SDL_Event e;
    Uint8 alpha = 0;
    bool shrinking = false;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
        }

        // Bầu trời trắng
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Đất xanh dương pastel (light blue)
        SDL_FRect ground = {0, 500, 800, 100};
        SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
        SDL_RenderFillRect(renderer, &ground);

        // Hiệu ứng fade-in logo
        if (alpha < 255 && !shrinking) {
            alpha = (Uint8)SDL_min(alpha + 3, 255);
            SDL_SetTextureAlphaMod(logo, alpha);
        }

        if (alpha >= 255) {
            shrinking = true;
        }
        if (shrinking && logoRect.w > 100) {
            logoRect.w *= 0.98f;
            logoRect.h *= 0.98f;
            logoRect.x = 20;
            logoRect.y = 20;
        }

        SDL_RenderTexture(renderer, logo, nullptr, &logoRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(logo);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
