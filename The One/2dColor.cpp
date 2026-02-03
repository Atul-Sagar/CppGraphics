#include <windows.h>
#include <sstream>
#include <vector>
#include <string>
#include <random>
#include <ctime>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// ================= CONSTANTS =================
const int SCREEN_W = 800;
const int SCREEN_H = 600;
const int PLAYER_W = 40;
const int PLAYER_H = 50;
const int GROUND_Y = 400;
const int LEVEL_END_X = 2200;

const float GRAVITY = 0.8f;
const float MOVE_SPEED = 5.0f;
const float JUMP_FORCE = -15.0f;
const float MAX_FALL_SPEED = 20.0f;
const float CAMERA_SMOOTHNESS = 0.1f;

// ================= GAME STATE =================
enum GameState {
    STATE_START,
    STATE_PLAYING,
    STATE_FINISH,
    STATE_DEAD,
    STATE_PAUSED
};

GameState gameState = STATE_START;

// ================= STRUCTURES =================
struct Ground {
    int x1;
    int x2;
    bool hasSpike = false;
};

struct Enemy {
    float x;
    float y;
    float speed;
    int dir;
    int patrolStart;
    int patrolEnd;
    bool isActive = true;
};

struct Particle {
    float x, y;
    float vx, vy;
    int lifetime;
    COLORREF color;
};

struct Collectible {
    float x, y;
    bool collected;
    int type; // 0 = coin, 1 = health, 2 = powerup
};

// ================= GAME DATA =================
float playerX = 100;
float playerY = 300;
float velY = 0;
float cameraX = 0;
float cameraTargetX = 0;
bool leftKey = false;
bool rightKey = false;
bool onGround = false;
int score = 0;
int lives = 3;
int coinsCollected = 0;
float gameTime = 0.0f;
bool playerInvincible = false;
float invincibleTimer = 0.0f;

// ================= LEVEL DATA =================
std::vector<Ground> groundSegments = {
    {0, 500},
    {650, 1000, true},
    {1150, 1500},
    {1650, 2100}
};

std::vector<Enemy> enemies = {
    {350, 350, 2.0f, 1, 300, 500},
    {800, 350, 2.5f, -1, 700, 900},
    {1300, 350, 2.0f, 1, 1250, 1350},
    {1800, 350, 2.5f, -1, 1700, 1900}
};

std::vector<Collectible> collectibles = {
    {200, 300, false, 0},
    {400, 250, false, 0},
    {750, 300, false, 1},
    {1200, 280, false, 0},
    {1400, 250, false, 2},
    {1900, 300, false, 0}
};

std::vector<Particle> particles;

// ================= RANDOM =================
std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
std::uniform_real_distribution<float> dist(-2.0f, 2.0f);

// ================= FUNCTIONS =================
void resetGame() {
    playerX = 100;
    playerY = 300;
    velY = 0;
    cameraX = 0;
    cameraTargetX = 0;
    score = 0;
    coinsCollected = 0;
    lives = 3;
    gameTime = 0.0f;
    playerInvincible = false;

    for (auto& col : collectibles) {
        col.collected = false;
    }

    for (auto& enemy : enemies) {
        enemy.isActive = true;
    }

    particles.clear();

    if (gameState == STATE_DEAD || gameState == STATE_FINISH) {
        gameState = STATE_START;
    }
}

void addParticles(float x, float y, int count, COLORREF color) {
    for (int i = 0; i < count; i++) {
        particles.push_back({
            x, y,
            dist(rng), dist(rng) - 2.0f,
            30 + rand() % 30,
            color
            });
    }
}

bool isOnGround(float x, float y) {
    for (const auto& ground : groundSegments) {
        if (x + PLAYER_W > ground.x1 &&
            x < ground.x2) {
            if (y + PLAYER_H >= GROUND_Y - 1 &&
                y + PLAYER_H <= GROUND_Y + 10) {
                return true;
            }
        }
    }
    return false;
}

bool checkSpikeCollision(float x, float y) {
    for (const auto& ground : groundSegments) {
        if (ground.hasSpike &&
            x + PLAYER_W > ground.x1 &&
            x < ground.x2 &&
            y + PLAYER_H >= GROUND_Y - 20 &&
            y < GROUND_Y + 10) {
            return true;
        }
    }
    return false;
}

void updateCamera() {
    cameraTargetX = playerX - SCREEN_W / 2;

    if (cameraTargetX < 0) cameraTargetX = 0;
    if (cameraTargetX > LEVEL_END_X - SCREEN_W)
        cameraTargetX = LEVEL_END_X - SCREEN_W;

    cameraX += (cameraTargetX - cameraX) * CAMERA_SMOOTHNESS;
}

