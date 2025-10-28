#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath> // Needed for powf and fmod
#include <ctime> // Needed for time()
#include <cstdlib> // Needed for rand() and fabs

enum class Scene {
    MENU,
    PLAY,
    RESUME, // Pause state
    SCORE,
    FINISH,
    GAME_OVER
};

// Character structure
struct Player {
    SDL_FRect rect;
    float velocityX;
    float velocityY;
    bool isJumping;
    bool onGround;
    int hp;
    SDL_Color color;

    Player() : rect{100, 400, 120, 140}, velocityX(0), velocityY(0), // Character size
             isJumping(false), onGround(false), hp(100),
             color{255, 100, 100, 255} {}
};

// Obstacle structure
struct Obstacle {
    SDL_FRect rect;
    SDL_Color color;
    bool isPassed;
    Obstacle(float x, float y, float w, float h, SDL_Color c) : rect{x, y, w, h}, color(c), isPassed(false) {}
};

// Collectible item structure
struct Collectible {
    SDL_FRect rect;
    SDL_Color color;
    bool isCollected;
    Collectible(float x, float y, float w, float h, SDL_Color c) : rect{x, y, w, h}, color(c), isCollected(false) {}
};

// ✅ Damage item structure (WITH MOVEMENT)
struct DamageItem {
    SDL_FRect rect;
    SDL_Color color; // Fallback color
    bool isCollected;
    float velocityX;    // Horizontal velocity
    float startX;       // Initial X position
    float moveRange;    // Horizontal movement range

    DamageItem(float x, float y, float w, float h, SDL_Color c)
        : rect{x, y, w, h}, color(c), isCollected(false),
          velocityX(100.0f + (rand() % 50)), // Random horizontal speed (pixels/s)
          startX(x),                         // Store initial position
          moveRange(80.0f + (rand() % 40))   // Random movement range
    {
        // 50% chance to start moving left
        if (rand() % 2 == 0) {
            velocityX = -velocityX;
        }
    }
};


// Mystery item structure
struct MysteryItem {
    SDL_FRect rect;
    SDL_Color color;
    bool isCollected;
    MysteryItem(float x, float y, float w, float h, SDL_Color c) : rect{x, y, w, h}, color(c), isCollected(false) {}
};


// --- GLOBAL VARIABLES ---
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
float g_menuBgOffsetX = 0.0f;

SDL_FRect g_logoRect = {200, 100, 400, 400};
Uint8 g_alpha = 0;
bool g_shrinking = false;
Uint32 g_startTime = 0;
Scene g_currentScene = Scene::MENU;
Player player;
Uint64 g_lastTime = 0;
float g_cameraX = 0.0f;

// Physics constants - MARIO-LIKE MOVEMENT V9
const float GRAVITY = 1900.0f;
const float JUMP_INITIAL_VELOCITY = -780.0f;
const float MOVE_ACCELERATION = 2000.0f;
const float MAX_MOVE_SPEED = 320.0f;
float g_maxMoveSpeed = MAX_MOVE_SPEED;
const float GROUND_FRICTION_FACTOR = 0.88f;
const float AIR_RESISTANCE_FACTOR = 0.96f;
const float JUMP_GRAVITY_MODIFIER = 0.6f;
const float JUMP_CUT_VELOCITY_REDUCTION = 0.5f;
const float EPSILON = 0.01f;

// Game world constants
const float GROUND_Y = 460.0f;
const float TRACK_LENGTH = 10000.0f;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Game object lists
std::vector<Obstacle> g_obstacles;
std::vector<Collectible> g_collectibles;
std::vector<DamageItem> g_damageItems;
std::vector<MysteryItem> g_mysteryItems;

// Game state variables
int g_score = 0; int g_level = 1; int g_itemCount = 0; int g_totalItemCount = 0;
const int ITEMS_PER_LEVEL = 10; Uint32 g_gameStartTime = 0; Uint32 g_playTimeSeconds = 0;

// Button structure and list
struct Button { std::string text; SDL_FRect rect; };
std::vector<Button> g_buttons = { {"Play",{300,200,180,80}},{"Resume",{300,300,180,80}},{"Score",{300,400,180,80}} };

