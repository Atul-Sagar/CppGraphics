#include <windows.h>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <chrono>
#include <algorithm>

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

struct Face {
    std::vector<int> vertexIndices;
    Vec3 normal;
    COLORREF color;

    Face(const std::vector<int>& indices, COLORREF col) : vertexIndices(indices), color(col) {}
};

struct Shape {
    std::string name;
    bool is3D;
    std::vector<Vec3> vertices;
    std::vector<std::pair<int, int>> edges;
    std::vector<Face> faces;
    COLORREF wireColor;
    COLORREF fillColor;

    Shape(std::string name, bool is3D, COLORREF wireColor, COLORREF fillColor)
        : name(name), is3D(is3D), wireColor(wireColor), fillColor(fillColor) {}
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

// Shapes
std::vector<Shape> shapes;
int currentShape = 0;

// Animation
bool autoRotate = true;
double rotationSpeed = 0.02;

// UI
bool showHelp = true;
bool wireframeMode = false;
bool showFilled = true;
HFONT hFont;

// FPS Counter
double fps = 0.0;
int frameCount = 0;
auto lastFpsUpdate = std::chrono::high_resolution_clock::now();
auto lastFrameTime = std::chrono::high_resolution_clock::now();

// Double Buffering
HDC hdcBuffer = NULL;
HBITMAP hBitmap = NULL;
HBITMAP hOldBitmap = NULL;
RECT clientRect;

// ==================== HELPER FUNCTIONS ====================
Vec3 CalculateNormal(const Vec3& v1, const Vec3& v2, const Vec3& v3) {
    Vec3 u = Vec3(v2.x - v1.x, v2.y - v1.y, v2.z - v1.z);
    Vec3 v = Vec3(v3.x - v1.x, v3.y - v1.y, v3.z - v1.z);

    Vec3 normal;
    normal.x = u.y * v.z - u.z * v.y;
    normal.y = u.z * v.x - u.x * v.z;
    normal.z = u.x * v.y - u.y * v.x;

    double length = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (length > 0) {
        normal.x /= length;
        normal.y /= length;
        normal.z /= length;
    }

    return normal;
}

// ==================== SHAPE CREATION ====================
void CreateShapes() {
    // ============ 3D SHAPES ============

    // 1. Cube
    Shape cube("Cube (3D)", true, RGB(255, 0, 0), RGB(255, 50, 50));
    cube.vertices = {
        Vec3(-1, -1, -1), Vec3(1, -1, -1), Vec3(1, 1, -1), Vec3(-1, 1, -1),
        Vec3(-1, -1, 1), Vec3(1, -1, 1), Vec3(1, 1, 1), Vec3(-1, 1, 1)
    };
    cube.edges = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };
    cube.faces = {
        Face({0,1,2,3}, RGB(255, 100, 100)),
        Face({4,5,6,7}, RGB(255, 150, 150)),
        Face({0,4,7,3}, RGB(200, 100, 100)),
        Face({1,5,6,2}, RGB(200, 150, 150)),
        Face({3,2,6,7}, RGB(150, 100, 100)),
        Face({0,1,5,4}, RGB(150, 150, 150))
    };
    shapes.push_back(cube);

    // 2. Tetrahedron
    Shape tetra("Tetrahedron (3D)", true, RGB(0, 255, 0), RGB(100, 255, 100));
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
    tetra.faces = {
        Face({0,1,2}, RGB(100, 255, 150)),
        Face({0,2,3}, RGB(100, 255, 100)),
        Face({0,3,1}, RGB(150, 255, 100)),
        Face({1,3,2}, RGB(100, 200, 100))
    };
    shapes.push_back(tetra);

    // 3. Pyramid
    Shape pyramid("Pyramid (3D)", true, RGB(0, 0, 255), RGB(100, 100, 255));
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
    pyramid.faces = {
        Face({0,1,2}, RGB(100, 100, 255)),
        Face({0,2,3}, RGB(120, 120, 255)),
        Face({0,3,4}, RGB(140, 140, 255)),
        Face({0,4,1}, RGB(160, 160, 255)),
        Face({1,2,3,4}, RGB(180, 180, 255))
    };
    shapes.push_back(pyramid);

