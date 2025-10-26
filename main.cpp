#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath> // Cần cho powf
#include <ctime> // Cần cho time()

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
    int hp;
    SDL_Color color;

    Player() : rect{100, 400, 100, 120}, velocityX(0), velocityY(0),
             isJumping(false), onGround(false), hp(100),
             color{255, 100, 100, 255} {}
};

// Cấu trúc cho chướng ngại vật
struct Obstacle {
    SDL_FRect rect;
    SDL_Color color;
    bool isPassed;

    Obstacle(float x, float y, float w, float h, SDL_Color c)
        : rect{x, y, w, h}, color(c), isPassed(false) {}
};

// Cấu trúc cho vật phẩm thu thập
struct Collectible {
    SDL_FRect rect;
    SDL_Color color;
    bool isCollected;

    Collectible(float x, float y, float w, float h, SDL_Color c)
        : rect{x, y, w, h}, color(c), isCollected(false) {}
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

Uint64 g_lastTime = 0; // Thời điểm của khung hình trước

// Camera
float g_cameraX = 0.0f;

// Hằng số vật lý (pixel/giây)
const float GRAVITY = 0.8f * 60.0f * 60.0f;     // ~2880 (pixel/s^2)
const float JUMP_FORCE = -15.0f * 60.0f;    // -900 (pixel/s)
const float MOVE_SPEED = 5.0f * 60.0f;      // 300 (pixel/s)

const float GROUND_Y = 460.0f;
const float TRACK_LENGTH = 10000.0f;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Danh sách
std::vector<Obstacle> g_obstacles;
std::vector<Collectible> g_collectibles;

// Điểm số, level và vật phẩm
int g_score = 0;
int g_level = 1;
int g_itemCount = 0;
const int ITEMS_PER_LEVEL = 10;
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
    for (float x = 800; x < TRACK_LENGTH; x += 300 + (rand() % 200)) {
        int obstacleType = rand() % 3;
        switch (obstacleType) {
            case 0: // Hộp nhỏ
                g_obstacles.push_back(Obstacle(x, GROUND_Y - 60, 60, 60, {139, 69, 19, 255}));
                break;
            case 1: // Hộp cao
                g_obstacles.push_back(Obstacle(x, GROUND_Y - 100, 50, 100, {128, 128, 128, 255}));
                break;
            case 2: // Hộp rộng thấp
                g_obstacles.push_back(Obstacle(x, GROUND_Y - 40, 100, 40, {160, 82, 45, 255}));
                break;
        }
    }
    // Vạch đích
    g_obstacles.push_back(Obstacle(TRACK_LENGTH + 100, GROUND_Y - 200, 20, 200, {255, 215, 0, 255}));
}

// Hàm tạo vật phẩm
void createCollectibles() {
    g_collectibles.clear();
    srand((unsigned int)time(NULL));
    for (float x = 600; x < TRACK_LENGTH; x += 200 + (rand() % 150)) {
        float yPos = (rand() % 2 == 0) ? (GROUND_Y - 40) : (GROUND_Y - 160);
        g_collectibles.push_back(Collectible(x, yPos, 30, 30, {255, 255, 0, 255}));
    }
}

// Hàm vẽ nút
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


