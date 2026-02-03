#include <windows.h>
#include <sstream>

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

// Player (WORLD coordinates)
float playerX = 100;
float playerY = 300;
float velY = 0;

// Camera
float cameraX = 0;

// Input
bool leftKey = false;
bool rightKey = false;
bool onGround = false;

// Constants
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
// Ground segments (with pits between them)
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

// =============================================

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

    // Horizontal movement
    if (leftKey)  playerX -= MOVE_SPEED;
    if (rightKey) playerX += MOVE_SPEED;

    // Gravity
    velY += GRAVITY;
    playerY += velY;

    // Ground collision check
    if (isOnGround(playerX, playerY)) {
        playerY = GROUND_Y - PLAYER_H;
        velY = 0;
        onGround = true;
    }
    else {
        onGround = false;
    }

    // Fell into pit (death)
    if (playerY > SCREEN_H + 200) {
        gameState = STATE_DEAD;
    }

    // Camera follows player
    cameraX = playerX - SCREEN_W / 2;
    if (cameraX < 0) cameraX = 0;

    // Finish
    if (playerX >= LEVEL_END_X) {
        gameState = STATE_FINISH;
    }
}

void drawText(HDC hdc, int x, int y, const std::wstring& text) {
    TextOutW(hdc, x, y, text.c_str(), text.length());
}

void drawGame(HDC hdc) {

    // Draw ground segments
    for (int i = 0; i < GROUND_COUNT; i++) {
        Rectangle(
            hdc,
            groundSegments[i].x1 - cameraX,
            GROUND_Y,
            groundSegments[i].x2 - cameraX,
            GROUND_Y + 50
        );
    }

    // Finish line
    Rectangle(
        hdc,
        LEVEL_END_X - cameraX,
        GROUND_Y - 60,
        LEVEL_END_X + 10 - cameraX,
        GROUND_Y
    );

    // Player
    Rectangle(
        hdc,
        playerX - cameraX,
        playerY,
        playerX - cameraX + PLAYER_W,
        playerY + PLAYER_H
    );

    // ================= HUD =================
    std::wstringstream ss;
    ss << L"Speed: " << (rightKey ? MOVE_SPEED : leftKey ? -MOVE_SPEED : 0);
    drawText(hdc, 10, 10, ss.str());

    ss.str(L"");
    ss << L"Height (Y): " << (int)playerY;
    drawText(hdc, 10, 30, ss.str());

    ss.str(L"");
    ss << L"World X: " << (int)playerX;
    drawText(hdc, 10, 50, ss.str());

    // ================= STATE TEXT =================
    if (gameState == STATE_START) {
        drawText(hdc, 300, 200, L"PRESS ENTER TO START");
    }
    else if (gameState == STATE_FINISH) {
        drawText(hdc, 320, 200, L"STAGE CLEAR!");
        drawText(hdc, 280, 230, L"PRESS R TO RESTART");
    }
    else if (gameState == STATE_DEAD) {
        drawText(hdc, 330, 200, L"YOU FELL!");
        drawText(hdc, 280, 230, L"PRESS R TO RETRY");
    }
}

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

            if (wParam == VK_SPACE && onGround) {
                velY = JUMP_FORCE;
                onGround = false;
            }
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

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"PitsPlatformer";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        L"PitsPlatformer",
        L"2D Platformer - Pits + HUD",
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
