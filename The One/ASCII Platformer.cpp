#include <windows.h>
#include <sstream>
#include <vector>
#include <string>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// ================= GAME STATE =================
enum GameState {
    STATE_START,
    STATE_PLAYING,
    STATE_FINISH,
    STATE_DEAD
};

GameState gameState = STATE_START;

// ================= PLAYER =================
float playerX = 100;
float playerY = 300;
float velY = 0;

// ================= CAMERA =================
float cameraX = 0;

// ================= INPUT =================
bool leftKey = false;
bool rightKey = false;
bool onGround = false;

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

// ================= LEVEL DATA =================
struct Ground {
    int x1;
    int x2;
};

Ground groundSegments[] = {
    {0, 500},
    {650, 1000},
    {1150, 1500},
    {1650, 2100}
};

const int GROUND_COUNT = sizeof(groundSegments) / sizeof(Ground);

// ================= ENEMIES =================
struct Enemy {
    float x;
    float y;
    float speed;
    int dir;
};

Enemy enemies[] = {
    {350, 350, 2.0f, 1},
    {800, 350, 2.5f, -1},
    {1300, 350, 2.0f, 1},
    {1800, 350, 2.5f, -1}
};

const int ENEMY_COUNT = sizeof(enemies) / sizeof(Enemy);

// ===========================================

void resetGame() {
    playerX = 100;
    playerY = 300;
    velY = 0;
    cameraX = 0;
    gameState = STATE_START;
}

bool isOnGround(float x, float y) {
    for (int i = 0; i < GROUND_COUNT; i++) {
        if (x + PLAYER_W > groundSegments[i].x1 &&
            x < groundSegments[i].x2) {
            if (y + PLAYER_H >= GROUND_Y)
                return true;
        }
    }
    return false;
}

void updateGame() {
    if (gameState != STATE_PLAYING)
        return;

    if (leftKey)  playerX -= MOVE_SPEED;
    if (rightKey) playerX += MOVE_SPEED;

    velY += GRAVITY;
    playerY += velY;

    if (isOnGround(playerX, playerY)) {
        playerY = GROUND_Y - PLAYER_H;
        velY = 0;
        onGround = true;
    }
    else {
        onGround = false;
    }

    if (playerY > SCREEN_H + 200)
        gameState = STATE_DEAD;

    cameraX = playerX - SCREEN_W / 2;
    if (cameraX < 0) cameraX = 0;

    if (playerX >= LEVEL_END_X)
        gameState = STATE_FINISH;

    // ===== ENEMIES =====
    for (int i = 0; i < ENEMY_COUNT; i++) {
        enemies[i].x += enemies[i].speed * enemies[i].dir;

        if (!isOnGround(enemies[i].x + enemies[i].dir * 20, enemies[i].y))
            enemies[i].dir *= -1;

        enemies[i].y = GROUND_Y - PLAYER_H;

        RECT p = {
            (LONG)playerX,
            (LONG)playerY,
            (LONG)(playerX + PLAYER_W),
            (LONG)(playerY + PLAYER_H)
        };

        RECT e = {
            (LONG)enemies[i].x,
            (LONG)enemies[i].y,
            (LONG)(enemies[i].x + PLAYER_W),
            (LONG)(enemies[i].y + PLAYER_H)
        };

        RECT out;
        if (IntersectRect(&out, &p, &e))
            gameState = STATE_DEAD;
    }
}

// ================= ASCII HELPERS =================
void drawAscii(HDC hdc, int x, int y, const std::vector<std::wstring>& art) {
    int lineHeight = 18;
    for (size_t i = 0; i < art.size(); i++) {
        TextOutW(hdc, x, y + i * lineHeight, art[i].c_str(), art[i].length());
    }
}