    // 4. Octahedron
    Shape octa("Octahedron (3D)", true, RGB(255, 255, 0), RGB(255, 255, 150));
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
    octa.faces = {
        Face({0,2,4}, RGB(255, 255, 100)),
        Face({0,4,3}, RGB(255, 255, 120)),
        Face({0,3,5}, RGB(255, 255, 140)),
        Face({0,5,2}, RGB(255, 255, 160)),
        Face({1,4,2}, RGB(255, 255, 180)),
        Face({1,3,4}, RGB(255, 255, 200)),
        Face({1,5,3}, RGB(255, 255, 220)),
        Face({1,2,5}, RGB(255, 255, 240))
    };
    shapes.push_back(octa);

    // ============ 2D SHAPES ============

    // 5. Square (2D)
    Shape square("Square (2D)", false, RGB(255, 128, 0), RGB(255, 200, 100));
    square.vertices = {
        Vec3(-1, -1, 0), Vec3(1, -1, 0),
        Vec3(1, 1, 0), Vec3(-1, 1, 0)
    };
    square.edges = {
        {0,1}, {1,2}, {2,3}, {3,0}
    };
    square.faces = {
        Face({0,1,2,3}, RGB(255, 200, 150))
    };
    shapes.push_back(square);

    // 6. Triangle (2D)
    Shape triangle("Triangle (2D)", false, RGB(128, 255, 0), RGB(200, 255, 150));
    triangle.vertices = {
        Vec3(0, 1, 0),
        Vec3(-1, -1, 0),
        Vec3(1, -1, 0)
    };
    triangle.edges = {
        {0,1}, {1,2}, {2,0}
    };
    triangle.faces = {
        Face({0,1,2}, RGB(200, 255, 180))
    };
    shapes.push_back(triangle);

    // 7. Hexagon (2D)
    Shape hexagon("Hexagon (2D)", false, RGB(0, 128, 255), RGB(150, 200, 255));
    for (int i = 0; i < 6; i++) {
        double angle = 2 * 3.14159 * i / 6;
        hexagon.vertices.push_back(Vec3(cos(angle), sin(angle), 0));
    }
    for (int i = 0; i < 6; i++) {
        hexagon.edges.push_back({ i, (i + 1) % 6 });
    }
    hexagon.faces = {
        Face({0,1,2,3,4,5}, RGB(180, 220, 255))
    };
    shapes.push_back(hexagon);

    // 8. Circle (approximated as 16-gon) (2D)
    Shape circle("Circle (2D)", false, RGB(255, 0, 255), RGB(255, 150, 255));
    int circleSegments = 16;
    for (int i = 0; i < circleSegments; i++) {
        double angle = 2 * 3.14159 * i / circleSegments;
        circle.vertices.push_back(Vec3(cos(angle), sin(angle), 0));
    }
    for (int i = 0; i < circleSegments; i++) {
        circle.edges.push_back({ i, (i + 1) % circleSegments });
    }
    std::vector<int> allIndices;
    for (int i = 0; i < circleSegments; i++) {
        allIndices.push_back(i);
    }
    circle.faces = {
        Face(allIndices, RGB(255, 180, 255))
    };
    shapes.push_back(circle);

    // 9. Star (2D)
    Shape star("Star (2D)", false, RGB(255, 255, 0), RGB(255, 255, 180));
    int starPoints = 5;
    double outerRadius = 1.0;
    double innerRadius = 0.4;
    for (int i = 0; i < starPoints * 2; i++) {
        double angle = 3.14159 * i / starPoints;
        double radius = (i % 2 == 0) ? outerRadius : innerRadius;
        star.vertices.push_back(Vec3(radius * cos(angle), radius * sin(angle), 0));
    }
    for (int i = 0; i < starPoints * 2; i++) {
        star.edges.push_back({ i, (i + 1) % (starPoints * 2) });
    }
    std::vector<int> starIndices;
    for (int i = 0; i < starPoints * 2; i++) {
        starIndices.push_back(i);
    }
    star.faces = {
        Face(starIndices, RGB(255, 255, 200))
    };
    shapes.push_back(star);