// --- HELPER FUNCTIONS ---
bool checkRectCollision(const SDL_FRect& a, const SDL_FRect& b) { return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y); }

// --- OBJECT CREATION FUNCTIONS ---
void createObstacles() { g_obstacles.clear(); for (float x=800;x<TRACK_LENGTH;x+=350+(rand()%250)){int t=rand()%3;switch(t){case 0:g_obstacles.push_back(Obstacle(x,GROUND_Y-80,80,80,{139,69,19,255}));break;case 1:g_obstacles.push_back(Obstacle(x,GROUND_Y-130,70,130,{128,128,128,255}));break;case 2:g_obstacles.push_back(Obstacle(x,GROUND_Y-50,130,50,{160,82,45,255}));break;}} g_obstacles.push_back(Obstacle(TRACK_LENGTH+100,GROUND_Y-250,30,250,{255,215,0,255})); }
void createCollectibles() { g_collectibles.clear();const float iw=40,ih=40,ob=15;for(float x=700;x<TRACK_LENGTH-200;x+=250+(rand()%200)){float y;int yc=rand()%3;if(yc==0)y=GROUND_Y-ih-5;else if(yc==1)y=GROUND_Y-150;else y=GROUND_Y-220;SDL_FRect nr={x,y,iw,ih};bool ol=false;for(const auto&o:g_obstacles){SDL_FRect b=o.rect;b.x-=ob;b.y-=ob;b.w+=2*ob;b.h+=2*ob;if(checkRectCollision(nr,b)){ol=true;break;}}if(!ol)g_collectibles.push_back(Collectible(x,y,iw,ih,{255,255,0,255}));}}
void createDamageItems() { g_damageItems.clear();const float iw=40,ih=40,ob=15,cb=10;for(float x=1200;x<TRACK_LENGTH-300;x+=500+(rand()%300)){float y;int yc=rand()%2;if(yc==0)y=GROUND_Y-ih-5;else y=GROUND_Y-100;SDL_FRect nr={x,y,iw,ih};bool ol=false;for(const auto&o:g_obstacles){SDL_FRect b=o.rect;b.x-=ob;b.y-=ob;b.w+=2*ob;b.h+=2*ob;if(checkRectCollision(nr,b)){ol=true;break;}}if(ol)continue;for(const auto&c:g_collectibles){SDL_FRect b=c.rect;b.x-=cb;b.y-=cb;b.w+=2*cb;b.h+=2*cb;if(!c.isCollected&&checkRectCollision(nr,b)){ol=true;break;}}if(ol)continue;g_damageItems.push_back(DamageItem(x,y,iw,ih,{255,0,0,255}));}} // DamageItem constructor now includes velocity etc.
void createMysteryItems() { g_mysteryItems.clear();const float iw=40,ih=40,ob=20,ib=15;for(float x=900;x<TRACK_LENGTH-500;x+=1200+(rand()%800)){float y=GROUND_Y-160-(rand()%50);SDL_FRect nr={x,y,iw,ih};bool ol=false;for(const auto&o:g_obstacles){SDL_FRect b=o.rect;b.x-=ob;b.y-=ob;b.w+=2*ob;b.h+=2*ob;if(checkRectCollision(nr,b)){ol=true;break;}}if(ol)continue;for(const auto&c:g_collectibles){SDL_FRect b=c.rect;b.x-=ib;b.y-=ib;b.w+=2*ib;b.h+=2*ib;if(!c.isCollected&&checkRectCollision(nr,b)){ol=true;break;}}if(ol)continue;for(const auto&d:g_damageItems){SDL_FRect b=d.rect;b.x-=ib;b.y-=ib;b.w+=2*ib;b.h+=2*ib;if(!d.isCollected&&checkRectCollision(nr,b)){ol=true;break;}}if(ol)continue;g_mysteryItems.push_back(MysteryItem(x,y,iw,ih,{128,0,128,255}));}}