void updateParticles() {
    for (auto it = particles.begin(); it != particles.end();) {
        it->x += it->vx;
        it->y += it->vy;
        it->vy += 0.1f;
        it->lifetime--;

        if (it->lifetime <= 0) {
            it = particles.erase(it);
        }
        else {
            ++it;
        }
    }
}

void updateCollectibles() {
    for (auto& col : collectibles) {
        if (!col.collected) {
            RECT p = {
                (LONG)playerX,
                (LONG)playerY,
                (LONG)(playerX + PLAYER_W),
                (LONG)(playerY + PLAYER_H)
            };

            RECT c = {
                (LONG)col.x,
                (LONG)col.y,
                (LONG)(col.x + 20),
                (LONG)(col.y + 20)
            };

            RECT out;
            if (IntersectRect(&out, &p, &c)) {
                col.collected = true;
                score += 100;
                coinsCollected++;

                COLORREF color = RGB(255, 215, 0); // Gold for coins
                if (col.type == 1) {
                    color = RGB(0, 255, 0); // Green for health
                    lives = min(lives + 1, 5);
                }
                else if (col.type == 2) {
                    color = RGB(255, 0, 255); // Purple for powerup
                    playerInvincible = true;
                    invincibleTimer = 5.0f;
                }

                addParticles(col.x + 10, col.y + 10, 15, color);
            }
        }
    }
}

void updateEnemies() {
    for (auto& enemy : enemies) {
        if (!enemy.isActive) continue;

        enemy.x += enemy.speed * enemy.dir;

        if (enemy.x < enemy.patrolStart || enemy.x > enemy.patrolEnd) {
            enemy.dir *= -1;
        }

        enemy.y = GROUND_Y - PLAYER_H;

        if (!playerInvincible) {
            RECT p = {
                (LONG)playerX,
                (LONG)playerY,
                (LONG)(playerX + PLAYER_W),
                (LONG)(playerY + PLAYER_H)
            };

            RECT e = {
                (LONG)enemy.x,
                (LONG)enemy.y,
                (LONG)(enemy.x + PLAYER_W),
                (LONG)(enemy.y + PLAYER_H)
            };

            RECT out;
            if (IntersectRect(&out, &p, &e)) {
                lives--;
                playerInvincible = true;
                invincibleTimer = 2.0f;

                if (lives <= 0) {
                    gameState = STATE_DEAD;
                }

                addParticles(playerX + PLAYER_W / 2, playerY + PLAYER_H / 2, 20, RGB(255, 50, 50));
            }
        }
    }
}

void updateGame() {
    if (gameState != STATE_PLAYING) return;

    gameTime += 0.016f;

    if (playerInvincible) {
        invincibleTimer -= 0.016f;
        if (invincibleTimer <= 0) {
            playerInvincible = false;
        }
    }

    // Input handling
    if (leftKey)  playerX -= MOVE_SPEED;
    if (rightKey) playerX += MOVE_SPEED;

    // Physics
    velY += GRAVITY;
    if (velY > MAX_FALL_SPEED) velY = MAX_FALL_SPEED;
    playerY += velY;

    // Ground collision
    if (isOnGround(playerX, playerY)) {
        playerY = GROUND_Y - PLAYER_H;
        velY = 0;
        onGround = true;
    }
    else {
        onGround = false;
    }

    // Spike collision
    if (checkSpikeCollision(playerX, playerY) && !playerInvincible) {
        lives--;
        playerInvincible = true;
        invincibleTimer = 2.0f;

        if (lives <= 0) {
            gameState = STATE_DEAD;
        }

        addParticles(playerX + PLAYER_W / 2, playerY + PLAYER_H / 2, 25, RGB(255, 100, 0));
    }

    // Death boundary
    if (playerY > SCREEN_H + 200) {
        gameState = STATE_DEAD;
    }

    // Level completion
    if (playerX >= LEVEL_END_X) {
        gameState = STATE_FINISH;
        score += (int)(10000 / gameTime) + coinsCollected * 500;
    }

    updateCamera();
    updateEnemies();
    updateCollectibles();
    updateParticles();
}

// ================= DRAWING =================
void drawAscii(HDC hdc, int x, int y, const std::vector<std::wstring>& art, COLORREF color = RGB(0, 0, 0)) {
    int lineHeight = 18;
    COLORREF oldColor = SetTextColor(hdc, color);
    for (size_t i = 0; i < art.size(); i++) {
        TextOutW(hdc, x, y + i * lineHeight, art[i].c_str(), art[i].length());
    }
    SetTextColor(hdc, oldColor);
}

