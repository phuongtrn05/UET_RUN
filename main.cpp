#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>

struct Button {
    std::string text;
    SDL_FRect rect;
};

void renderRoundedButton(SDL_Renderer* renderer, const Button& btn,
                           TTF_Font* font, SDL_Texture* bgTexture, SDL_Color borderColor, SDL_Color textColor)
{
    // Vẽ ảnh nền nút (stretch cho vừa với rect)
    SDL_RenderTexture(renderer, bgTexture, nullptr, &btn.rect);

    // Viền vàng nhạt
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);

    // Hiển thị chữ
    SDL_Surface* surface = TTF_RenderText_Blended(font, btn.text.c_str(), btn.text.length(), textColor);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_FRect textRect = {
        btn.rect.x + (btn.rect.w - surface->w) / 2.0f,
        btn.rect.y + (btn.rect.h - surface->h) / 2.0f,
        (float)surface->w,
        (float)surface->h
    };

    SDL_RenderTexture(renderer, texture, nullptr, &textRect);
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

    SDL_Texture* logo = IMG_LoadTexture(renderer, "Assets/uet.png");
    if (!logo) {
        std::cout << "Load image failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Texture* buttonTexture = IMG_LoadTexture(renderer, "Assets/button.png");
    if (!buttonTexture) {
        std::cout << "Failed to load button image: " << SDL_GetError() << "\n";
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("NotoSans-Regular.ttf", 36);
    if (!font) {
        std::cout << "Failed to load font: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_FRect logoRect = {200, 100, 400, 400};
    Uint8 alpha = 0;
    bool shrinking = false;
    Uint32 startTime = SDL_GetTicks();

    bool running = true;
    SDL_Event e;

    std::vector<Button> buttons = {
        {"Play",   {300, 200, 180, 80}},
        {"Resume", {300, 300, 180, 80}},
        {"Score",  {300, 400, 180, 80}}
    };

    SDL_Color borderColor = {255, 215, 0, 255}; // vàng nhạt
    SDL_Color textColor   = {255, 255, 255, 255};     // chữ đen

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT || (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE))
                running = false;
        }

        // Bầu trời trắng
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Nền đất xanh dương nhạt
        SDL_FRect ground = {0, 500, 800, 100};
        SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255);
        SDL_RenderFillRect(renderer, &ground);

        // Hiệu ứng fade-in logo
        if (alpha < 255 && !shrinking) {
            alpha = (Uint8)SDL_min(alpha + 3, 255);
            SDL_SetTextureAlphaMod(logo, alpha);
        } else {
            shrinking = true;
        }

        // Thu nhỏ rồi trượt về góc trái
        if (shrinking) {
            if (logoRect.w > 100) {
                logoRect.w *= 0.98f;
                logoRect.h *= 0.98f;
                logoRect.x = (800 - logoRect.w) / 2;
                logoRect.y = (600 - logoRect.h) / 2;
            } else {
                logoRect.x -= (logoRect.x - 20) * 0.1f;
                logoRect.y -= (logoRect.y - 20) * 0.1f;
            }
        }

        SDL_RenderTexture(renderer, logo, nullptr, &logoRect);

        // Sau 4 giây: hiện menu đẹp
        if (SDL_GetTicks() - startTime > 4000) {
            for (auto& b : buttons) {
                renderRoundedButton(renderer, b, font, buttonTexture, borderColor, textColor);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    SDL_DestroyTexture(logo);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
