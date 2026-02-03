#include <windows.h>
#include <cmath>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
POINT lastMouse;
bool dragging = false;

struct Vec3 {
    float x, y, z;
};

Vec3 cube[] = {
    {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
    {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
};

int edges[][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

Vec3 rotate(Vec3 p) {
    float x = p.x;
    float y = p.y * cos(rotX) - p.z * sin(rotX);
    float z = p.y * sin(rotX) + p.z * cos(rotX);

    float x2 = x * cos(rotY) + z * sin(rotY);
    float y2 = y;
    float z2 = -x * sin(rotY) + z * cos(rotY);

    float x3 = x2 * cos(rotZ) - y2 * sin(rotZ);
    float y3 = x2 * sin(rotZ) + y2 * cos(rotZ);
    float z3 = z2;

    return { x3, y3, z3 };
}

POINT project(Vec3 p, int w, int h) {
    float scale = 150;
    return {
        int(p.x * scale + w / 2),
        int(p.y * scale + h / 2)
    };
}

void drawCube(HDC hdc, int w, int h) {
    Vec3 r[8];
    POINT p[8];

    for (int i = 0; i < 8; i++) {
        r[i] = rotate(cube[i]);
        p[i] = project(r[i], w, h);
    }

    for (auto& e : edges) {
        MoveToEx(hdc, p[e[0]].x, p[e[0]].y, NULL);
        LineTo(hdc, p[e[1]].x, p[e[1]].y);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_KEYDOWN:
        if (wParam == VK_LEFT)  rotY -= 0.1f;
        if (wParam == VK_RIGHT) rotY += 0.1f;
        if (wParam == VK_UP)    rotX -= 0.1f;
        if (wParam == VK_DOWN)  rotX += 0.1f;
        if (wParam == 'Q')      rotZ -= 0.1f;
        if (wParam == 'E')      rotZ += 0.1f;
        if (wParam == 'R')      rotX = rotY = rotZ = 0;
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_LBUTTONDOWN:
        dragging = true;
        lastMouse = { LOWORD(lParam), HIWORD(lParam) };
        break;

    case WM_LBUTTONUP:
        dragging = false;
        break;

    case WM_MOUSEMOVE:
        if (dragging) {
            POINT cur = { LOWORD(lParam), HIWORD(lParam) };
            rotY += (cur.x - lastMouse.x) * 0.01f;
            rotX += (cur.y - lastMouse.y) * 0.01f;
            lastMouse = cur;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        // Create back buffer
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(
            hdc, rc.right, rc.bottom
        );
        SelectObject(memDC, memBmp);

        // Clear background
        HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(memDC, &rc, bg);
        DeleteObject(bg);

        // Draw
        SetBkMode(memDC, TRANSPARENT);
        drawCube(memDC, rc.right, rc.bottom);

        // Copy to screen
        BitBlt(
            hdc, 0, 0, rc.right, rc.bottom,
            memDC, 0, 0, SRCCOPY
        );

        // Cleanup
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_ERASEBKGND:
        return 1; // prevent flicker

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"3DWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        L"3DWindow",
        L"3D Shapes - Keyboard & Mouse Control",
        WS_OVERLAPPEDWINDOW,
        100, 100, 800, 600,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