    // 10. Cylinder (3D)
    Shape cylinder("Cylinder (3D)", true, RGB(0, 255, 255), RGB(150, 255, 255));
    int cylinderSides = 16;
    double radius = 0.8;
    double height = 1.6;

    for (int i = 0; i < cylinderSides; i++) {
        double angle = 2 * 3.14159 * i / cylinderSides;
        cylinder.vertices.push_back(Vec3(radius * cos(angle), height / 2, radius * sin(angle)));
        cylinder.vertices.push_back(Vec3(radius * cos(angle), -height / 2, radius * sin(angle)));
    }

    for (int i = 0; i < cylinderSides; i++) {
        int topIdx = i * 2;
        int bottomIdx = i * 2 + 1;
        int nextTopIdx = ((i + 1) % cylinderSides) * 2;
        int nextBottomIdx = ((i + 1) % cylinderSides) * 2 + 1;

        cylinder.edges.push_back({ topIdx, bottomIdx });
        cylinder.edges.push_back({ topIdx, nextTopIdx });
        cylinder.edges.push_back({ bottomIdx, nextBottomIdx });
    }

    for (int i = 0; i < cylinderSides; i++) {
        int topIdx = i * 2;
        int bottomIdx = i * 2 + 1;
        int nextTopIdx = ((i + 1) % cylinderSides) * 2;
        int nextBottomIdx = ((i + 1) % cylinderSides) * 2 + 1;

        cylinder.faces.push_back(Face({ topIdx, nextTopIdx, nextBottomIdx, bottomIdx },
            RGB(100 + i * 10, 255, 255)));
    }

    // Add top and bottom faces
    std::vector<int> topFace, bottomFace;
    for (int i = 0; i < cylinderSides; i++) {
        topFace.push_back(i * 2);
        bottomFace.push_back(i * 2 + 1);
    }
    cylinder.faces.push_back(Face(topFace, RGB(80, 240, 255)));
    cylinder.faces.push_back(Face(bottomFace, RGB(120, 240, 255)));

    shapes.push_back(cylinder);
}

// ==================== 3D TO 2D PROJECTION ====================
POINT Project3DTo2D(const Vec3& point) {
    double scale = 300.0;
    double z = point.z + cameraDistance;

    if (z > 0.1) {
        double factor = scale / z;
        return {
            (LONG)(WINDOW_WIDTH / 2 + point.x * factor),
            (LONG)(WINDOW_HEIGHT / 2 - point.y * factor)
        };
    }

    return { 0, 0 };
}

// ==================== DOUBLE BUFFERING ====================
void InitDoubleBuffer(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    GetClientRect(hwnd, &clientRect);

    if (hdcBuffer) {
        SelectObject(hdcBuffer, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcBuffer);
    }

    hdcBuffer = CreateCompatibleDC(hdc);
    hBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    hOldBitmap = (HBITMAP)SelectObject(hdcBuffer, hBitmap);

    ReleaseDC(hwnd, hdc);
}

void CleanupDoubleBuffer() {
    if (hdcBuffer) {
        SelectObject(hdcBuffer, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcBuffer);
        hdcBuffer = NULL;
    }
}

