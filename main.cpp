#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>

enum class Scene {
    MENU,
    PLAY,
    RESUME,
    SCORE
};

SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
TTF_Font* g_font = nullptr;
SDL_Texture* g_logoTexture = nullptr;
SDL_Texture* g_buttonTexture = nullptr;
SDL_FRect g_logoRect = {200, 100, 400, 400};
Uint8 g_alpha = 0;
bool g_shrinking = false;
Uint32 g_startTime = 0;
Scene g_currentScene = Scene::MENU;
struct Button {
    std::string text;
    SDL_FRect rect;
};

std::vector<Button> g_buttons = {
    {"Play",   {300, 200, 180, 80}},
    {"Resume", {300, 300, 180, 80}},
    {"Score",  {300, 400, 180, 80}}
};

// Hàm vẽ nút tròn với viền và chữ
void renderRoundedButton(SDL_Renderer* renderer, const Button& btn,
                           TTF_Font* font, SDL_Texture* bgTexture, SDL_Color borderColor, SDL_Color textColor)
{
    if (!renderer || !font || !bgTexture) return;

    // Vẽ ảnh nền nút (stretch cho vừa với rect)
    SDL_RenderTexture(renderer, bgTexture, nullptr, &btn.rect);

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

// Hàm kiểm tra va chạm (collision detection) giữa điểm và hình chữ nhật
bool checkCollision(float x, float y, const SDL_FRect& rect) {
    return (x >= rect.x && x < rect.x + rect.w &&
            y >= rect.y && y < rect.y + rect.h);
}

void renderScenePlay() {
    SDL_SetRenderDrawColor(g_renderer, 100, 200, 100, 255);
    SDL_RenderClear(g_renderer);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(g_font, "Cảnh Play: Bấm ESC để về Menu", 0, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FRect textRect = {50, 50, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void renderSceneResume() {
    SDL_SetRenderDrawColor(g_renderer, 150, 150, 255, 255);
    SDL_RenderClear(g_renderer);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(g_font, "Cảnh Resume: Bấm ESC để về Menu", 0, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FRect textRect = {50, 50, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }
}


void renderSceneScore() {
    SDL_SetRenderDrawColor(g_renderer, 255, 165, 0, 255);
    SDL_RenderClear(g_renderer);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(g_font, "Cảnh Score: Bấm ESC để về Menu", 0, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FRect textRect = {50, 50, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void renderSceneMenu(Uint32 currentTime) {
    // Bầu trời trắng
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    SDL_RenderClear(g_renderer);

    // Nền đất xanh dương nhạt
    SDL_FRect ground = {0, 500, 800, 100};
    SDL_SetRenderDrawColor(g_renderer, 173, 216, 230, 255);
    SDL_RenderFillRect(g_renderer, &ground);

    // Hiệu ứng fade-in logo
    if (g_alpha < 255 && !g_shrinking) {
        g_alpha = (Uint8)SDL_min(g_alpha + 3, 255);
        SDL_SetTextureAlphaMod(g_logoTexture, g_alpha);
    } else {
        g_shrinking = true;
    }

    // Thu nhỏ rồi trượt về góc trái
    if (g_shrinking) {
        if (g_logoRect.w > 100) {
            g_logoRect.w *= 0.98f;
            g_logoRect.h *= 0.98f;
            g_logoRect.x = (800 - g_logoRect.w) / 2;
            g_logoRect.y = (600 - g_logoRect.h) / 2;
        } else {
            g_logoRect.x -= (g_logoRect.x - 20) * 0.1f;
            g_logoRect.y -= (g_logoRect.y - 20) * 0.1f;
        }
    }

    SDL_RenderTexture(g_renderer, g_logoTexture, nullptr, &g_logoRect);

    if (currentTime - g_startTime > 4000) {
        SDL_Color borderColor = {255, 215, 0, 255}; // vàng nhạt
        SDL_Color textColor   = {255, 255, 255, 255};     // chữ trắng

        for (auto& b : g_buttons) {
            renderRoundedButton(g_renderer, b, g_font, g_buttonTexture, borderColor, textColor);
        }
    }
}

int main(int argc, char* argv[]) {
    // Khởi tạo SDL và các thư viện
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    if (TTF_Init() < 0) {
        std::cout << "TTF_Init failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    g_window = SDL_CreateWindow("UET_RUN", 800, 600, 0);
    g_renderer = SDL_CreateRenderer(g_window, nullptr);

    g_logoTexture = IMG_LoadTexture(g_renderer, "Assets/uet.png");
    if (!g_logoTexture) {
        std::cout << "Load image failed: " << SDL_GetError() << "\n";
        // Tiếp tục xử lý lỗi...
    }

    g_buttonTexture = IMG_LoadTexture(g_renderer, "Assets/button.png");
    if (!g_buttonTexture) {
        std::cout << "Failed to load button image: " << SDL_GetError() << "\n";
    }

    g_font = TTF_OpenFont("NotoSans-Regular.ttf", 36);
    if (!g_font) {
        std::cout << "Failed to load font: " << SDL_GetError() << "\n";
    }

    g_startTime = SDL_GetTicks();

    bool running = true;
    SDL_Event e;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();

        // Xử lý sự kiện
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) {
                // Quay về Menu khi bấm ESC ở các cảnh khác
                if (g_currentScene != Scene::MENU) {
                    g_currentScene = Scene::MENU;
                } else {
                    running = false;
                }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                // Xử lý click chuột
                if (g_currentScene == Scene::MENU && currentTime - g_startTime > 4000) {
                    float mouseX = (float)e.button.x;
                    float mouseY = (float)e.button.y;
                    // Kiểm tra nút Play
                    if (checkCollision(mouseX, mouseY, g_buttons[0].rect)) {
                        g_currentScene = Scene::PLAY;
                    }
                    // Kiểm tra nút Resume
                    else if (checkCollision(mouseX, mouseY, g_buttons[1].rect)) {
                        g_currentScene = Scene::RESUME;
                    }
                    // Kiểm tra nút Score
                    else if (checkCollision(mouseX, mouseY, g_buttons[2].rect)) {
                        g_currentScene = Scene::SCORE;
                    }
                }
            }
        }

        // Render (Vẽ) cảnh hiện tại
        switch (g_currentScene) {
            case Scene::MENU:
                renderSceneMenu(currentTime);
                break;
            case Scene::PLAY:
                renderScenePlay();
                break;
            case Scene::RESUME:
                renderSceneResume();
                break;
            case Scene::SCORE:
                renderSceneScore();
                break;
        }

        SDL_RenderPresent(g_renderer);
        SDL_Delay(16); // Giới hạn khoảng 60 FPS
    }

    // Dọn dẹp
    if (g_font) TTF_CloseFont(g_font);
    if (g_logoTexture) SDL_DestroyTexture(g_logoTexture);
    if (g_buttonTexture) SDL_DestroyTexture(g_buttonTexture);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);

    TTF_Quit();
    SDL_Quit();
    return 0;
}