// --- RENDER FUNCTIONS ---
void renderRoundedButton(SDL_Renderer* renderer, const Button& btn, TTF_Font* font, SDL_Texture* bgTexture, SDL_Color bc, SDL_Color tc) { if(!renderer||!font||!bgTexture)return;SDL_RenderTexture(renderer,bgTexture,nullptr,&btn.rect);SDL_Surface*s=TTF_RenderText_Blended(font,btn.text.c_str(),btn.text.length(),tc);if(!s)return;SDL_Texture*t=SDL_CreateTextureFromSurface(renderer,s);SDL_FRect tr={btn.rect.x+(btn.rect.w-s->w)/2.0f,btn.rect.y+(btn.rect.h-s->h)/2.0f,(float)s->w,(float)s->h};SDL_RenderTexture(renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t); }
bool checkCollision(float x, float y, const SDL_FRect& rect) { return (x>=rect.x&&x<rect.x+rect.w&&y>=rect.y&&y<rect.y+rect.h); }

// ✅ --- DAMAGE ITEM UPDATE FUNCTION ---
void updateDamageItems(float dt) {
    for (auto& item : g_damageItems) {
        if (!item.isCollected) {
            // Update X position
            item.rect.x += item.velocityX * dt;

            // Reverse direction at edges of move range
            if (item.velocityX > 0 && item.rect.x >= item.startX + item.moveRange) {
                item.rect.x = item.startX + item.moveRange; // Clamp position
                item.velocityX = -item.velocityX; // Reverse
            } else if (item.velocityX < 0 && item.rect.x <= item.startX - item.moveRange) {
                item.rect.x = item.startX - item.moveRange; // Clamp position
                item.velocityX = -item.velocityX; // Reverse
            }
        }
    }
}

