#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>

int main(int argc, char* argv[]) {
    // Init SDL
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

    // Load logo (PNG tự động hỗ trợ, không cần IMG_Init)
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

    // Vị trí ban đầu (giữa màn hình)
    SDL_FRect logoRect = {400 - logoW / 2, 300 - logoH / 2, logoW, logoH};

    bool running = true;
    SDL_Event e;
    Uint8 alpha = 0;   // fade-in
    bool shrinking = false;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
        }

        // Nền xanh trời + mặt đất trắng
        SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
        SDL_RenderClear(renderer);

        SDL_FRect ground = {0, 500, 800, 100};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &ground);

        // Fade in
        if (alpha < 255 && !shrinking) {
            alpha += 3;
            SDL_SetTextureAlphaMod(logo, alpha);
        }

        // Sau khi fade-in xong, thu nhỏ logo
        if (alpha >= 255) {
            shrinking = true;
        }
        if (shrinking && logoRect.w > 100) {
            logoRect.w *= 0.98f;
            logoRect.h *= 0.98f;
            logoRect.x = 20; // dịch sang trái
            logoRect.y = 20; // dịch lên trên
        }

        SDL_RenderTexture(renderer, logo, nullptr, &logoRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    SDL_DestroyTexture(logo);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