// ==================== DRAWING FUNCTIONS ====================
void DrawShape(HDC hdc, const Shape& shape) {
    // Transform vertices
    std::vector<Vec3> transformed;
    for (const auto& v : shape.vertices) {
        Vec3 rotated = v;
        if (shape.is3D) {
            rotated = rotated.rotateX(angleX);
            rotated = rotated.rotateY(angleY);
            rotated = rotated.rotateZ(angleZ);
        }
        transformed.push_back(rotated);
    }

    // Project to 2D
    std::vector<POINT> points2D;
    for (const auto& v : transformed) {
        points2D.push_back(Project3DTo2D(v));
    }

    // Draw filled faces if enabled
    if (showFilled && !shape.faces.empty()) {
        for (const auto& face : shape.faces) {
            if (face.vertexIndices.size() >= 3) {
                std::vector<POINT> polyPoints;
                for (int idx : face.vertexIndices) {
                    if (idx < (int)points2D.size()) {
                        polyPoints.push_back(points2D[idx]);
                    }
                }

                if (polyPoints.size() >= 3) {
                    HBRUSH hBrush = CreateSolidBrush(face.color);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

                    Polygon(hdc, polyPoints.data(), (int)polyPoints.size());

                    SelectObject(hdc, hOldBrush);
                    DeleteObject(hBrush);
                }
            }
        }
    }

    // Draw wireframe edges (always show edges in wireframe mode)
    // Even if showFilled is true, we still want to see the wireframe overlay
    if (wireframeMode) {
        // Create pen for edges
        HPEN hPen = CreatePen(PS_SOLID, 2, shape.wireColor);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        // Draw all edges
        for (const auto& edge : shape.edges) {
            if (edge.first < (int)points2D.size() && edge.second < (int)points2D.size()) {
                MoveToEx(hdc, points2D[edge.first].x, points2D[edge.first].y, NULL);
                LineTo(hdc, points2D[edge.second].x, points2D[edge.second].y);
            }
        }

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }

    // Draw vertices (nodes) - ALWAYS show vertices in wireframe mode
    if (wireframeMode || !showFilled) {
        // Create brush for vertices
        HBRUSH hVertexBrush = CreateSolidBrush(RGB(255, 255, 255)); // White vertices
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hVertexBrush);

        // Create pen for vertex outline
        HPEN hVertexPen = CreatePen(PS_SOLID, 1, shape.wireColor); // Outline in wire color
        HPEN hOldPen = (HPEN)SelectObject(hdc, hVertexPen);

        // Draw all vertices as circles
        for (const auto& p : points2D) {
            // Draw a filled circle for each vertex
            Ellipse(hdc, p.x - 6, p.y - 6, p.x + 6, p.y + 6);
        }

        // Cleanup
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hVertexPen);
        DeleteObject(hVertexBrush);
    }
}

void DrawTextInfo(HDC hdc) {
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkColor(hdc, RGB(0, 0, 0));

    frameCount++;
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFpsUpdate).count();

    if (elapsed >= 1000) {
        fps = frameCount * 1000.0 / elapsed;
        frameCount = 0;
        lastFpsUpdate = now;
    }

    std::stringstream ss;
    ss << "Shape: " << shapes[currentShape].name;
    if (shapes[currentShape].is3D) {
        ss << "  |  X: " << (int)(angleX * 180 / 3.14159) << "°";
        ss << "  Y: " << (int)(angleY * 180 / 3.14159) << "°";
        ss << "  Z: " << (int)(angleZ * 180 / 3.14159) << "°";
    }
    ss << "  |  FPS: " << (int)fps;
    ss << "  |  Auto: " << (autoRotate ? "ON" : "OFF");
    ss << "  |  Display: ";
    if (wireframeMode && showFilled) {
        ss << "Wireframe + Fill";
    }
    else if (wireframeMode) {
        ss << "Wireframe Only";
    }
    else if (showFilled) {
        ss << "Fill Only";
    }
    else {
        ss << "Hidden";
    }

    TextOutA(hdc, 10, 10, ss.str().c_str(), ss.str().length());

    if (showHelp) {
        std::vector<std::string> controls = {
            "=== CONTROLS ===",
            "A/D: Rotate Y-axis (3D only)",
            "W/S: Rotate X-axis (3D only)",
            "Q/E: Rotate Z-axis (3D only)",
            "N/M: Next/Prev Shape",
            "R: Reset Rotation & Zoom",
            "T: Toggle Auto-Rotate",
            "F: Toggle Fill (faces)",
            "G: Toggle Wireframe (edges + vertices)",
            "+/-: Zoom In/Out",
            "H: Toggle Help",
            "ESC: Exit"
        };

        int y = 40;
        for (const auto& line : controls) {
            TextOutA(hdc, 10, y, line.c_str(), line.length());
            y += 20;
        }

        // Show current shape info
        y += 10;
        std::string shapeInfo = "Shape Info: " + std::to_string(shapes[currentShape].vertices.size()) +
            " vertices, " + std::to_string(shapes[currentShape].edges.size()) +
            " edges, " + std::to_string(shapes[currentShape].faces.size()) + " faces";
        TextOutA(hdc, 10, y, shapeInfo.c_str(), shapeInfo.length());
    }

    SelectObject(hdc, hOldFont);
}

