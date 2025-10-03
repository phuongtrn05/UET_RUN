#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>

void renderTextCentered(SDL_Renderer* renderer, const std::string& text, int x, int y, TTF_Font* currentFont, SDL_Color color) {
    if (text.empty() || !currentFont) return;

    SDL_Surface* surface = TTF_RenderText_Blended(currentFont, text.c_str(), text.length(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        return;
    }
    SDL_FRect destRect = { (float)(x - surface->w / 2), (float)(y - surface->h / 2), (float)surface->w, (float)surface->h };
    SDL_RenderTexture(renderer, texture, nullptr, &destRect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

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

    SDL_Window* window = SDL_CreateWindow("UET_RUN", 800, 600, 0);
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

    TTF_Font* font = TTF_OpenFont("NotoSans-Regular.ttf", 16);
    if (!font) {
        std::cout << "Failed to load fonts: " << SDL_GetError() << std::endl;
    }

    float logoW, logoH;
    SDL_GetTextureSize(logo, &logoW, &logoH);

    SDL_FRect logoRect = {200, 100, 400, 400};

    bool running = true;
    SDL_Event e;
    Uint8 alpha = 0;
    bool shrinking = false;
    Uint32 startTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
        }

        if (SDL_GetTicks() - startTime > 3000) {
            running = false;
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

    renderTextCentered(renderer, "Game", 400, 300, font, {255, 0, 0, 255});
    SDL_FRect border = {360, 260, 80, 60};
    SDL_SetRenderDrawColor(renderer, 200, 200, 0, 255);
    SDL_RenderRect(renderer, &border);
    SDL_RenderPresent(renderer);
    SDL_Delay(10000);

    SDL_DestroyTexture(logo);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
