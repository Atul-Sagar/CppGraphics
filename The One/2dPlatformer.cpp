#include <windows.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// ================= GAME STATE =================
enum GameState {
    STATE_START,
    STATE_PLAYING,
    STATE_FINISH
};

GameState gameState = STATE_START;

// Player
float playerX = 100;
float playerY = 300;
float velY = 0;

bool leftKey = false;
bool rightKey = false;
bool onGround = false;

const int PLAYER_W = 40;
const int PLAYER_H = 50;

// Stage
const int GROUND_Y = 400;
const int FINISH_X = 700;

// Physics
const float GRAVITY = 0.8f;
const float MOVE_SPEED = 5.0f;
const float JUMP_FORCE = -15.0f;

// =============================================

void resetGame() {
    playerX = 100;
    playerY = 300;
    velY = 0;
    gameState = STATE_START;
}

void updateGame() {
    if (gameState != STATE_PLAYING)
        return;

    if (leftKey)  playerX -= MOVE_SPEED;
    if (rightKey) playerX += MOVE_SPEED;

    velY += GRAVITY;
    playerY += velY;

    if (playerY + PLAYER_H >= GROUND_Y) {
        playerY = GROUND_Y - PLAYER_H;
        velY = 0;
        onGround = true;
    }
    else {
        onGround = false;
    }

    // Win condition
    if (playerX >= FINISH_X) {
        gameState = STATE_FINISH;
    }
}

void drawText(HDC hdc, int x, int y, const wchar_t* text) {
    TextOut(hdc, x, y, text, lstrlenW(text));
}

void drawGame(HDC hdc, int w, int h) {
    // Ground
    Rectangle(hdc, 0, GROUND_Y, w, GROUND_Y + 50);

    // Finish line
    Rectangle(hdc, FINISH_X, GROUND_Y - 60, FINISH_X + 10, GROUND_Y);

    // Player
    Rectangle(
        hdc,
        (int)playerX,
        (int)playerY,
        (int)playerX + PLAYER_W,
        (int)playerY + PLAYER_H
    );

    // UI
    if (gameState == STATE_START) {
        drawText(hdc, 300, 200, L"PRESS ENTER TO START");
    }
    else if (gameState == STATE_FINISH) {
        drawText(hdc, 320, 200, L"STAGE CLEAR!");
        drawText(hdc, 280, 230, L"PRESS R TO RESTART");
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_TIMER:
        updateGame();
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_KEYDOWN:
        if (gameState == STATE_START && wParam == VK_RETURN) {
            gameState = STATE_PLAYING;
        }

        if (gameState == STATE_FINISH && wParam == 'R') {
            resetGame();
        }

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

        drawGame(memDC, rc.right, rc.bottom);

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
    wc.lpszClassName = L"PlatformerStage";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        L"PlatformerStage",
        L"2D Platformer - Stage System",
        WS_OVERLAPPEDWINDOW,
        100, 100, 800, 600,
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
