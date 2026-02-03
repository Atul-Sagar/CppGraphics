#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Pens & Brushes
        HPEN blackPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        HBRUSH blueBrush = CreateSolidBrush(RGB(0, 150, 255));
        HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        HBRUSH redBrush = CreateSolidBrush(RGB(220, 0, 0));
        HBRUSH yellowBrush = CreateSolidBrush(RGB(255, 215, 0));

        SelectObject(hdc, blackPen);

        int cx = 400, cy = 200; // head center

        // ================= HEAD =================
        SelectObject(hdc, blueBrush);
        Ellipse(hdc, cx - 150, cy - 150, cx + 150, cy + 150);

        SelectObject(hdc, whiteBrush);
        Ellipse(hdc, cx - 120, cy - 80, cx + 120, cy + 150);

        // Eyes
        Ellipse(hdc, cx - 60, cy - 100, cx - 10, cy - 30);
        Ellipse(hdc, cx + 10, cy - 100, cx + 60, cy - 30);

        Ellipse(hdc, cx - 35, cy - 60, cx - 25, cy - 50);
        Ellipse(hdc, cx + 25, cy - 60, cx + 35, cy - 50);

        // Nose
        SelectObject(hdc, redBrush);
        Ellipse(hdc, cx - 15, cy - 30, cx + 15, cy);

        MoveToEx(hdc, cx, cy, NULL);
        LineTo(hdc, cx, cy + 40);

        // Mouth
        Arc(hdc, cx - 80, cy, cx + 80, cy + 100,
            cx - 80, cy + 20, cx + 80, cy + 20);

        // Whiskers
        for (int i = -1; i <= 1; i++)
        {
            MoveToEx(hdc, cx - 100, cy + i * 20, NULL);
            LineTo(hdc, cx - 40, cy + i * 20);

            MoveToEx(hdc, cx + 40, cy + i * 20, NULL);
            LineTo(hdc, cx + 100, cy + i * 20);
        }

        // ================= BODY =================
        int bodyTop = cy + 150;

        SelectObject(hdc, blueBrush);
        Ellipse(hdc, cx - 140, bodyTop, cx + 140, bodyTop + 260);

        // Belly
        SelectObject(hdc, whiteBrush);
        Ellipse(hdc, cx - 100, bodyTop + 40, cx + 100, bodyTop + 230);

        // Pocket
        Arc(hdc, cx - 50, bodyTop + 110, cx + 50, bodyTop + 180,
            cx - 50, bodyTop + 140, cx + 50, bodyTop + 140);

        // Collar
        SelectObject(hdc, redBrush);
        Rectangle(hdc, cx - 100, bodyTop - 15, cx + 100, bodyTop + 10);

        // Bell
        SelectObject(hdc, yellowBrush);
        Ellipse(hdc, cx - 20, bodyTop + 10, cx + 20, bodyTop + 50);
        LineTo(hdc, cx - 20, bodyTop + 30);
        LineTo(hdc, cx + 20, bodyTop + 30);
        Ellipse(hdc, cx - 5, bodyTop + 35, cx + 5, bodyTop + 45);

        // ================= HANDS =================
        SelectObject(hdc, blueBrush);
        Ellipse(hdc, cx - 190, bodyTop + 40, cx - 130, bodyTop + 120);
        Ellipse(hdc, cx + 130, bodyTop + 40, cx + 190, bodyTop + 120);

        SelectObject(hdc, whiteBrush);
        Ellipse(hdc, cx - 210, bodyTop + 80, cx - 160, bodyTop + 130);
        Ellipse(hdc, cx + 160, bodyTop + 80, cx + 210, bodyTop + 130);

        // ================= FEET =================
        Ellipse(hdc, cx - 110, bodyTop + 240, cx - 10, bodyTop + 290);
        Ellipse(hdc, cx + 10, bodyTop + 240, cx + 110, bodyTop + 290);

        // Cleanup
        DeleteObject(blackPen);
        DeleteObject(blueBrush);
        DeleteObject(whiteBrush);
        DeleteObject(redBrush);
        DeleteObject(yellowBrush);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"DoraemonFull";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Doraemon Full Body 🐱",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 700,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
