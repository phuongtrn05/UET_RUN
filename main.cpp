#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath> // Cần cho powf và fmod
#include <ctime> // Cần cho time()
#include <cstdlib> // Cần cho rand() và fabs

enum class Scene {
    MENU,
    PLAY,
    RESUME,
    SCORE,
    FINISH,
    GAME_OVER
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

    // Kích thước nhân vật
    Player() : rect{100, 400, 120, 140}, velocityX(0), velocityY(0),
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

// Cấu trúc cho vật phẩm gây sát thương
struct DamageItem {
    SDL_FRect rect;
    SDL_Color color;
    bool isCollected;

    DamageItem(float x, float y, float w, float h, SDL_Color c)
        : rect{x, y, w, h}, color(c), isCollected(false) {}
};

// Cấu trúc cho vật phẩm bí ẩn
struct MysteryItem {
    SDL_FRect rect;
    SDL_Color color;
    bool isCollected;

    MysteryItem(float x, float y, float w, float h, SDL_Color c)
        : rect{x, y, w, h}, color(c), isCollected(false) {}
};


// --- BIẾN TOÀN CỤC ---
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
TTF_Font* g_font = nullptr;
TTF_Font* g_smallFont = nullptr;
SDL_Texture* g_logoTexture = nullptr;
SDL_Texture* g_buttonTexture = nullptr;
SDL_Texture* g_player = nullptr;
SDL_Texture* g_backgroundTexture = nullptr;
SDL_Texture* g_damageItemTexture = nullptr;
SDL_Texture* g_collectibleTexture = nullptr;
SDL_Texture* g_mysteryItemTexture = nullptr;
SDL_Texture* g_obstacleTexture = nullptr;
float g_bgWidth = 0.0f;
float g_bgHeight = 0.0f;

SDL_FRect g_logoRect = {200, 100, 400, 400};
Uint8 g_alpha = 0;
bool g_shrinking = false;
Uint32 g_startTime = 0;
Scene g_currentScene = Scene::MENU;
Player player; // Khai báo nhân vật toàn cục

Uint64 g_lastTime = 0;

// Camera
float g_cameraX = 0.0f;

// Hằng số vật lý
const float GRAVITY = 0.8f * 60.0f * 60.0f;
const float JUMP_FORCE = -15.0f * 60.0f;
const float BASE_MOVE_SPEED = 5.0f * 60.0f;
float g_moveSpeed = BASE_MOVE_SPEED; // Tốc độ di chuyển hiện tại
const float EPSILON = 0.01f; // Giá trị nhỏ để xử lý sai số floating point

const float GROUND_Y = 460.0f; // Vị trí mặt đất
const float TRACK_LENGTH = 10000.0f; // Chiều dài màn chơi
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Danh sách các đối tượng trong game
std::vector<Obstacle> g_obstacles;
std::vector<Collectible> g_collectibles;
std::vector<DamageItem> g_damageItems;
std::vector<MysteryItem> g_mysteryItems;

// Biến trạng thái game
int g_score = 0;
int g_level = 1;
int g_itemCount = 0; // Số item nhặt được trong level hiện tại
int g_totalItemCount = 0; // Tổng số item nhặt được
const int ITEMS_PER_LEVEL = 10; // Số item cần nhặt để lên level
Uint32 g_gameStartTime = 0; // Thời điểm bắt đầu chơi (để tính giờ)
Uint32 g_playTimeSeconds = 0; // Thời gian chơi đã trôi qua

// Cấu trúc nút bấm (cho menu)
struct Button {
    std::string text;
    SDL_FRect rect;
};

// Danh sách nút bấm
std::vector<Button> g_buttons = {
    {"Play",   {300, 200, 180, 80}},
    {"Resume", {300, 300, 180, 80}}, // Nút Resume (chưa hoàn thiện)
    {"Score",  {300, 400, 180, 80}}  // Nút Score (chưa hoàn thiện)
};

// --- CÁC HÀM HỖ TRỢ ---

// Hàm kiểm tra va chạm giữa hai hình chữ nhật
bool checkRectCollision(const SDL_FRect& a, const SDL_FRect& b) {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}

// --- CÁC HÀM TẠO ĐỐI TƯỢNG ---

// Hàm tạo chướng ngại vật (đã cập nhật kích thước)
void createObstacles() {
    g_obstacles.clear();
    for (float x = 800; x < TRACK_LENGTH; x += 350 + (rand() % 250)) {
        int obstacleType = rand() % 3;
        switch (obstacleType) {
            case 0: g_obstacles.push_back(Obstacle(x, GROUND_Y - 80, 80, 80, {139, 69, 19, 255})); break;
            case 1: g_obstacles.push_back(Obstacle(x, GROUND_Y - 130, 70, 130, {128, 128, 128, 255})); break;
            case 2: g_obstacles.push_back(Obstacle(x, GROUND_Y - 50, 130, 50, {160, 82, 45, 255})); break;
        }
    }
    g_obstacles.push_back(Obstacle(TRACK_LENGTH + 100, GROUND_Y - 250, 30, 250, {255, 215, 0, 255}));
}

// Hàm tạo vật phẩm thu thập (đã cập nhật kích thước)
void createCollectibles() {
    g_collectibles.clear();
    for (float x = 600; x < TRACK_LENGTH; x += 200 + (rand() % 150)) {
        float yPos = (rand() % 2 == 0) ? (GROUND_Y - 50) : (GROUND_Y - 180);
        SDL_FRect newItemRect = {x, yPos, 40, 40};
        bool isOverlapping = false;
        for (const auto& obs : g_obstacles) { if (checkRectCollision(newItemRect, obs.rect)) { isOverlapping = true; break; } }
        if (!isOverlapping) { g_collectibles.push_back(Collectible(x, yPos, 40, 40, {255, 255, 0, 255})); }
    }
}

// Hàm tạo vật phẩm trừ HP (đã cập nhật kích thước)
void createDamageItems() {
    g_damageItems.clear();
    for (float x = 1000; x < TRACK_LENGTH; x += 400 + (rand() % 200)) {
        float yPos = (rand() % 2 == 0) ? (GROUND_Y - 50) : (GROUND_Y - 170);
        SDL_FRect newItemRect = {x, yPos, 40, 40};
        bool isOverlapping = false;
        for (const auto& obs : g_obstacles) { if (checkRectCollision(newItemRect, obs.rect)) { isOverlapping = true; break; } }
        if (isOverlapping) continue;
        for (const auto& item : g_collectibles) { if (checkRectCollision(newItemRect, item.rect)) { isOverlapping = true; break; } }
        if (isOverlapping) continue;
        g_damageItems.push_back(DamageItem(x, yPos, 40, 40, {255, 0, 0, 255}));
    }
}

// Hàm tạo vật phẩm bí ẩn (đã cập nhật kích thước)
void createMysteryItems() {
    g_mysteryItems.clear();
    for (float x = 700; x < TRACK_LENGTH; x += 1500 + (rand() % 1000)) {
        float yPos = (rand() % 2 == 0) ? (GROUND_Y - 50) : (GROUND_Y - 190);
        SDL_FRect newItemRect = {x, yPos, 40, 40};
        bool isOverlapping = false;
        for (const auto& obs : g_obstacles) { if (checkRectCollision(newItemRect, obs.rect)) { isOverlapping = true; break; } }
        if (isOverlapping) continue;
        for (const auto& item : g_collectibles) { if (checkRectCollision(newItemRect, item.rect)) { isOverlapping = true; break; } }
        if (isOverlapping) continue;
        for (const auto& item : g_damageItems) { if (checkRectCollision(newItemRect, item.rect)) { isOverlapping = true; break; } }
        if (isOverlapping) continue;
        g_mysteryItems.push_back(MysteryItem(x, yPos, 40, 40, {128, 0, 128, 255}));
    }
}

// Hàm vẽ nút bấm (cho menu)
void renderRoundedButton(SDL_Renderer* renderer, const Button& btn,
                       TTF_Font* font, SDL_Texture* bgTexture, SDL_Color borderColor, SDL_Color textColor)
{
    if (!renderer || !font || !bgTexture) return;
    SDL_RenderTexture(renderer, bgTexture, nullptr, &btn.rect);
    SDL_Surface* surface = TTF_RenderText_Blended(font, btn.text.c_str(), btn.text.length(), textColor);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FRect textRect = { btn.rect.x + (btn.rect.w - surface->w) / 2.0f, btn.rect.y + (btn.rect.h - surface->h) / 2.0f, (float)surface->w, (float)surface->h };
    SDL_RenderTexture(renderer, texture, nullptr, &textRect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

// Hàm kiểm tra va chạm điểm với hình chữ nhật (cho nút bấm)
bool checkCollision(float x, float y, const SDL_FRect& rect) {
    return (x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h);
}


// --- HÀM CẬP NHẬT TRẠNG THÁI NHÂN VẬT ---
// ✅ HÀM updatePlayer (ĐÃ VIẾT LẠI LOGIC VA CHẠM GIỐNG MARIO HƠN)
void updatePlayer(float dt, bool isMovingLeft, bool isMovingRight) { // Nhận trạng thái input làm tham số

    // --- 1. XỬ LÝ INPUT VÀ MA SÁT/LỰC CẢN (TRỤC X) ---
    // A. Input
    if (isMovingRight) {
        player.velocityX = g_moveSpeed;
    } else if (isMovingLeft) {
        player.velocityX = -g_moveSpeed;
    } else {
        // B. Ma sát (chỉ khi chạm đất và không có input)
        if (player.onGround) {
            player.velocityX *= powf(0.85f, dt * 60.0f); // Tăng ma sát
            if (fabs(player.velocityX) < 0.5f) player.velocityX = 0; // Dừng hẳn
        }
    }
    // C. Lực cản không khí (luôn áp dụng khi không chạm đất)
    if (!player.onGround) {
         player.velocityX *= powf(0.98f, dt * 60.0f);
    }


    // --- 2. ÁP DỤNG VẬN TỐC HIỆN TẠI ĐỂ TÍNH VỊ TRÍ TIỀM NĂNG ---
    float potentialX = player.rect.x + (player.velocityX * dt);
    // Áp dụng trọng lực vào vận tốc Y *trước* khi tính vị trí Y tiềm năng
    player.velocityY += GRAVITY * dt;
    float potentialY = player.rect.y + (player.velocityY * dt);


    // --- 3. XỬ LÝ VA CHẠM VÀ CẬP NHẬT VỊ TRÍ TRỤC Y ---
    SDL_FRect playerTestRectY = player.rect;
    playerTestRectY.y = potentialY;
    bool landedOnSomething = false; // Cờ kiểm tra tiếp đất

    // A. Kiểm tra va chạm với mặt đất chính (GROUND_Y)
    if (player.velocityY >= 0 && playerTestRectY.y + playerTestRectY.h >= GROUND_Y) {
        player.rect.y = GROUND_Y - player.rect.h; // Đặt lên mặt đất
        player.velocityY = 0;
        landedOnSomething = true;
    } else {
        // B. Kiểm tra va chạm với chướng ngại vật (Obstacles)
        for (auto& obs : g_obstacles) {
            // Chỉ kiểm tra va chạm Y với vật cản nếu có khả năng va chạm theo trục X
            if (potentialX < obs.rect.x + obs.rect.w && potentialX + player.rect.w > obs.rect.x)
            {
                SDL_FRect checkRect = {player.rect.x, potentialY, player.rect.w, player.rect.h}; // Dùng X hiện tại
                 if (checkRectCollision(checkRect, obs.rect)) {
                    // i. Đang rơi (velocityY >= 0) và đáp xuống nóc vật cản
                    if (player.velocityY >= 0 && player.rect.y + player.rect.h <= obs.rect.y + EPSILON*5) {
                        player.rect.y = obs.rect.y - player.rect.h - EPSILON; // Đặt lên trên
                        player.velocityY = 0;
                        landedOnSomething = true;
                        break; // Đã xử lý va chạm Y, thoát vòng lặp
                    }
                    // ii. Đang nhảy lên (velocityY < 0) và đụng phải đáy vật cản
                    else if (player.velocityY < 0 && player.rect.y >= obs.rect.y + obs.rect.h - EPSILON*5) {
                        player.rect.y = obs.rect.y + obs.rect.h + EPSILON; // Đẩy xuống dưới
                        player.velocityY = 0; // Dừng nhảy
                        break; // Đã xử lý va chạm Y, thoát vòng lặp
                    }
                }
            }
        }
    }

    // C. Cập nhật vị trí Y cuối cùng nếu không có va chạm đáp đất
    if (!landedOnSomething) {
        player.rect.y = potentialY;
    }
    // D. Cập nhật trạng thái onGround cuối cùng
    player.onGround = landedOnSomething;


    // --- 4. XỬ LÝ VA CHẠM VÀ CẬP NHẬT VỊ TRÍ TRỤC X ---
    SDL_FRect playerTestRectX = player.rect; // Dùng Y đã được cập nhật
    playerTestRectX.x = potentialX;
    bool hitWall = false;

    for (auto& obs : g_obstacles) {
        // Chỉ kiểm tra va chạm X nếu có khả năng va chạm theo trục Y
        if (player.rect.y < obs.rect.y + obs.rect.h && player.rect.y + player.rect.h > obs.rect.y)
        {
             if (checkRectCollision(playerTestRectX, obs.rect)) {
                // i. Di chuyển sang phải (velocityX > 0) và va vào cạnh trái vật cản
                if (player.velocityX > 0) {
                    player.rect.x = obs.rect.x - player.rect.w - EPSILON; // Đặt sát bên trái
                    player.velocityX = 0; // Dừng lại
                    hitWall = true;
                    break; // Đã xử lý va chạm X, thoát
                }
                // ii. Di chuyển sang trái (velocityX < 0) và va vào cạnh phải vật cản
                else if (player.velocityX < 0) {
                    player.rect.x = obs.rect.x + obs.rect.w + EPSILON; // Đặt sát bên phải
                    player.velocityX = 0; // Dừng lại
                    hitWall = true;
                    break; // Đã xử lý va chạm X, thoát
                }
            }
        }
    }

    // Cập nhật vị trí X cuối cùng nếu không va vào tường
    if (!hitWall) {
        player.rect.x = potentialX;
    }


    // --- 5. LOGIC KHÁC (Items, Score, Camera, Fall, Finish) ---
    // Giới hạn không cho nhân vật đi lùi khỏi camera
    if (player.rect.x < g_cameraX) {
        player.rect.x = g_cameraX;
        if (player.velocityX < 0) player.velocityX = 0;
    }
    // Tính điểm khi vượt qua vật cản
    for (auto& obs : g_obstacles) {
         if (!obs.isPassed && player.rect.x + player.rect.w / 2 > obs.rect.x + obs.rect.w) {
            obs.isPassed = true;
            g_score += 10;
        }
    }
    // Nhặt vật phẩm (Vàng)
    for (auto& item : g_collectibles) {
        if (!item.isCollected && checkRectCollision(player.rect, item.rect)) {
            item.isCollected = true; g_itemCount++; g_totalItemCount++; g_score += 5;
            if (g_itemCount >= ITEMS_PER_LEVEL) { g_level++; g_itemCount = 0; g_moveSpeed *= 1.1f; }
        }
    }
    // Va chạm vật phẩm (Đỏ)
    for (auto& item : g_damageItems) {
        if (!item.isCollected && checkRectCollision(player.rect, item.rect)) {
            item.isCollected = true; player.hp -= 20;
            if (player.hp <= 0) { player.hp = 0; g_currentScene = Scene::GAME_OVER; }
        }
    }
    // Va chạm vật phẩm (Tím)
    for (auto& item : g_mysteryItems) {
        if (!item.isCollected && checkRectCollision(player.rect, item.rect)) {
            item.isCollected = true; int effect = rand() % 3;
            switch (effect) {
                case 0: player.hp += 20; if (player.hp > 100) player.hp = 100; break;
                case 1: g_itemCount++; g_totalItemCount++; g_score += 20;
                        if (g_itemCount >= ITEMS_PER_LEVEL) { g_level++; g_itemCount = 0; g_moveSpeed *= 1.1f; } break;
                case 2: player.hp -= 20; if (player.hp <= 0) { player.hp = 0; g_currentScene = Scene::GAME_OVER; } break;
            }
        }
    }
    // Cập nhật camera
    g_cameraX = player.rect.x - 200; if (g_cameraX < 0) g_cameraX = 0;
    // Kiểm tra đến đích
    if (player.rect.x >= TRACK_LENGTH && g_currentScene != Scene::GAME_OVER) { g_currentScene = Scene::FINISH; }
    // Rơi xuống vực -> Game Over
    if (player.rect.y > SCREEN_HEIGHT + player.rect.h && g_currentScene != Scene::GAME_OVER) { g_currentScene = Scene::GAME_OVER; }
}

// --- CÁC HÀM VẼ SCENE ---

// HÀM renderScenePlay (Sửa cách gọi updatePlayer)
void renderScenePlay(float dt) {
    // 1. Xóa màn hình
    SDL_SetRenderDrawColor(g_renderer, 135, 206, 250, 255);
    SDL_RenderClear(g_renderer);

    // 2. Vẽ nền lặp lại
     if (g_backgroundTexture && g_bgWidth > 0 && g_bgHeight > 0) {
        float parallaxFactor = 0.5f;
        float scale = (float)SCREEN_HEIGHT / (float)g_bgHeight;
        float scaledWidth = (float)g_bgWidth * scale;
        float bgOffset = fmod(g_cameraX * parallaxFactor, scaledWidth);
        SDL_FRect destRect1 = { -bgOffset, 0, scaledWidth, (float)SCREEN_HEIGHT };
        SDL_RenderTexture(g_renderer, g_backgroundTexture, nullptr, &destRect1);
        SDL_FRect destRect2 = { -bgOffset + scaledWidth, 0, scaledWidth, (float)SCREEN_HEIGHT };
        SDL_RenderTexture(g_renderer, g_backgroundTexture, nullptr, &destRect2);
    }

    // 3. Vẽ vật cản
      for (const auto& obs : g_obstacles) {
        SDL_FRect renderRect = {obs.rect.x - g_cameraX, obs.rect.y, obs.rect.w, obs.rect.h};
        if (g_obstacleTexture) { SDL_RenderTexture(g_renderer, g_obstacleTexture, nullptr, &renderRect); }
        else { /* Vẽ màu dự phòng nếu cần */ }
    }

    // 4. Vẽ vật phẩm (Vàng)
     for (const auto& item : g_collectibles) {
        if (!item.isCollected) {
            SDL_FRect renderRect = {item.rect.x - g_cameraX, item.rect.y, item.rect.w, item.rect.h};
            if (g_collectibleTexture) { SDL_RenderTexture(g_renderer, g_collectibleTexture, nullptr, &renderRect); }
            else { /* Vẽ màu dự phòng nếu cần */ }
        }
    }

    // 5. Vẽ vật phẩm (Đỏ)
     for (const auto& item : g_damageItems) {
        if (!item.isCollected) {
            SDL_FRect renderRect = {item.rect.x - g_cameraX, item.rect.y, item.rect.w, item.rect.h};
            if (g_damageItemTexture) { SDL_RenderTexture(g_renderer, g_damageItemTexture, nullptr, &renderRect); }
            else { /* Vẽ màu dự phòng nếu cần */ }
        }
    }

    // 6. Vẽ vật phẩm (Tím - Bí ẩn)
    for (const auto& item : g_mysteryItems) {
        if (!item.isCollected) {
            SDL_FRect renderRect = {item.rect.x - g_cameraX, item.rect.y, item.rect.w, item.rect.h};
            if (g_mysteryItemTexture) { SDL_RenderTexture(g_renderer, g_mysteryItemTexture, nullptr, &renderRect); }
            else { /* Vẽ màu dự phòng nếu cần */ }
        }
    }

    // 7. INPUT VÀ UPDATE
    const bool* keystate = SDL_GetKeyboardState(nullptr);
    bool isMovingLeft = keystate[SDL_SCANCODE_A];
    bool isMovingRight = keystate[SDL_SCANCODE_D];
    updatePlayer(dt, isMovingLeft, isMovingRight); // Gọi updatePlayer sau khi lấy input

    // 8. Vẽ nhân vật
    SDL_FRect playerRenderRect = { player.rect.x - g_cameraX, player.rect.y, player.rect.w, player.rect.h };
    SDL_RenderTexture(g_renderer, g_player, nullptr, &playerRenderRect);

    // --- 9. HIỂN THỊ HUD KIỂU MARIO ---
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_FRect textRect;
    float col1_x = 50.0f, col2_x = 250.0f, col3_x = 450.0f, col4_x = 650.0f;
    // Cột 1
    surface = TTF_RenderText_Blended(g_smallFont, "UET", 0, textColor); if (surface) { texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {col1_x, 20.0f, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }
    std::string itemsText = std::to_string(g_totalItemCount); surface = TTF_RenderText_Blended(g_smallFont, itemsText.c_str(), itemsText.length(), textColor); if (surface) { texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {col1_x, 50.0f, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }
    // Cột 2
    surface = TTF_RenderText_Blended(g_smallFont, "HP", 0, textColor); if (surface) { texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {col2_x, 20.0f, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }
    std::string hpText = std::to_string(player.hp); surface = TTF_RenderText_Blended(g_smallFont, hpText.c_str(), hpText.length(), textColor); if (surface) { texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {col2_x, 50.0f, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }
    // Cột 3
    surface = TTF_RenderText_Blended(g_smallFont, "LEVEL", 0, textColor); if (surface) { texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {col3_x, 20.0f, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }
    std::string levelText = std::to_string(g_level); surface = TTF_RenderText_Blended(g_smallFont, levelText.c_str(), levelText.length(), textColor); if (surface) { texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {col3_x, 50.0f, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }
    // Cột 4
    surface = TTF_RenderText_Blended(g_smallFont, "TIME", 0, textColor); if (surface) { texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {col4_x, 20.0f, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }
    g_playTimeSeconds = (SDL_GetTicks() - g_gameStartTime) / 1000; std::string timeText = std::to_string(g_playTimeSeconds); surface = TTF_RenderText_Blended(g_smallFont, timeText.c_str(), timeText.length(), textColor); if (surface) { texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {col4_x, 50.0f, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }

}

// Hàm vẽ màn hình Chúc mừng/Kết thúc
void renderSceneFinish() {
    SDL_SetRenderDrawColor(g_renderer, 255, 215, 0, 255);
    SDL_RenderClear(g_renderer);
    SDL_Color textColor = {0, 0, 0, 255};
    SDL_Surface* surface = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_FRect textRect;

    surface = TTF_RenderText_Blended(g_font, "CHUC MUNG!", 0, textColor);
    if (surface) { /* Vẽ chữ Chúc mừng */ texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {SCREEN_WIDTH/2.0f - surface->w/2.0f, 100, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }

    std::string scoreText = "Vat pham: " + std::to_string(g_totalItemCount);
    surface = TTF_RenderText_Blended(g_font, scoreText.c_str(), scoreText.length(), textColor);
    if (surface) { /* Vẽ điểm */ texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {SCREEN_WIDTH/2.0f - surface->w/2.0f, 200, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }

    std::string timeText = "Thoi gian: " + std::to_string(g_playTimeSeconds) + "s";
    surface = TTF_RenderText_Blended(g_smallFont, timeText.c_str(), timeText.length(), textColor);
    if (surface) { /* Vẽ thời gian */ texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {SCREEN_WIDTH/2.0f - surface->w/2.0f, 300, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }

    surface = TTF_RenderText_Blended(g_smallFont, "Bam ESC de ve Menu", 0, textColor);
    if (surface) { /* Vẽ hướng dẫn */ texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {SCREEN_WIDTH/2.0f - surface->w/2.0f, 400, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }
}

// Hàm vẽ màn hình Game Over
void renderSceneGameOver() {
    SDL_SetRenderDrawColor(g_renderer, 139, 0, 0, 255);
    SDL_RenderClear(g_renderer);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_FRect textRect;

    surface = TTF_RenderText_Blended(g_font, "GAME OVER", 0, textColor);
    if (surface) { /* Vẽ chữ Game Over */ texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {SCREEN_WIDTH/2.0f - surface->w/2.0f, 150, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }

    std::string scoreText = "Vat pham: " + std::to_string(g_totalItemCount);
    surface = TTF_RenderText_Blended(g_smallFont, scoreText.c_str(), scoreText.length(), textColor);
    if (surface) { /* Vẽ điểm */ texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {SCREEN_WIDTH/2.0f - surface->w/2.0f, 300, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }

    surface = TTF_RenderText_Blended(g_smallFont, "Bam ESC de ve Menu", 0, textColor);
    if (surface) { /* Vẽ hướng dẫn */ texture = SDL_CreateTextureFromSurface(g_renderer, surface); textRect = {SCREEN_WIDTH/2.0f - surface->w/2.0f, 400, (float)surface->w, (float)surface->h}; SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture); }
}

// Hàm vẽ màn hình Resume (Tạm thời)
void renderSceneResume() {
    SDL_SetRenderDrawColor(g_renderer, 150, 150, 255, 255);
    SDL_RenderClear(g_renderer);
    // ... (Vẽ chữ tạm thời) ...
}
// Hàm vẽ màn hình Score (Tạm thời)
void renderSceneScore() {
    SDL_SetRenderDrawColor(g_renderer, 255, 165, 0, 255);
    SDL_RenderClear(g_renderer);
     // ... (Vẽ chữ tạm thời) ...
}

// Hàm vẽ màn hình Menu
void renderSceneMenu(Uint32 currentTime) {
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);
    SDL_RenderClear(g_renderer);
    SDL_FRect ground = {0, 500, 800, 100};
    SDL_SetRenderDrawColor(g_renderer, 173, 216, 230, 255);
    SDL_RenderFillRect(g_renderer, &ground);
    // ... (Animation logo) ...
     if (g_alpha < 255 && !g_shrinking) { g_alpha = (Uint8)SDL_min(g_alpha + 3, 255); SDL_SetTextureAlphaMod(g_logoTexture, g_alpha); } else { g_shrinking = true; }
    if (g_shrinking) { if (g_logoRect.w > 100) { g_logoRect.w *= 0.98f; g_logoRect.h *= 0.98f; g_logoRect.x = (800 - g_logoRect.w) / 2; g_logoRect.y = (600 - g_logoRect.h) / 2; } else { g_logoRect.x -= (g_logoRect.x - 20) * 0.1f; g_logoRect.y -= (g_logoRect.y - 20) * 0.1f; } }
    SDL_RenderTexture(g_renderer, g_logoTexture, nullptr, &g_logoRect);

    // Hiển thị nút bấm sau một khoảng thời gian
    if (currentTime - g_startTime > 4000) {
        SDL_Color borderColor = {255, 215, 0, 255};
        SDL_Color textColor   = {255, 255, 255, 255};
        for (auto& b : g_buttons) {
            renderRoundedButton(g_renderer, b, g_font, g_buttonTexture, borderColor, textColor);
        }
    }
}

// Hàm reset trạng thái game khi bắt đầu chơi mới
void resetPlayer() {
    player.rect.x = 100;
    player.rect.y = 300; // Đặt Y ban đầu cao hơn một chút
    player.velocityX = 0; player.velocityY = 0;
    player.isJumping = false; player.onGround = false; player.hp = 100;
    g_cameraX = 0; g_score = 0; g_level = 1; g_moveSpeed = BASE_MOVE_SPEED;
    g_itemCount = 0; g_totalItemCount = 0; g_gameStartTime = SDL_GetTicks(); g_playTimeSeconds = 0;
    createObstacles(); createCollectibles(); createDamageItems(); createMysteryItems();
}


// --- HÀM MAIN ---
int main(int argc, char* argv[]) {
    // Khởi tạo SDL, TTF, Window, Renderer
     if (!SDL_Init(SDL_INIT_VIDEO)) { std::cout << "SDL_Init failed: " << SDL_GetError() << "\n"; return 1; }
    if (!TTF_Init()) { std::cout << "TTF_Init failed: " << SDL_GetError() << "\n"; SDL_Quit(); return 1; }
    srand((unsigned int)time(NULL));
    g_window = SDL_CreateWindow("UET_RUN", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) { std::cout << "Failed to create renderer: " << SDL_GetError() << "\n"; SDL_DestroyWindow(g_window); TTF_Quit(); SDL_Quit(); return 1; }
    SDL_SetRenderVSync(g_renderer, 1);


    // Load tất cả textures
    g_logoTexture = IMG_LoadTexture(g_renderer, "Assets/uet.png");
    g_buttonTexture = IMG_LoadTexture(g_renderer, "Assets/button.png");
    g_player = IMG_LoadTexture(g_renderer, "Assets/player.png"); // Đảm bảo đây là file ảnh nhân vật đúng
    g_damageItemTexture = IMG_LoadTexture(g_renderer, "Assets/bad.png"); if (!g_damageItemTexture) { std::cout << "Failed to load Assets/bad.png: " << SDL_GetError() << "\n"; }
    g_collectibleTexture = IMG_LoadTexture(g_renderer, "Assets/item.png"); if (!g_collectibleTexture) { std::cout << "Failed to load Assets/item.png: " << SDL_GetError() << "\n"; }
    g_mysteryItemTexture = IMG_LoadTexture(g_renderer, "Assets/mystery.png"); if (!g_mysteryItemTexture) { std::cout << "Failed to load Assets/mystery.png: " << SDL_GetError() << "\n"; }
    g_obstacleTexture = IMG_LoadTexture(g_renderer, "Assets/deadline.png"); if (!g_obstacleTexture) { std::cout << "Failed to load Assets/deadline.png: " << SDL_GetError() << "\n"; }
    g_backgroundTexture = IMG_LoadTexture(g_renderer, "Assets/background.png"); if (g_backgroundTexture) { SDL_GetTextureSize(g_backgroundTexture, &g_bgWidth, &g_bgHeight); } else { std::cout << "Failed to load Assets/background.png: " << SDL_GetError() << "\n"; }


    // Load Fonts
     g_font = TTF_OpenFont("NotoSans-Regular.ttf", 36);
     g_smallFont = TTF_OpenFont("NotoSans-Regular.ttf", 24);

    // Khởi tạo thời gian
    g_startTime = SDL_GetTicks();
    g_lastTime = SDL_GetTicks();

    // Vòng lặp chính của game
    bool running = true;
    SDL_Event e;
    while (running) {
        Uint64 frameStartTime = SDL_GetTicks();
        float dt = (frameStartTime - g_lastTime) / 1000.0f;
        g_lastTime = frameStartTime;
        if (dt > 0.05f) { dt = 0.05f; } // Cap dt

        Uint32 menuCurrentTime = SDL_GetTicks(); // Dùng riêng cho animation menu

        // Xử lý sự kiện (Input)
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) { // Thoát hoặc về Menu
                    if (g_currentScene != Scene::MENU) g_currentScene = Scene::MENU;
                    else running = false;
                }
                // ✅ XỬ LÝ NHẢY TRONG EVENT LOOP
                else if (g_currentScene == Scene::PLAY) {
                    if ((e.key.key == SDLK_W || e.key.key == SDLK_SPACE) && player.onGround) {
                        player.velocityY = JUMP_FORCE;
                        player.onGround = false; // Ngăn nhảy liên tục trong 1 frame
                    }
                }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                 if (g_currentScene == Scene::MENU && menuCurrentTime - g_startTime > 4000) { // Chỉ xử lý click menu khi nút hiện
                    float mouseX = (float)e.button.x; float mouseY = (float)e.button.y;
                    if (checkCollision(mouseX, mouseY, g_buttons[0].rect)) { // Nút Play
                        g_currentScene = Scene::PLAY; resetPlayer();
                    }
                    else if (checkCollision(mouseX, mouseY, g_buttons[1].rect)) { // Nút Resume
                        std::cout << "Resume function not fully implemented yet.\n";
                        g_currentScene = Scene::PLAY; // Tạm reset
                        resetPlayer();
                    }
                    else if (checkCollision(mouseX, mouseY, g_buttons[2].rect)) { // Nút Score
                         std::cout << "Score scene not implemented yet.\n";
                    }
                }
            }
        }

        // Vẽ scene hiện tại
        switch (g_currentScene) {
            case Scene::MENU:       renderSceneMenu(menuCurrentTime); break;
            case Scene::PLAY:       renderScenePlay(dt);              break;
            case Scene::RESUME:     renderSceneResume();              break;
            case Scene::SCORE:      renderSceneScore();               break;
            case Scene::FINISH:     renderSceneFinish();              break;
            case Scene::GAME_OVER:  renderSceneGameOver();            break;
        }

        // Hiển thị lên màn hình
        SDL_RenderPresent(g_renderer);
    }

    // Dọn dẹp tài nguyên
    if (g_font) TTF_CloseFont(g_font); if (g_smallFont) TTF_CloseFont(g_smallFont);
    if (g_logoTexture) SDL_DestroyTexture(g_logoTexture); if (g_buttonTexture) SDL_DestroyTexture(g_buttonTexture);
    if (g_player) SDL_DestroyTexture(g_player); if (g_backgroundTexture) SDL_DestroyTexture(g_backgroundTexture);
    if (g_damageItemTexture) SDL_DestroyTexture(g_damageItemTexture); if (g_collectibleTexture) SDL_DestroyTexture(g_collectibleTexture);
    if (g_mysteryItemTexture) SDL_DestroyTexture(g_mysteryItemTexture); if (g_obstacleTexture) SDL_DestroyTexture(g_obstacleTexture);
    if (g_renderer) SDL_DestroyRenderer(g_renderer); if (g_window) SDL_DestroyWindow(g_window);

    TTF_Quit();
    SDL_Quit();
    return 0;
}
