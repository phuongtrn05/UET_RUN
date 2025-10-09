#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>

enum class Scene {
    MENU,
    PLAY,
    RESUME,
    SCORE,
    FINISH
};

// Cấu trúc cho nhân vật
struct Player {
    SDL_FRect rect;
    float velocityX;
    float velocityY;
    bool isJumping;
    bool onGround;
    SDL_Color color;

    Player() : rect{100, 400, 100, 120}, velocityX(0), velocityY(0),
               isJumping(false), onGround(false),
               color{255, 100, 100, 255} {}
};

// Cấu trúc cho chướng ngại vật
struct Obstacle {
    SDL_FRect rect;
    SDL_Color color;
    bool isPassed; // Đã vượt qua chưa

    Obstacle(float x, float y, float w, float h, SDL_Color c)
        : rect{x, y, w, h}, color(c), isPassed(false) {}
};

SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
TTF_Font* g_font = nullptr;
TTF_Font* g_smallFont = nullptr;
SDL_Texture* g_logoTexture = nullptr;
SDL_Texture* g_buttonTexture = nullptr;
SDL_Texture* g_player = nullptr;

SDL_FRect g_logoRect = {200, 100, 400, 400};
Uint8 g_alpha = 0;
bool g_shrinking = false;
Uint32 g_startTime = 0;
Scene g_currentScene = Scene::MENU;
Player player;

// Camera
float g_cameraX = 0.0f;

// Các hằng số vật lý
const float GRAVITY = 0.8f;
const float JUMP_FORCE = -15.0f;
const float MOVE_SPEED = 5.0f;
const float GROUND_Y = 460.0f;
const float TRACK_LENGTH = 10000.0f; // 1000m (tỉ lệ 10 pixel = 1m)
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Danh sách chướng ngại vật
std::vector<Obstacle> g_obstacles;

// Điểm số và thời gian
int g_score = 0;
Uint32 g_gameStartTime = 0;

struct Button {
    std::string text;
    SDL_FRect rect;
};

std::vector<Button> g_buttons = {
    {"Play",   {300, 200, 180, 80}},
    {"Resume", {300, 300, 180, 80}},
    {"Score",  {300, 400, 180, 80}}
};

// Hàm tạo chướng ngại vật
void createObstacles() {
    g_obstacles.clear();

    // Tạo các chướng ngại vật dọc đường
    for (float x = 800; x < TRACK_LENGTH; x += 300 + (rand() % 200)) {
        int obstacleType = rand() % 3;

        switch (obstacleType) {
            case 0: // Hộp nhỏ
                g_obstacles.push_back(Obstacle(
                    x, GROUND_Y - 60, 60, 60,
                    {139, 69, 19, 255} // Nâu
                ));
                break;
            case 1: // Hộp cao
                g_obstacles.push_back(Obstacle(
                    x, GROUND_Y - 100, 50, 100,
                    {128, 128, 128, 255} // Xám
                ));
                break;
            case 2: // Hộp rộng thấp
                g_obstacles.push_back(Obstacle(
                    x, GROUND_Y - 40, 100, 40,
                    {160, 82, 45, 255} // Nâu đỏ
                ));
                break;
        }
    }

    // Thêm vạch đích
    g_obstacles.push_back(Obstacle(
        TRACK_LENGTH + 100, GROUND_Y - 200, 20, 200,
        {255, 215, 0, 255} // Vàng
    ));
}