void drawParticles(HDC hdc) {
    for (const auto& p : particles) {
        int alpha = p.lifetime * 255 / 60;
        COLORREF color = p.color;
        int r = GetRValue(color);
        int g = GetGValue(color);
        int b = GetBValue(color);

        HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(r, g, b));
        SelectObject(hdc, brush);
        SelectObject(hdc, pen);

        int size = 3 + p.lifetime / 20;
        Ellipse(hdc,
            p.x - cameraX - size,
            p.y - size,
            p.x - cameraX + size,
            p.y + size);

        DeleteObject(brush);
        DeleteObject(pen);
    }
}

void drawHealthBar(HDC hdc) {
    int barWidth = 100;
    int barHeight = 15;
    int barX = 10;
    int barY = 120;

    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(100, 0, 0));
    SelectObject(hdc, bgBrush);
    Rectangle(hdc, barX, barY, barX + barWidth, barY + barHeight);
    DeleteObject(bgBrush);

    // Health
    float healthPercent = lives / 3.0f;
    HBRUSH healthBrush = CreateSolidBrush(RGB(0, 255, 0));
    SelectObject(hdc, healthBrush);
    Rectangle(hdc, barX, barY,
        barX + (int)(barWidth * healthPercent),
        barY + barHeight);
    DeleteObject(healthBrush);

    // Border
    HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    SelectObject(hdc, borderPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, barX, barY, barX + barWidth, barY + barHeight);
    DeleteObject(borderPen);
}

void drawGame(HDC hdc) {
    // ===== BACKGROUND =====
    HBRUSH skyBrush = CreateSolidBrush(RGB(135, 206, 235));
    RECT skyRect = { 0, 0, SCREEN_W, GROUND_Y };
    FillRect(hdc, &skyRect, skyBrush);
    DeleteObject(skyBrush);

    // ===== GROUND =====
    for (const auto& ground : groundSegments) {
        HBRUSH groundBrush = CreateSolidBrush(RGB(100, 70, 40));
        SelectObject(hdc, groundBrush);

        Rectangle(
            hdc,
            ground.x1 - cameraX,
            GROUND_Y,
            ground.x2 - cameraX,
            GROUND_Y + 50
        );

        if (ground.hasSpike) {
            HBRUSH spikeBrush = CreateSolidBrush(RGB(200, 50, 50));
            SelectObject(hdc, spikeBrush);

            for (int x = ground.x1 + 10; x < ground.x2; x += 30) {
                POINT spike[3] = {
                    {x - cameraX, GROUND_Y},
                    {x + 10 - cameraX, GROUND_Y - 20},
                    {x + 20 - cameraX, GROUND_Y}
                };
                Polygon(hdc, spike, 3);
            }
            DeleteObject(spikeBrush);
        }

        DeleteObject(groundBrush);
    }

    // ===== COLLECTIBLES =====
    for (const auto& col : collectibles) {
        if (!col.collected) {
            HBRUSH colBrush;
            switch (col.type) {
            case 0: colBrush = CreateSolidBrush(RGB(255, 215, 0)); break; // Coin
            case 1: colBrush = CreateSolidBrush(RGB(0, 255, 0)); break;   // Health
            case 2: colBrush = CreateSolidBrush(RGB(255, 0, 255)); break; // Powerup
            default: colBrush = CreateSolidBrush(RGB(255, 255, 255));
            }

            SelectObject(hdc, colBrush);
            Ellipse(hdc,
                col.x - cameraX,
                col.y,
                col.x - cameraX + 20,
                col.y + 20);
            DeleteObject(colBrush);
        }
    }

    // ===== FINISH LINE =====
    HBRUSH finishBrush = CreateSolidBrush(RGB(0, 255, 0));
    SelectObject(hdc, finishBrush);
    Rectangle(
        hdc,
        LEVEL_END_X - cameraX,
        GROUND_Y - 60,
        LEVEL_END_X + 10 - cameraX,
        GROUND_Y
    );
    DeleteObject(finishBrush);

    // ===== ENEMIES =====
    for (const auto& enemy : enemies) {
        if (!enemy.isActive) continue;

        HBRUSH enemyBrush = CreateSolidBrush(RGB(200, 60, 60));
        SelectObject(hdc, enemyBrush);

        // Enemy body
        Rectangle(
            hdc,
            enemy.x - cameraX,
            enemy.y,
            enemy.x - cameraX + PLAYER_W,
            enemy.y + PLAYER_H
        );

        // Enemy eyes
        HBRUSH eyeBrush = CreateSolidBrush(RGB(255, 255, 255));
        SelectObject(hdc, eyeBrush);
        Ellipse(hdc,
            enemy.x - cameraX + 10,
            enemy.y + 10,
            enemy.x - cameraX + 20,
            enemy.y + 20);
        Ellipse(hdc,
            enemy.x - cameraX + 25,
            enemy.y + 10,
            enemy.x - cameraX + 35,
            enemy.y + 20);
        DeleteObject(eyeBrush);

        DeleteObject(enemyBrush);
    }

    // ===== PLAYER =====
    COLORREF playerColor = playerInvincible ?
        (fmod(gameTime, 0.2f) < 0.1f ? RGB(255, 255, 255) : RGB(100, 150, 255)) :
        RGB(100, 150, 255);

    HBRUSH playerBrush = CreateSolidBrush(playerColor);
    SelectObject(hdc, playerBrush);

    // Player body with rounded head
    Rectangle(
        hdc,
        playerX - cameraX,
        playerY + 10,
        playerX - cameraX + PLAYER_W,
        playerY + PLAYER_H
    );

    // Head
    Ellipse(hdc,
        playerX - cameraX + 5,
        playerY,
        playerX - cameraX + PLAYER_W - 5,
        playerY + 20);

    DeleteObject(playerBrush);

    // ===== PARTICLES =====
    drawParticles(hdc);

    // ===== HUD =====
    std::vector<std::wstring> playerArt = {
        L"  O  ",
        L" /|\\ ",
        L" / \\ "
    };
    drawAscii(hdc, 10, 10, playerArt, RGB(50, 100, 200));

    // Draw health bar
    drawHealthBar(hdc);

    // HUD text
    std::wstringstream ss;
    ss << L"Score: " << score << L"  Coins: " << coinsCollected;
    TextOutW(hdc, 10, 140, ss.str().c_str(), ss.str().length());

    ss.str(L"");
    ss << L"Time: " << (int)gameTime << "s";
    TextOutW(hdc, 10, 160, ss.str().c_str(), ss.str().length());

    // ===== STATE SCREENS =====
    if (gameState == STATE_START) {
        drawAscii(hdc, 200, 200, {
            L"  ____  _       _   _               ",
            L" |  _ \\| | __ _| |_| |_ ___ _ __   ",
            L" | |_) | |/ _` | __| __/ _ \\ '__|  ",
            L" |  __/| | (_| | |_| ||  __/ |     ",
            L" |_|   |_|\\__,_|\\__|\\__\\___|_|     ",
            L"",
            L"        CONTROLS:                   ",
            L"        A/D = Move                  ",
            L"        SPACE = Jump                ",
            L"        P = Pause                   ",
            L"        R = Restart                 ",
            L"",
            L"        PRESS ENTER TO START        "
            }, RGB(50, 100, 200));
    }

    if (gameState == STATE_DEAD) {
        drawAscii(hdc, 260, 200, {
            L"  YOU DIED  ",
            L"   x_x      ",
            L"",
            L" Final Score: " + std::to_wstring(score),
            L"",
            L" PRESS R TO RETRY "
            }, RGB(200, 50, 50));
    }

    if (gameState == STATE_FINISH) {
        drawAscii(hdc, 230, 200, {
            L"  STAGE CLEAR!  ",
            L"  \\o/  \\o/     ",
            L"",
            L" Score: " + std::to_wstring(score),
            L" Time: " + std::to_wstring((int)gameTime) + L"s",
            L" Coins: " + std::to_wstring(coinsCollected),
            L"",
            L" PRESS R TO PLAY AGAIN "
            }, RGB(50, 200, 50));
    }

    if (gameState == STATE_PAUSED) {
        drawAscii(hdc, 300, 200, {
            L" PAUSED ",
            L"",
            L" PRESS P TO CONTINUE "
            }, RGB(255, 255, 0));
    }
}