void drawGame(HDC hdc) {

    // ===== GROUND =====
    for (int i = 0; i < GROUND_COUNT; i++) {
        Rectangle(
            hdc,
            groundSegments[i].x1 - cameraX,
            GROUND_Y,
            groundSegments[i].x2 - cameraX,
            GROUND_Y + 50
        );
    }

    // ===== FINISH LINE =====
    Rectangle(
        hdc,
        LEVEL_END_X - cameraX,
        GROUND_Y - 60,
        LEVEL_END_X + 10 - cameraX,
        GROUND_Y
    );

    // ===== PLAYER =====
    Rectangle(
        hdc,
        playerX - cameraX,
        playerY,
        playerX - cameraX + PLAYER_W,
        playerY + PLAYER_H
    );

    // ===== ENEMIES =====
    HBRUSH enemyBrush = CreateSolidBrush(RGB(200, 60, 60));
    SelectObject(hdc, enemyBrush);

    for (int i = 0; i < ENEMY_COUNT; i++) {
        Rectangle(
            hdc,
            enemies[i].x - cameraX,
            enemies[i].y,
            enemies[i].x - cameraX + PLAYER_W,
            enemies[i].y + PLAYER_H
        );
    }
    DeleteObject(enemyBrush);

    // ===== HUD ASCII =====
    std::vector<std::wstring> playerArt = {
        L"  O  ",
        L" /|\\ ",
        L" / \\ "
    };
    drawAscii(hdc, 10, 10, playerArt);

    std::wstringstream ss;
    ss << L"X: " << (int)playerX << L"  Y: " << (int)playerY;
    TextOutW(hdc, 10, 80, ss.str().c_str(), ss.str().length());

    // ===== STATE ASCII =====
    if (gameState == STATE_START) {
        drawAscii(hdc, 200, 200, {
            L"  ____  _       _   _               ",
            L" |  _ \\| | __ _| |_| |_ ___ _ __   ",
            L" | |_) | |/ _` | __| __/ _ \\ '__|  ",
            L" |  __/| | (_| | |_| ||  __/ |     ",
            L" |_|   |_|\\__,_|\\__|\\__\\___|_|     ",
            L"",
            L"        PRESS ENTER TO START        "
            });
    }

    if (gameState == STATE_DEAD) {
        drawAscii(hdc, 260, 200, {
            L"  YOU DIED  ",
            L"   x_x      ",
            L"",
            L" PRESS R TO RETRY "
            });
    }

    if (gameState == STATE_FINISH) {
        drawAscii(hdc, 230, 200, {
            L"  STAGE CLEAR!  ",
            L"  \\o/  \\o/     ",
            L"",
            L" PRESS R TO PLAY AGAIN "
            });
    }
}

// ================= WINDOW =================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_TIMER:
        updateGame();
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_KEYDOWN:
        if (gameState == STATE_START && wParam == VK_RETURN)
            gameState = STATE_PLAYING;

        if ((gameState == STATE_FINISH || gameState == STATE_DEAD) && wParam == 'R')
            resetGame();

        if (gameState == STATE_PLAYING) {
            if (wParam == 'A') leftKey = true;
            if (wParam == 'D') rightKey = true;
            if (wParam == VK_SPACE && onGround)
                velY = JUMP_FORCE;
        }
        break;

    case WM_KEYUP:
        if (wParam == 'A') leftKey = false;
        if (wParam == 'D') rightKey = false;
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

        HBRUSH bg = CreateSolidBrush(RGB(200, 230, 255));
        FillRect(memDC, &rc, bg);
        DeleteObject(bg);

        HFONT font = CreateFontW(
            18, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            FIXED_PITCH | FF_MODERN,
            L"Courier New"
        );
        SelectObject(memDC, font);

        drawGame(memDC);

        DeleteObject(font);

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

// ================= ENTRY =================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"PitsPlatformer";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        L"PitsPlatformer",
        L"2D Platformer - ASCII Edition",
        WS_OVERLAPPEDWINDOW,
        100, 100, SCREEN_W, SCREEN_H,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    SetTimer(hwnd, 1, 16, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