// Hàm vẽ nút tròn với viền và chữ
void renderRoundedButton(SDL_Renderer* renderer, const Button& btn,
                           TTF_Font* font, SDL_Texture* bgTexture, SDL_Color borderColor, SDL_Color textColor)
{
    if (!renderer || !font || !bgTexture) return;

    SDL_RenderTexture(renderer, bgTexture, nullptr, &btn.rect);

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

// Hàm kiểm tra va chạm điểm với hình chữ nhật
bool checkCollision(float x, float y, const SDL_FRect& rect) {
    return (x >= rect.x && x < rect.x + rect.w &&
            y >= rect.y && y < rect.y + rect.h);
}

// Hàm kiểm tra va chạm giữa 2 hình chữ nhật
bool checkRectCollision(const SDL_FRect& a, const SDL_FRect& b) {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}

// Hàm cập nhật vật lý cho nhân vật
void updatePlayer() {
    // Áp dụng trọng lực
    player.velocityY += GRAVITY;

    // Cập nhật vị trí
    player.rect.x += player.velocityX;
    player.rect.y += player.velocityY;

    // Kiểm tra va chạm với mặt đất
    if (player.rect.y + player.rect.h >= GROUND_Y) {
        player.rect.y = GROUND_Y - player.rect.h;
        player.velocityY = 0;
        player.onGround = true;
        player.isJumping = false;
    } else {
        player.onGround = false;
    }

    // Giới hạn không cho lùi lại
    if (player.rect.x < g_cameraX) {
        player.rect.x = g_cameraX;
        player.velocityX = 0;
    }

    // Kiểm tra va chạm với chướng ngại vật
    for (auto& obs : g_obstacles) {
        if (checkRectCollision(player.rect, obs.rect)) {
            // Va chạm từ trên xuống
            if (player.velocityY > 0 &&
                player.rect.y + player.rect.h - player.velocityY <= obs.rect.y) {
                player.rect.y = obs.rect.y - player.rect.h;
                player.velocityY = 0;
                player.onGround = true;
                player.isJumping = false;
            }
            // Va chạm từ bên trái (đâm vào)
            else if (player.velocityX > 0) {
                player.rect.x = obs.rect.x - player.rect.w;
                player.velocityX = 0;
            }
        }

        // Tính điểm khi vượt qua chướng ngại vật
        if (!obs.isPassed && player.rect.x > obs.rect.x + obs.rect.w) {
            obs.isPassed = true;
            g_score += 10;
        }
    }

    // Cập nhật camera theo nhân vật
    g_cameraX = player.rect.x - 200;
    if (g_cameraX < 0) g_cameraX = 0;

    // Giảm tốc độ di chuyển ngang (ma sát)
    player.velocityX *= 0.9f;

    // Kiểm tra đến đích
    if (player.rect.x >= TRACK_LENGTH) {
        g_currentScene = Scene::FINISH;
    }
}

void renderScenePlay() {
    // Vẽ bầu trời
    SDL_SetRenderDrawColor(g_renderer, 135, 206, 250, 255);
    SDL_RenderClear(g_renderer);

    // Vẽ mặt đất (dài theo track)
    SDL_FRect ground = {-g_cameraX, GROUND_Y, TRACK_LENGTH, 140};
    SDL_SetRenderDrawColor(g_renderer, 34, 139, 34, 255);
    SDL_RenderFillRect(g_renderer, &ground);

    // Vẽ vạch kẻ đường (mỗi 100 pixel)
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    for (float x = 0; x < TRACK_LENGTH; x += 100) {
        SDL_FRect line = {x - g_cameraX, GROUND_Y + 60, 50, 5};
        SDL_RenderFillRect(g_renderer, &line);
    }

    // Vẽ chướng ngại vật
    for (const auto& obs : g_obstacles) {
        SDL_FRect renderRect = {
            obs.rect.x - g_cameraX,
            obs.rect.y,
            obs.rect.w,
            obs.rect.h
        };
        SDL_SetRenderDrawColor(g_renderer, obs.color.r, obs.color.g, obs.color.b, obs.color.a);
        SDL_RenderFillRect(g_renderer, &renderRect);

        // Vẽ viền
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
        SDL_RenderRect(g_renderer, &renderRect);
    }

    // Cập nhật và vẽ nhân vật
    updatePlayer();
    SDL_FRect playerRenderRect = {
        player.rect.x - g_cameraX,
        player.rect.y,
        player.rect.w,
        player.rect.h
    };
    SDL_RenderTexture(g_renderer, g_player, nullptr, &playerRenderRect);

    // Hiển thị thông tin
    SDL_Color textColor = {255, 255, 255, 255};

    // Điểm số
    std::string scoreText = "Score: " + std::to_string(g_score);
    SDL_Surface* scoreSurface = TTF_RenderText_Blended(g_smallFont, scoreText.c_str(), scoreText.length(), textColor);
    if (scoreSurface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, scoreSurface);
        SDL_FRect textRect = {10, 10, (float)scoreSurface->w, (float)scoreSurface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(scoreSurface);
        SDL_DestroyTexture(texture);
    }

    // Khoảng cách
    int distance = (int)(player.rect.x / 10); // Chuyển pixel thành mét
    std::string distText = "Distance: " + std::to_string(distance) + "m / 1000m";
    SDL_Surface* distSurface = TTF_RenderText_Blended(g_smallFont, distText.c_str(), distText.length(), textColor);
    if (distSurface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, distSurface);
        SDL_FRect textRect = {10, 40, (float)distSurface->w, (float)distSurface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(distSurface);
        SDL_DestroyTexture(texture);
    }

    // Hướng dẫn
    SDL_Surface* surface = TTF_RenderText_Blended(g_smallFont, "A/D: Di chuyen, W/Space: Nhay, ESC: Ve Menu", 0, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FRect textRect = {10, 70, (float)surface->w * 0.8f, (float)surface->h * 0.8f};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void renderSceneFinish() {
    SDL_SetRenderDrawColor(g_renderer, 255, 215, 0, 255);
    SDL_RenderClear(g_renderer);

    SDL_Color textColor = {0, 0, 0, 255};

    // Chúc mừng
    SDL_Surface* surface1 = TTF_RenderText_Blended(g_font, "CHUC MUNG!", 0, textColor);
    if (surface1) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface1);
        SDL_FRect textRect = {SCREEN_WIDTH/2 - surface1->w/2.0f, 100, (float)surface1->w, (float)surface1->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface1);
        SDL_DestroyTexture(texture);
    }

    // Điểm số
    std::string scoreText = "Diem so: " + std::to_string(g_score);
    SDL_Surface* surface2 = TTF_RenderText_Blended(g_font, scoreText.c_str(), scoreText.length(), textColor);
    if (surface2) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface2);
        SDL_FRect textRect = {SCREEN_WIDTH/2 - surface2->w/2.0f, 200, (float)surface2->w, (float)surface2->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface2);
        SDL_DestroyTexture(texture);
    }

    // Thời gian
    Uint32 timeTaken = (SDL_GetTicks() - g_gameStartTime) / 1000;
    std::string timeText = "Thoi gian: " + std::to_string(timeTaken) + "s";
    SDL_Surface* surface3 = TTF_RenderText_Blended(g_smallFont, timeText.c_str(), timeText.length(), textColor);
    if (surface3) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface3);
        SDL_FRect textRect = {SCREEN_WIDTH/2 - surface3->w/2.0f, 300, (float)surface3->w, (float)surface3->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface3);
        SDL_DestroyTexture(texture);
    }

    // Hướng dẫn
    SDL_Surface* surface4 = TTF_RenderText_Blended(g_smallFont, "Bam ESC de ve Menu", 0, textColor);
    if (surface4) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface4);
        SDL_FRect textRect = {SCREEN_WIDTH/2 - surface4->w/2.0f, 400, (float)surface4->w, (float)surface4->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface4);
        SDL_DestroyTexture(texture);
    }
}