// --- PLAYER UPDATE FUNCTION (MARIO-LIKE PHYSICS & COLLISION V9) ---
void updatePlayer(float dt, bool isMovingLeft, bool isMovingRight, bool isJumpHeld) {
    // --- 1. HORIZONTAL MOVEMENT ---
    float current_accel = MOVE_ACCELERATION;
    if (isMovingRight) { player.velocityX += current_accel * dt; if (player.velocityX > g_maxMoveSpeed) player.velocityX = g_maxMoveSpeed; }
    else if (isMovingLeft) { player.velocityX -= current_accel * dt; if (player.velocityX < -g_maxMoveSpeed) player.velocityX = -g_maxMoveSpeed; }
    // Friction applied later (Step 5)

    // --- 2. X-AXIS COLLISION ---
    player.rect.x += player.velocityX * dt;
    for (const auto& obs : g_obstacles) {
        if (player.rect.y < obs.rect.y + obs.rect.h && player.rect.y + player.rect.h > obs.rect.y) {
            if (checkRectCollision(player.rect, obs.rect)) {
                if (player.velocityX > 0) player.rect.x = obs.rect.x - player.rect.w - EPSILON;
                else if (player.velocityX < 0) player.rect.x = obs.rect.x + obs.rect.w + EPSILON;
                player.velocityX = 0; break;
            }
        }
    }

    // --- 3. VERTICAL MOVEMENT ---
    float currentGravity = GRAVITY;
    if (isJumpHeld && player.velocityY < 0) currentGravity *= JUMP_GRAVITY_MODIFIER;
    player.velocityY += currentGravity * dt;
    if (!isJumpHeld && player.velocityY < 0) { if (player.velocityY < -150.0f) player.velocityY *= JUMP_CUT_VELOCITY_REDUCTION; }

    // --- 4. Y-AXIS COLLISION ---
    player.rect.y += player.velocityY * dt;
    player.onGround = false;
    if (player.rect.y + player.rect.h > GROUND_Y) { player.rect.y = GROUND_Y - player.rect.h; if (player.velocityY > 0) player.velocityY = 0; player.onGround = true; }
    for (const auto& obs : g_obstacles) {
        if (player.rect.x < obs.rect.x + obs.rect.w && player.rect.x + player.rect.w > obs.rect.x) {
            if (checkRectCollision(player.rect, obs.rect)) {
                if (player.velocityY > 0) { float prevBot = (player.rect.y - player.velocityY * dt) + player.rect.h; if (prevBot <= obs.rect.y + EPSILON * 2) { player.rect.y = obs.rect.y - player.rect.h - EPSILON; player.velocityY = 0; player.onGround = true; break; } }
                else if (player.velocityY < 0) { float prevTop = player.rect.y - player.velocityY * dt; if (prevTop >= obs.rect.y + obs.rect.h - EPSILON * 2) { player.rect.y = obs.rect.y + obs.rect.h + EPSILON; player.velocityY = 0; break; } }
            }
        }
    }
     if (!player.onGround && fabs(player.velocityY) < GRAVITY * dt * 1.5) { SDL_FRect feetRect = player.rect; feetRect.y += EPSILON * 5; if (feetRect.y + feetRect.h >= GROUND_Y) { player.onGround = true; } else { for (const auto& obs : g_obstacles) { if (player.rect.x < obs.rect.x + obs.rect.w && player.rect.x + player.rect.w > obs.rect.x) { if(checkRectCollision(feetRect, obs.rect)){ player.onGround = true; break; } } } } if (player.onGround && player.velocityY > 0) player.velocityY = 0; }

    // --- 5. APPLY FRICTION/AIR RESISTANCE ---
     if (!isMovingLeft && !isMovingRight) { float frictionFactor = player.onGround ? GROUND_FRICTION_FACTOR : AIR_RESISTANCE_FACTOR; player.velocityX *= powf(frictionFactor, dt * 60.0f); if (fabs(player.velocityX) < 1.0f) { player.velocityX = 0; } }

    // --- 6. OTHER GAME LOGIC ---
    if (player.rect.x < g_cameraX) { player.rect.x = g_cameraX; if (player.velocityX < 0) player.velocityX = 0; }
    for (auto& obs : g_obstacles) { if (!obs.isPassed && player.rect.x + player.rect.w / 2 > obs.rect.x + obs.rect.w) { obs.isPassed = true; g_score += 10; } }
    for (auto& item : g_collectibles) { if (!item.isCollected && checkRectCollision(player.rect, item.rect)) { item.isCollected = true; g_itemCount++; g_totalItemCount++; g_score += 5; if (g_itemCount >= ITEMS_PER_LEVEL) { g_level++; g_itemCount = 0; g_maxMoveSpeed *= 1.05f; } } }
    // Check collision with MOVING damage items
    for (auto& item : g_damageItems) { if (!item.isCollected && checkRectCollision(player.rect, item.rect)) { item.isCollected = true; player.hp -= 20; if (player.hp <= 0) { player.hp = 0; g_currentScene = Scene::GAME_OVER; } } }
    for (auto& item : g_mysteryItems) { if (!item.isCollected && checkRectCollision(player.rect, item.rect)) { item.isCollected = true; int effect = rand() % 3; switch (effect) { case 0: player.hp += 20; if (player.hp > 100) player.hp = 100; break; case 1: g_itemCount++; g_totalItemCount++; g_score += 20; if (g_itemCount >= ITEMS_PER_LEVEL) { g_level++; g_itemCount = 0; g_maxMoveSpeed *= 1.05f; } break; case 2: player.hp -= 20; if (player.hp <= 0) { player.hp = 0; g_currentScene = Scene::GAME_OVER; } break; } } }
    g_cameraX = player.rect.x - 200; if (g_cameraX < 0) g_cameraX = 0;
    if (player.rect.x >= TRACK_LENGTH && g_currentScene != Scene::GAME_OVER) { g_currentScene = Scene::FINISH; }
    if (player.rect.y > SCREEN_HEIGHT + player.rect.h * 2 && g_currentScene != Scene::GAME_OVER) { g_currentScene = Scene::GAME_OVER; }
}