// ================= WINDOW PROCEDURE =================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        if (gameState == STATE_PLAYING) {
            updateGame();
        }
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_RETURN:
            if (gameState == STATE_START) gameState = STATE_PLAYING;
            break;

        case 'R':
            resetGame();
            break;

        case 'P':
            if (gameState == STATE_PLAYING) gameState = STATE_PAUSED;
            else if (gameState == STATE_PAUSED) gameState = STATE_PLAYING;
            break;

        case 'A':
            leftKey = true;
            break;

        case 'D':
            rightKey = true;
            break;

        case VK_SPACE:
            if (gameState == STATE_PLAYING && onGround) {
                velY = JUMP_FORCE;
                addParticles(playerX + PLAYER_W / 2, playerY + PLAYER_H, 10, RGB(200, 200, 200));
            }
            break;
        }
        break;

    case WM_KEYUP:
        switch (wParam) {
        case 'A':
            leftKey = false;
            break;

        case 'D':
            rightKey = false;
            break;
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        SelectObject(memDC, memBmp);

        drawGame(memDC);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ================= MAIN =================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"PitsPlatformer";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    // Adjust window size for client area
    RECT rc = { 0, 0, SCREEN_W, SCREEN_H };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindow(
        L"PitsPlatformer",
        L"2D Platformer - Enhanced Edition",
        WS_OVERLAPPEDWINDOW,
        100, 100, rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    SetTimer(hwnd, 1, 16, NULL); // ~60 FPS

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}