void renderSceneResume() {
    SDL_SetRenderDrawColor(g_renderer, 150, 150, 255, 255);
    SDL_RenderClear(g_renderer);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(g_font, "Canh Resume: Bam ESC de ve Menu", 0, textColor);
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
    SDL_Surface* surface = TTF_RenderText_Blended(g_font, "Canh Score: Bam ESC de ve Menu", 0, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FRect textRect = {50, 50, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void renderSceneMenu(Uint32 currentTime) {
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    SDL_RenderClear(g_renderer);

    SDL_FRect ground = {0, 500, 800, 100};
    SDL_SetRenderDrawColor(g_renderer, 173, 216, 230, 255);
    SDL_RenderFillRect(g_renderer, &ground);

    if (g_alpha < 255 && !g_shrinking) {
        g_alpha = (Uint8)SDL_min(g_alpha + 3, 255);
        SDL_SetTextureAlphaMod(g_logoTexture, g_alpha);
    } else {
        g_shrinking = true;
    }

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
        SDL_Color borderColor = {255, 215, 0, 255};
        SDL_Color textColor   = {255, 255, 255, 255};

        for (auto& b : g_buttons) {
            renderRoundedButton(g_renderer, b, g_font, g_buttonTexture, borderColor, textColor);
        }
    }
}

// Hàm reset nhân vật khi vào scene Play
void resetPlayer() {
    player.rect.x = 100;
    player.rect.y = 400;
    player.velocityX = 0;
    player.velocityY = 0;
    player.isJumping = false;
    player.onGround = false;
    g_cameraX = 0;
    g_score = 0;
    g_gameStartTime = SDL_GetTicks();
    createObstacles();
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    if (TTF_Init() < 0) {
        std::cout << "TTF_Init failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    g_window = SDL_CreateWindow("UET_RUN", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    g_renderer = SDL_CreateRenderer(g_window, nullptr);

    g_logoTexture = IMG_LoadTexture(g_renderer, "Assets/uet.png");
    if (!g_logoTexture) {
        std::cout << "Load image failed: " << SDL_GetError() << "\n";
    }

    g_buttonTexture = IMG_LoadTexture(g_renderer, "Assets/button.png");
    if (!g_buttonTexture) {
        std::cout << "Failed to load button image: " << SDL_GetError() << "\n";
    }

    g_player = IMG_LoadTexture(g_renderer, "Assets/player.png");
    if (!g_player) {
        std::cout << "Failed to load player image: " << SDL_GetError() << "\n";
    }

    g_font = TTF_OpenFont("NotoSans-Regular.ttf", 36);
    if (!g_font) {
        std::cout << "Failed to load font: " << SDL_GetError() << "\n";
    }

    g_smallFont = TTF_OpenFont("NotoSans-Regular.ttf", 24);
    if (!g_smallFont) {
        std::cout << "Failed to load small font: " << SDL_GetError() << "\n";
    }

    g_startTime = SDL_GetTicks();

    bool running = true;
    SDL_Event e;
    const bool* keystate = SDL_GetKeyboardState(nullptr);

    while (running) {
        Uint32 currentTime = SDL_GetTicks();

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) {
                    if (g_currentScene != Scene::MENU) {
                        g_currentScene = Scene::MENU;
                    } else {
                        running = false;
                    }
                }
                else if (g_currentScene == Scene::PLAY) {
                    if ((e.key.key == SDLK_W || e.key.key == SDLK_SPACE) && player.onGround) {
                        player.velocityY = JUMP_FORCE;
                        player.isJumping = true;
                        player.onGround = false;
                    }
                }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                if (g_currentScene == Scene::MENU && currentTime - g_startTime > 4000) {
                    float mouseX = (float)e.button.x;
                    float mouseY = (float)e.button.y;
                    if (checkCollision(mouseX, mouseY, g_buttons[0].rect)) {
                        g_currentScene = Scene::PLAY;
                        resetPlayer();
                    }
                    else if (checkCollision(mouseX, mouseY, g_buttons[1].rect)) {
                        g_currentScene = Scene::RESUME;
                    }
                    else if (checkCollision(mouseX, mouseY, g_buttons[2].rect)) {
                        g_currentScene = Scene::SCORE;
                    }
                }
            }
        }

        if (g_currentScene == Scene::PLAY) {
            if (keystate[SDL_SCANCODE_A]) {
                player.velocityX = -MOVE_SPEED;
            }
            if (keystate[SDL_SCANCODE_D]) {
                player.velocityX = MOVE_SPEED;
            }
        }

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
            case Scene::FINISH:
                renderSceneFinish();
                break;
        }

        SDL_RenderPresent(g_renderer);
        SDL_Delay(16);
    }

    if (g_font) TTF_CloseFont(g_font);
    if (g_smallFont) TTF_CloseFont(g_smallFont);
    if (g_logoTexture) SDL_DestroyTexture(g_logoTexture);
    if (g_buttonTexture) SDL_DestroyTexture(g_buttonTexture);
    if (g_player) SDL_DestroyTexture(g_player);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);

    TTF_Quit();
    SDL_Quit();
    return 0;
}
