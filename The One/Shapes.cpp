#include <windows.h>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// ==================== 3D MATH STRUCTURES ====================
struct Vec3 {
    double x, y, z;

    Vec3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

    Vec3 rotateX(double angle) const {
        double c = cos(angle);
        double s = sin(angle);
        return Vec3(x, y * c - z * s, y * s + z * c);
    }

    Vec3 rotateY(double angle) const {
        double c = cos(angle);
        double s = sin(angle);
        return Vec3(x * c + z * s, y, -x * s + z * c);
    }

    Vec3 rotateZ(double angle) const {
        double c = cos(angle);
        double s = sin(angle);
        return Vec3(x * c - y * s, x * s + y * c, z);
    }

    Vec3 operator+(const Vec3& v) const {
        return Vec3(x + v.x, y + v.y, z + v.z);
    }

    Vec3 operator*(double s) const {
        return Vec3(x * s, y * s, z * s);
    }
};

struct Shape {
    std::string name;
    std::vector<Vec3> vertices;
    std::vector<std::pair<int, int>> edges;
    COLORREF color;

    Shape(std::string name, COLORREF color) : name(name), color(color) {}
};

// ==================== GLOBAL VARIABLES ====================
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Rotation variables
double angleX = 0.0;
double angleY = 0.0;
double angleZ = 0.0;

// Camera
double cameraDistance = 4.0;
Vec3 cameraPosition = Vec3(0, 0, 5);

// Shapes
std::vector<Shape> shapes;
int currentShape = 0;

// Animation
bool autoRotate = true;
double rotationSpeed = 0.02;

// UI
bool showHelp = true;
HFONT hFont;

// ==================== SHAPE CREATION ====================
void CreateShapes() {
    // Cube
    Shape cube("Cube", RGB(255, 0, 0));
    cube.vertices = {
        Vec3(-1, -1, -1), Vec3(1, -1, -1), Vec3(1, 1, -1), Vec3(-1, 1, -1),
        Vec3(-1, -1, 1), Vec3(1, -1, 1), Vec3(1, 1, 1), Vec3(-1, 1, 1)
    };
    cube.edges = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };
    shapes.push_back(cube);

    // Tetrahedron
    Shape tetra("Tetrahedron", RGB(0, 255, 0));
    tetra.vertices = {
        Vec3(0, 1, 0),
        Vec3(-0.8, -0.6, 0.8),
        Vec3(0.8, -0.6, 0.8),
        Vec3(0, -0.6, -0.8)
    };
    tetra.edges = {
        {0,1}, {0,2}, {0,3},
        {1,2}, {2,3}, {3,1}
    };
    shapes.push_back(tetra);

    // Pyramid
    Shape pyramid("Pyramid", RGB(0, 0, 255));
    pyramid.vertices = {
        Vec3(0, 1, 0),
        Vec3(-1, -1, -1),
        Vec3(1, -1, -1),
        Vec3(1, -1, 1),
        Vec3(-1, -1, 1)
    };
    pyramid.edges = {
        {0,1}, {0,2}, {0,3}, {0,4},
        {1,2}, {2,3}, {3,4}, {4,1}
    };
    shapes.push_back(pyramid);

    // Octahedron
    Shape octa("Octahedron", RGB(255, 255, 0));
    octa.vertices = {
        Vec3(0, 1, 0),
        Vec3(0, -1, 0),
        Vec3(1, 0, 0),
        Vec3(-1, 0, 0),
        Vec3(0, 0, 1),
        Vec3(0, 0, -1)
    };
    octa.edges = {
        {0,2}, {0,3}, {0,4}, {0,5},
        {1,2}, {1,3}, {1,4}, {1,5},
        {2,4}, {4,3}, {3,5}, {5,2}
    };
    shapes.push_back(octa);
}

// ==================== 3D TO 2D PROJECTION ====================
POINT Project3DTo2D(const Vec3& point) {
    // Simple perspective projection
    double scale = 300.0;
    double z = point.z + cameraDistance;

    if (z > 0.1) {
        double factor = scale / z;
        return {
            (LONG)(WINDOW_WIDTH / 2 + point.x * factor),
            (LONG)(WINDOW_HEIGHT / 2 - point.y * factor)  // Flip Y for screen coordinates
        };
    }

    return { 0, 0 };
}