void updatePlayer(float dt) {
    // Áp dụng trọng lực
    player.velocityY += GRAVITY * dt;

    // Cập nhật vị trí
    player.rect.x += player.velocityX * dt;
    player.rect.y += player.velocityY * dt;

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

    // === SỬA LỖI VA CHẠM LOGIC ===
    // Kiểm tra va chạm với chướng ngại vật
    for (auto& obs : g_obstacles) {
        if (checkRectCollision(player.rect, obs.rect)) {

            // 1. Xử lý va chạm KHI ĐÁP XUỐNG (Va chạm đỉnh)
            // "prev_bottom" là vị trí đáy của player ở khung hình trước
            float prev_bottom = (player.rect.y + player.rect.h) - (player.velocityY * dt);

            // Nếu đang rơi (hoặc vận tốc Y = 0) VÀ ở khung hình trước chân bạn ở trên đỉnh vật cản
            if (player.velocityY >= 0 &&
                prev_bottom <= obs.rect.y + 1.0f) // +1.0f để xử lý sai số float
            {
                player.rect.y = obs.rect.y - player.rect.h; // Đặt player lên trên
                player.velocityY = 0;
                player.onGround = true;
                player.isJumping = false;
            }

            // 2. Xử lý va chạm KHI ĐÂM VÀO HÔNG (Va chạm bên trái)
            // Chỉ kích hoạt nếu KHÔNG phải là va chạm đỉnh (else if)
            // VÀ player đang đi sang phải
            // VÀ chân player (player.rect.y + player.rect.h) ở dưới đỉnh của vật cản (obs.rect.y)
            else if (player.velocityX > 0 &&
                     (player.rect.y + player.rect.h) > obs.rect.y + 1.0f)
            {
                player.rect.x = obs.rect.x - player.rect.w; // Đẩy lùi về bên trái
                player.velocityX = 0;
            }
        }
        // === KẾT THÚC SỬA LỖI ===

        // Tính điểm khi vượt qua chướng ngại vật
        if (!obs.isPassed && player.rect.x > obs.rect.x + obs.rect.w) {
            obs.isPassed = true;
            g_score += 10;
        }
    }

    // Kiểm tra va chạm với vật phẩm
    for (auto& item : g_collectibles) {
        if (!item.isCollected && checkRectCollision(player.rect, item.rect)) {
            item.isCollected = true;
            g_itemCount++;
            g_score += 5;

            // Kiểm tra level up
            if (g_itemCount >= ITEMS_PER_LEVEL) {
                g_level++;
                g_itemCount = 0;
                std::cout << "LEVEL UP! Chuc mung ban len level " << g_level << std::endl;
            }
        }
    }

    // Cập nhật camera theo nhân vật
    g_cameraX = player.rect.x - 200;
    if (g_cameraX < 0) g_cameraX = 0;

    // Giảm tốc độ di chuyển ngang (ma sát)
    player.velocityX *= powf(0.9f, dt * 60.0f);


    // Kiểm tra đến đích
    if (player.rect.x >= TRACK_LENGTH) {
        g_currentScene = Scene::FINISH;
    }
}