// --- SCENE RENDERING FUNCTIONS ---
void renderScenePlay(float dt, bool isMovingLeft, bool isMovingRight, bool isJumpHeld) {
    SDL_SetRenderDrawColor(g_renderer, 135, 206, 250, 255); SDL_RenderClear(g_renderer);
    // Render Background
    if(g_backgroundTexture&&g_bgWidth>0&&g_bgHeight>0){float p=0.5f,s=(float)SCREEN_HEIGHT/g_bgHeight,sw=g_bgWidth*s,o=fmod(g_cameraX*p,sw); SDL_FRect r1={-o,0,sw,(float)SCREEN_HEIGHT},r2={-o+sw,0,sw,(float)SCREEN_HEIGHT}; SDL_RenderTexture(g_renderer,g_backgroundTexture,nullptr,&r1); SDL_RenderTexture(g_renderer,g_backgroundTexture,nullptr,&r2); }
    // Render Obstacles, Collectibles, Mystery Items
    for(const auto&o:g_obstacles){ SDL_FRect r={o.rect.x-g_cameraX,o.rect.y,o.rect.w,o.rect.h}; if(g_obstacleTexture)SDL_RenderTexture(g_renderer,g_obstacleTexture,nullptr,&r); else{/*Fallback*/}}
    for(const auto&i:g_collectibles){if(!i.isCollected){SDL_FRect r={i.rect.x-g_cameraX,i.rect.y,i.rect.w,i.rect.h};if(g_collectibleTexture)SDL_RenderTexture(g_renderer,g_collectibleTexture,nullptr,&r); else{/*Fallback*/}}}
    for(const auto&i:g_mysteryItems){if(!i.isCollected){SDL_FRect r={i.rect.x-g_cameraX,i.rect.y,i.rect.w,i.rect.h};if(g_mysteryItemTexture)SDL_RenderTexture(g_renderer,g_mysteryItemTexture,nullptr,&r); else{/*Fallback*/}}}

    // ✅ UPDATE DAMAGE ITEMS before drawing them
    updateDamageItems(dt);

    // Render Damage Items
    for(const auto&i:g_damageItems){if(!i.isCollected){SDL_FRect r={i.rect.x-g_cameraX,i.rect.y,i.rect.w,i.rect.h};if(g_damageItemTexture)SDL_RenderTexture(g_renderer,g_damageItemTexture,nullptr,&r); else{/*Fallback*/}}}

    // UPDATE PLAYER (After items are updated, before player is drawn)
    updatePlayer(dt, isMovingLeft, isMovingRight, isJumpHeld);

    // Render Player
    SDL_FRect playerRenderRect = { player.rect.x - g_cameraX, player.rect.y, player.rect.w, player.rect.h }; SDL_RenderTexture(g_renderer, g_player, nullptr, &playerRenderRect);
    // Render HUD
    SDL_Color tc={255,255,255,255}; SDL_Surface*s=nullptr; SDL_Texture*t=nullptr; SDL_FRect tr; float c1=50,c2=250,c3=450,c4=650; s=TTF_RenderText_Blended(g_smallFont,"UET",0,tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={c1,20,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} std::string it=std::to_string(g_totalItemCount);s=TTF_RenderText_Blended(g_smallFont,it.c_str(),it.length(),tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={c1,50,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} s=TTF_RenderText_Blended(g_smallFont,"HP",0,tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={c2,20,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} std::string ht=std::to_string(player.hp);s=TTF_RenderText_Blended(g_smallFont,ht.c_str(),ht.length(),tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={c2,50,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} s=TTF_RenderText_Blended(g_smallFont,"LEVEL",0,tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={c3,20,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} std::string lt=std::to_string(g_level);s=TTF_RenderText_Blended(g_smallFont,lt.c_str(),lt.length(),tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={c3,50,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} s=TTF_RenderText_Blended(g_smallFont,"TIME",0,tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={c4,20,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} g_playTimeSeconds=(SDL_GetTicks()-g_gameStartTime)/1000; std::string tt=std::to_string(g_playTimeSeconds);s=TTF_RenderText_Blended(g_smallFont,tt.c_str(),tt.length(),tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={c4,50,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);}
}

void renderSceneFinish() { SDL_SetRenderDrawColor(g_renderer, 255, 215, 0, 255); SDL_RenderClear(g_renderer); SDL_Color tc={0,0,0,255}; SDL_Surface*s=nullptr; SDL_Texture*t=nullptr; SDL_FRect tr; s=TTF_RenderText_Blended(g_font,"CHUC MUNG!",0,tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={SCREEN_WIDTH/2.0f-s->w/2.0f,100,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} std::string st="Vat pham: "+std::to_string(g_totalItemCount);s=TTF_RenderText_Blended(g_font,st.c_str(),st.length(),tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={SCREEN_WIDTH/2.0f-s->w/2.0f,200,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} std::string tt="Thoi gian: "+std::to_string(g_playTimeSeconds)+"s";s=TTF_RenderText_Blended(g_smallFont,tt.c_str(),tt.length(),tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={SCREEN_WIDTH/2.0f-s->w/2.0f,300,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} s=TTF_RenderText_Blended(g_smallFont,"Bam ESC de ve Menu",0,tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={SCREEN_WIDTH/2.0f-s->w/2.0f,400,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} }
void renderSceneGameOver() { SDL_SetRenderDrawColor(g_renderer, 139, 0, 0, 255); SDL_RenderClear(g_renderer); SDL_Color tc={255,255,255,255}; SDL_Surface*s=nullptr; SDL_Texture*t=nullptr; SDL_FRect tr; s=TTF_RenderText_Blended(g_font,"GAME OVER",0,tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={SCREEN_WIDTH/2.0f-s->w/2.0f,150,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} std::string st="Vat pham: "+std::to_string(g_totalItemCount);s=TTF_RenderText_Blended(g_smallFont,st.c_str(),st.length(),tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={SCREEN_WIDTH/2.0f-s->w/2.0f,300,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} s=TTF_RenderText_Blended(g_smallFont,"Bam ESC de ve Menu",0,tc);if(s){t=SDL_CreateTextureFromSurface(g_renderer,s);tr={SCREEN_WIDTH/2.0f-s->w/2.0f,400,(float)s->w,(float)s->h};SDL_RenderTexture(g_renderer,t,nullptr,&tr);SDL_DestroySurface(s);SDL_DestroyTexture(t);} }

// ✅ Cập nhật hàm vẽ màn hình Pause
void renderSceneResume() {
    SDL_SetRenderDrawColor(g_renderer, 150, 150, 255, 255); // Màu nền pause
    SDL_RenderClear(g_renderer);
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(g_font, "PAUSED (Press P or Click to Resume)", 0, textColor);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FRect textRect = { SCREEN_WIDTH / 2.0f - surface->w / 2.0f, SCREEN_HEIGHT / 2.0f - surface->h / 2.0f, (float)surface->w, (float)surface->h };
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture);
    }
     surface = TTF_RenderText_Blended(g_smallFont, "Press ESC for Menu", 0, textColor);
     if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FRect textRect = { SCREEN_WIDTH / 2.0f - surface->w / 2.0f, SCREEN_HEIGHT / 2.0f + 50.0f, (float)surface->w, (float)surface->h };
        SDL_RenderTexture(g_renderer, texture, nullptr, &textRect); SDL_DestroySurface(surface); SDL_DestroyTexture(texture);
    }
}
void renderSceneScore() { SDL_SetRenderDrawColor(g_renderer, 255, 165, 0, 255); SDL_RenderClear(g_renderer); /* Placeholder */ }
void renderSceneMenu(Uint32 currentTime) { if(g_backgroundTexture&&g_bgWidth>0&&g_bgHeight>0){float scrollSpeed=30.0f;float dt=(currentTime>g_lastTime)?(currentTime-g_lastTime)/1000.0f:0.0f;if(dt>0.05f)dt=0.05f;g_menuBgOffsetX+=scrollSpeed*dt;float s=(float)SCREEN_HEIGHT/g_bgHeight,sw=g_bgWidth*s,o=fmod(g_menuBgOffsetX,sw);SDL_FRect r1={-o,0,sw,(float)SCREEN_HEIGHT},r2={-o+sw,0,sw,(float)SCREEN_HEIGHT};SDL_RenderTexture(g_renderer,g_backgroundTexture,nullptr,&r1);SDL_RenderTexture(g_renderer,g_backgroundTexture,nullptr,&r2);}else{SDL_SetRenderDrawColor(g_renderer,173,216,230,255);SDL_RenderClear(g_renderer);} if(g_logoTexture){if(g_alpha<255&&!g_shrinking){g_alpha=(Uint8)SDL_min(g_alpha+3,255);SDL_SetTextureAlphaMod(g_logoTexture,g_alpha);}else{g_shrinking=true;}if(g_shrinking){if(g_logoRect.w>100){g_logoRect.w*=0.98f;g_logoRect.h*=0.98f;g_logoRect.x=(SCREEN_WIDTH-g_logoRect.w)/2.0f;g_logoRect.y=50.0f;}else{g_logoRect.x=20.0f;g_logoRect.y=20.0f;g_logoRect.w=100.0f;g_logoRect.h=100.0f;}}SDL_RenderTexture(g_renderer,g_logoTexture,nullptr,&g_logoRect);} if(g_player){float lw=100,lh=120;SDL_FRect lr={150.0f,SCREEN_HEIGHT-lh-50.0f,lw,lh};lr.y+=sinf((float)currentTime/500.0f)*5.0f;SDL_RenderTexture(g_renderer,g_player,nullptr,&lr);} g_buttons[0].rect={350,200,180,80};g_buttons[1].rect={350,300,180,80};g_buttons[2].rect={350,400,180,80}; if(currentTime-g_startTime>2000){SDL_Color bc={255,105,180,200},tc={80,80,80,255};for(auto&b:g_buttons){renderRoundedButton(g_renderer,b,g_font,g_buttonTexture,bc,tc);}} }

// --- RESET FUNCTION ---
void resetPlayer() {
    player.rect.x = 100; player.rect.y = 300;
    player.velocityX = 0; player.velocityY = 0; player.isJumping = false; player.onGround = false; player.hp = 100;
    g_cameraX = 0; g_score = 0; g_level = 1;
    g_maxMoveSpeed = MAX_MOVE_SPEED; // Reset max speed
    g_itemCount = 0; g_totalItemCount = 0; g_gameStartTime = SDL_GetTicks(); g_playTimeSeconds = 0;
    createObstacles(); createCollectibles(); createDamageItems(); createMysteryItems();
}


// --- MAIN FUNCTION ---
int main(int argc, char* argv[]) {
    // Initialization
    if (!SDL_Init(SDL_INIT_VIDEO)) { std::cout << "SDL_Init failed: " << SDL_GetError() << "\n"; return 1; }
    if (!TTF_Init()) { std::cout << "TTF_Init failed: " << SDL_GetError() << "\n"; SDL_Quit(); return 1; }
    srand((unsigned int)time(NULL));
    g_window = SDL_CreateWindow("UET_RUN", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) { std::cout << "Failed to create renderer: " << SDL_GetError() << "\n"; SDL_DestroyWindow(g_window); TTF_Quit(); SDL_Quit(); return 1; }
    SDL_SetRenderVSync(g_renderer, 1);

    // Load Textures
    g_logoTexture = IMG_LoadTexture(g_renderer, "Assets/uet.png"); g_buttonTexture = IMG_LoadTexture(g_renderer, "Assets/button.png"); g_player = IMG_LoadTexture(g_renderer, "Assets/player.png"); g_damageItemTexture = IMG_LoadTexture(g_renderer, "Assets/bad.png"); g_collectibleTexture = IMG_LoadTexture(g_renderer, "Assets/item.png"); g_mysteryItemTexture = IMG_LoadTexture(g_renderer, "Assets/mystery.png"); g_obstacleTexture = IMG_LoadTexture(g_renderer, "Assets/deadline.png"); g_backgroundTexture = IMG_LoadTexture(g_renderer, "Assets/background.png"); if (g_backgroundTexture) SDL_GetTextureSize(g_backgroundTexture, &g_bgWidth, &g_bgHeight); else std::cout << "Failed to load Assets/background.png: " << SDL_GetError() << "\n";

    // Load Fonts
    g_font = TTF_OpenFont("NotoSans-Regular.ttf", 36); g_smallFont = TTF_OpenFont("NotoSans-Regular.ttf", 24); if (!g_font || !g_smallFont) { std::cout << "Failed to load font: " << SDL_GetError() << "\n"; /* Cleanup */ if(g_renderer)SDL_DestroyRenderer(g_renderer); if(g_window)SDL_DestroyWindow(g_window); TTF_Quit(); SDL_Quit(); return 1; }

    g_startTime = SDL_GetTicks(); g_lastTime = SDL_GetTicks();

    // Main Loop
    bool running = true; SDL_Event e;
    while (running) {
        Uint64 frameStartTime = SDL_GetTicks(); float dt = (frameStartTime - g_lastTime) / 1000.0f; g_lastTime = frameStartTime; if (dt > 0.05f) dt = 0.05f;
        Uint32 menuCurrentTime = SDL_GetTicks();

        // Event Handling
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) { if (g_currentScene == Scene::PLAY || g_currentScene == Scene::RESUME) g_currentScene = Scene::MENU; else if (g_currentScene == Scene::FINISH || g_currentScene == Scene::GAME_OVER) g_currentScene = Scene::MENU; else if (g_currentScene == Scene::MENU) running = false; }
                // ✅ SỬA LỖI: Dùng SDLK_P (chữ hoa)
                else if (e.key.key == SDLK_P) { if (g_currentScene == Scene::PLAY) { g_currentScene = Scene::RESUME; std::cout << "Paused\n";} else if (g_currentScene == Scene::RESUME) { g_currentScene = Scene::PLAY; g_lastTime = SDL_GetTicks(); std::cout << "Resumed\n";} } // Pause/Resume Key
                else if (g_currentScene == Scene::PLAY && (e.key.key == SDLK_W || e.key.key == SDLK_SPACE) && player.onGround) { player.velocityY = JUMP_INITIAL_VELOCITY; player.onGround = false; } // Jump
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                 if (g_currentScene == Scene::MENU && menuCurrentTime - g_startTime > 2000) { float mx=(float)e.button.x, my=(float)e.button.y; if (checkCollision(mx,my,g_buttons[0].rect)) { g_currentScene = Scene::PLAY; resetPlayer(); } else if (checkCollision(mx,my,g_buttons[1].rect)) { g_currentScene = Scene::PLAY; resetPlayer(); } else if (checkCollision(mx,my,g_buttons[2].rect)) { /* Score TBD */ } }
                 else if (g_currentScene == Scene::RESUME) { g_currentScene = Scene::PLAY; g_lastTime = SDL_GetTicks(); std::cout << "Resumed via Click\n";} // Click to Resume
            }
        }

        const bool* keystate = SDL_GetKeyboardState(nullptr);
        bool isMovingLeft = keystate[SDL_SCANCODE_A];
        bool isMovingRight = keystate[SDL_SCANCODE_D];
        bool isJumpHeld = keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_SPACE];

        // Render Current Scene
        switch (g_currentScene) {
             case Scene::MENU: renderSceneMenu(menuCurrentTime); break;
             case Scene::PLAY: renderScenePlay(dt, isMovingLeft, isMovingRight, isJumpHeld); break; // Pass states
             case Scene::RESUME: renderSceneResume(); break; // Draw pause screen, no update
             case Scene::SCORE: renderSceneScore(); break;
             case Scene::FINISH: renderSceneFinish(); break;
             case Scene::GAME_OVER: renderSceneGameOver(); break;
         }
        SDL_RenderPresent(g_renderer); // Show frame
    }

    // Cleanup
    // ✅ SỬA CẢNH BÁO: Tách if ra nhiều dòng
    if (g_font) TTF_CloseFont(g_font);
    if (g_smallFont) TTF_CloseFont(g_smallFont);
    if (g_logoTexture) SDL_DestroyTexture(g_logoTexture);
    if (g_buttonTexture) SDL_DestroyTexture(g_buttonTexture);
    if (g_player) SDL_DestroyTexture(g_player);
    if (g_backgroundTexture) SDL_DestroyTexture(g_backgroundTexture);
    if (g_damageItemTexture) SDL_DestroyTexture(g_damageItemTexture);
    if (g_collectibleTexture) SDL_DestroyTexture(g_collectibleTexture);
    if (g_mysteryItemTexture) SDL_DestroyTexture(g_mysteryItemTexture);
    if (g_obstacleTexture) SDL_DestroyTexture(g_obstacleTexture);
    if (g_renderer) SDL_DestroyRenderer(g_renderer);
    if (g_window) SDL_DestroyWindow(g_window);

    TTF_Quit();
    SDL_Quit();
    return 0;
}