// ==================== DRAWING FUNCTIONS ====================
void DrawShape(HDC hdc, const Shape& shape) {
    // Transform vertices
    std::vector<Vec3> transformed;
    for (const auto& v : shape.vertices) {
        Vec3 rotated = v;
        rotated = rotated.rotateX(angleX);
        rotated = rotated.rotateY(angleY);
        rotated = rotated.rotateZ(angleZ);
        transformed.push_back(rotated);
    }

    // Project to 2D
    std::vector<POINT> points2D;
    for (const auto& v : transformed) {
        points2D.push_back(Project3DTo2D(v));
    }

    // Create pen for edges
    HPEN hPen = CreatePen(PS_SOLID, 2, shape.color);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    // Draw edges
    for (const auto& edge : shape.edges) {
        if (edge.first < points2D.size() && edge.second < points2D.size()) {
            MoveToEx(hdc, points2D[edge.first].x, points2D[edge.first].y, NULL);
            LineTo(hdc, points2D[edge.second].x, points2D[edge.second].y);
        }
    }

    // Draw vertices
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    for (const auto& p : points2D) {
        Ellipse(hdc, p.x - 4, p.y - 4, p.x + 4, p.y + 4);
    }

    // Cleanup
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void DrawTextInfo(HDC hdc) {
    // Save current font
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    // Set text color
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 0, 0));

    // Draw shape info
    std::stringstream ss;
    ss << "Shape: " << shapes[currentShape].name;
    ss << "  |  X: " << (int)(angleX * 180 / 3.14159) << "°";
    ss << "  Y: " << (int)(angleY * 180 / 3.14159) << "°";
    ss << "  Z: " << (int)(angleZ * 180 / 3.14159) << "°";
    ss << "  |  Auto: " << (autoRotate ? "ON" : "OFF");

    TextOutA(hdc, 10, 10, ss.str().c_str(), ss.str().length());

    // Draw help if enabled
    if (showHelp) {
        std::vector<std::string> controls = {
            "=== CONTROLS ===",
            "A/D: Rotate Y-axis",
            "W/S: Rotate X-axis",
            "Q/E: Rotate Z-axis",
            "N/M: Next/Prev Shape",
            "R: Reset Rotation",
            "T: Toggle Auto-Rotate",
            "+/-: Zoom In/Out",
            "H: Toggle Help",
            "ESC: Exit"
        };

        int y = 40;
        for (const auto& line : controls) {
            TextOutA(hdc, 10, y, line.c_str(), line.length());
            y += 20;
        }
    }

    // Restore font
    SelectObject(hdc, hOldFont);
}

// ==================== WINDOW PROCEDURE ====================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        // Create font for text - FIXED: Use wide string
        hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        CreateShapes();

        // Set timer for animation (60 FPS)
        SetTimer(hwnd, 1, 16, NULL);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Fill background
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH hBrush = CreateSolidBrush(RGB(20, 20, 40));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        // Draw current shape
        if (!shapes.empty()) {
            DrawShape(hdc, shapes[currentShape]);
        }

        // Draw text info
        DrawTextInfo(hdc);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        // Update rotation
        if (autoRotate) {
            angleY += rotationSpeed;
            angleX += rotationSpeed * 0.5;
        }

        // Redraw window
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        return 0;
    }

    case WM_KEYDOWN: {
        switch (wParam) {
            // Rotation controls
        case 'A': angleY -= 0.1; break;
        case 'D': angleY += 0.1; break;
        case 'W': angleX += 0.1; break;
        case 'S': angleX -= 0.1; break;
        case 'Q': angleZ += 0.1; break;
        case 'E': angleZ -= 0.1; break;

            // Shape selection
        case 'N':
            currentShape = (currentShape + 1) % shapes.size();
            break;
        case 'M':
            currentShape = (currentShape - 1 + shapes.size()) % shapes.size();
            break;

            // Other controls
        case 'R':  // Reset
            angleX = angleY = angleZ = 0.0;
            cameraDistance = 4.0;
            break;
        case 'T':  // Toggle auto-rotate
            autoRotate = !autoRotate;
            break;
        case VK_ADD:   // Zoom in
        case VK_OEM_PLUS:
            cameraDistance = max(1.0, cameraDistance - 0.5);
            break;
        case VK_SUBTRACT:  // Zoom out
        case VK_OEM_MINUS:
            cameraDistance = min(10.0, cameraDistance + 0.5);
            break;
        case 'H':  // Toggle help
            showHelp = !showHelp;
            break;
        case VK_ESCAPE:  // Exit
            PostQuitMessage(0);
            break;
        }

        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_DESTROY: {
        KillTimer(hwnd, 1);
        DeleteObject(hFont);
        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// ==================== MAIN ENTRY POINT ====================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class - FIXED: Use wide strings
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"3DShapeRotator";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    // Create window - FIXED: Use wide strings
    HWND hwnd = CreateWindowEx(
        0,
        L"3DShapeRotator",
        L"3D Shape Rotator - GDI Version",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Show window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}