// ==================== WINDOW PROCEDURE ====================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        CreateShapes();
        InitDoubleBuffer(hwnd);

        SetTimer(hwnd, 1, 16, NULL);
        return 0;
    }

    case WM_SIZE: {
        InitDoubleBuffer(hwnd);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (hdcBuffer) {
            GetClientRect(hwnd, &clientRect);

            HBRUSH hBrush = CreateSolidBrush(RGB(20, 20, 40));
            FillRect(hdcBuffer, &clientRect, hBrush);
            DeleteObject(hBrush);

            if (!shapes.empty()) {
                DrawShape(hdcBuffer, shapes[currentShape]);
            }

            DrawTextInfo(hdcBuffer);

            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom,
                hdcBuffer, 0, 0, SRCCOPY);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        auto now = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(now - lastFrameTime).count();
        lastFrameTime = now;

        if (autoRotate && shapes[currentShape].is3D) {
            angleY += rotationSpeed * deltaTime * 60.0;
            angleX += rotationSpeed * 0.5 * deltaTime * 60.0;
        }

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_KEYDOWN: {
        auto now = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(now - lastFrameTime).count();

        switch (wParam) {
        case 'A':
            if (shapes[currentShape].is3D) angleY -= 0.1 * deltaTime * 60.0;
            break;
        case 'D':
            if (shapes[currentShape].is3D) angleY += 0.1 * deltaTime * 60.0;
            break;
        case 'W':
            if (shapes[currentShape].is3D) angleX += 0.1 * deltaTime * 60.0;
            break;
        case 'S':
            if (shapes[currentShape].is3D) angleX -= 0.1 * deltaTime * 60.0;
            break;
        case 'Q':
            if (shapes[currentShape].is3D) angleZ += 0.1 * deltaTime * 60.0;
            break;
        case 'E':
            if (shapes[currentShape].is3D) angleZ -= 0.1 * deltaTime * 60.0;
            break;

        case 'N':
            currentShape = (currentShape + 1) % shapes.size();
            break;
        case 'M':
            currentShape = (currentShape - 1 + shapes.size()) % shapes.size();
            break;

        case 'R':
            angleX = angleY = angleZ = 0.0;
            cameraDistance = 4.0;
            break;
        case 'T':
            autoRotate = !autoRotate;
            break;
        case 'F':
            showFilled = !showFilled;
            break;
        case 'G':
            wireframeMode = !wireframeMode;
            break;
        case VK_ADD:
        case VK_OEM_PLUS:
            cameraDistance = max(1.0, cameraDistance - 0.5);
            break;
        case VK_SUBTRACT:
        case VK_OEM_MINUS:
            cameraDistance = min(10.0, cameraDistance + 0.5);
            break;
        case 'H':
            showHelp = !showHelp;
            break;
        case VK_ESCAPE:
            PostQuitMessage(0);
            break;
        }

        lastFrameTime = now;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_ERASEBKGND: {
        return 1;
    }

    case WM_DESTROY: {
        KillTimer(hwnd, 1);
        CleanupDoubleBuffer();
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
    lastFpsUpdate = std::chrono::high_resolution_clock::now();
    lastFrameTime = std::chrono::high_resolution_clock::now();

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ShapeViewer";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        L"ShapeViewer",
        L"2D/3D Shape Viewer - Wireframe (Edges + Vertices) & Fill Modes",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}