void renderScenePlay(float dt) {
    // Vẽ bầu trời
    SDL_SetRenderDrawColor(g_renderer, 135, 206, 250, 255);
    SDL_RenderClear(g_renderer);

    // Vẽ mặt đất
    SDL_FRect ground = {-g_cameraX, GROUND_Y, TRACK_LENGTH, 140};
    SDL_SetRenderDrawColor(g_renderer, 34, 139, 34, 255);
    SDL_RenderFillRect(g_renderer, &ground);

    // Vẽ vạch kẻ đường
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    for (float x = 0; x < TRACK_LENGTH; x += 100) {
        SDL_FRect line = {x - g_cameraX, GROUND_Y + 60, 50, 5};
        SDL_RenderFillRect(g_renderer, &line);
    }

    // Vẽ chướng ngại vật
    for (const auto& obs : g_obstacles) {
        SDL_FRect renderRect = {
            obs.rect.x - g_cameraX, obs.rect.y, obs.rect.w, obs.rect.h
        };
        SDL_SetRenderDrawColor(g_renderer, obs.color.r, obs.color.g, obs.color.b, obs.color.a);
        SDL_RenderFillRect(g_renderer, &renderRect);
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
        SDL_RenderRect(g_renderer, &renderRect);
    }

    // Vẽ vật phẩm
    for (const auto& item : g_collectibles) {
        if (!item.isCollected) {
            SDL_FRect renderRect = {
                item.rect.x - g_cameraX, item.rect.y, item.rect.w, item.rect.h
            };
            SDL_SetRenderDrawColor(g_renderer, item.color.r, item.color.g, item.color.b, item.color.a);
            SDL_RenderFillRect(g_renderer, &renderRect);
            SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
            SDL_RenderRect(g_renderer, &renderRect);
        }
    }

    // Cập nhật và vẽ nhân vật
    updatePlayer(dt);
    SDL_FRect playerRenderRect = {
        player.rect.x - g_cameraX,
        player.rect.y,
        player.rect.w,
        player.rect.h
    };
    SDL_RenderTexture(g_renderer, g_player, nullptr, &playerRenderRect);

    // Hiển thị thông tin (HUD)
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_FRect textRect = {0,0,0,0};

    // Điểm số
    std::string scoreText = "Score: " + std::to_string(g_score);
    surface = TTF_RenderText_Blended(g_smallFont, scoreText.c_str(), scoreText.length(), textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {10, 10, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Khoảng cách
    int distance = (int)(player.rect.x / 10);
    std::string distText = "Distance: " + std::to_string(distance) + "m / 1000m";
    surface = TTF_RenderText_Blended(g_smallFont, distText.c_str(), distText.length(), textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {10, 40, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Hướng dẫn
    surface = TTF_RenderText_Blended(g_smallFont, "A/D: Di chuyen, W/Space: Nhay, ESC: Ve Menu", 0, textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {10, 70, (float)surface->w * 0.8f, (float)surface->h * 0.8f};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }

    // HP
    std::string hpText = "HP: " + std::to_string(player.hp);
    surface = TTF_RenderText_Blended(g_smallFont, hpText.c_str(), hpText.length(), textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {10, 100, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Level (Góc trên phải)
    std::string levelText = "Level: " + std::to_string(g_level);
    surface = TTF_RenderText_Blended(g_smallFont, levelText.c_str(), levelText.length(), textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {SCREEN_WIDTH - 10.0f - surface->w, 10.0f, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Vật phẩm (Góc trên phải, dưới Level)
    std::string itemText = "Items: " + std::to_string(g_itemCount) + "/" + std::to_string(ITEMS_PER_LEVEL);
    surface = TTF_RenderText_Blended(g_smallFont, itemText.c_str(), itemText.length(), textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {SCREEN_WIDTH - 10.0f - surface->w, 40.0f, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }
}


void renderSceneFinish() {
    SDL_SetRenderDrawColor(g_renderer, 255, 215, 0, 255);
    SDL_RenderClear(g_renderer);
    SDL_Color textColor = {0, 0, 0, 255};
    SDL_Surface* surface = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_FRect textRect = {0,0,0,0};

    // Chúc mừng
    surface = TTF_RenderText_Blended(g_font, "CHUC MUNG!", 0, textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {SCREEN_WIDTH/2 - surface->w/2.0f, 100, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Điểm số
    std::string scoreText = "Diem so: " + std::to_string(g_score);
    surface = TTF_RenderText_Blended(g_font, scoreText.c_str(), scoreText.length(), textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {SCREEN_WIDTH/2 - surface->w/2.0f, 200, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Thời gian
    Uint32 timeTaken = (SDL_GetTicks() - g_gameStartTime) / 1000;
    std::string timeText = "Thoi gian: " + std::to_string(timeTaken) + "s";
    surface = TTF_RenderText_Blended(g_smallFont, timeText.c_str(), timeText.length(), textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {SCREEN_WIDTH/2 - surface->w/2.0f, 300, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
        SDL_DestroyTexture(texture);
    }

    // Hướng dẫn
    surface = TTF_RenderText_Blended(g_smallFont, "Bam ESC de ve Menu", 0, textColor);
    if (surface) {
        texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        textRect = {SCREEN_WIDTH/2 - surface->w/2.0f, 400, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect);
        SDL_DestroySurface(surface);
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
    g_level = 1;
    g_itemCount = 0;
    g_gameStartTime = SDL_GetTicks();
    createObstacles();
    createCollectibles();
}

int main(int argc, char* argv[]) {
    // Sửa lỗi check SDL_Init và TTF_Init cho SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    if (!TTF_Init()) {
        std::cout << "TTF_Init failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    g_window = SDL_CreateWindow("UET_RUN", SCREEN_WIDTH, SCREEN_HEIGHT, 0);

    // Sửa lỗi V-Sync cho SDL3
    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) {
        std::cout << "Failed to create renderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(g_window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(g_renderer, 1); // Bật VSync


    g_logoTexture = IMG_LoadTexture(g_renderer, "Assets/uet.png");
    if (!g_logoTexture) std::cout << "Load uet.png failed: " << SDL_GetError() << "\n";

    g_buttonTexture = IMG_LoadTexture(g_renderer, "Assets/button.png");
    if (!g_buttonTexture) std::cout << "Load button.png failed: " << SDL_GetError() << "\n";

    g_player = IMG_LoadTexture(g_renderer, "Assets/player.png");
    if (!g_player) std::cout << "Load player.png failed: " << SDL_GetError() << "\n";

    g_font = TTF_OpenFont("NotoSans-Regular.ttf", 36);
    if (!g_font) std::cout << "Failed to load font: " << SDL_GetError() << "\n";

    g_smallFont = TTF_OpenFont("NotoSans-Regular.ttf", 24);
    if (!g_smallFont) std::cout << "Failed to load small font: " << SDL_GetError() << "\n";

    g_startTime = SDL_GetTicks();
    g_lastTime = SDL_GetTicks(); // Khởi tạo dt

    bool running = true;
    SDL_Event e;
    const bool* keystate = SDL_GetKeyboardState(nullptr);

    while (running) {

        // Tính Delta Time (dt)
        Uint64 frameStartTime = SDL_GetTicks();
        if (g_lastTime == 0) g_lastTime = frameStartTime;

        float dt = (frameStartTime - g_lastTime) / 1000.0f;
        g_lastTime = frameStartTime;

        Uint32 menuCurrentTime = SDL_GetTicks(); // Dùng cho animation menu

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
                if (g_currentScene == Scene::MENU && menuCurrentTime - g_startTime > 4000) {
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
                renderSceneMenu(menuCurrentTime);
                break;
            case Scene::PLAY:
                renderScenePlay(dt);
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
