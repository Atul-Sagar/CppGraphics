#include <windows.h>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <algorithm>

// Add these lines to prevent Windows.h min/max macros from interfering
#undef min
#undef max

// Include WinUser.h for mouse macros (though Windows.h should include it)
#include <winuser.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// ==================== BLOCK TYPES ====================
// Changed to enum class to fix warning
enum class BlockType {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_WOOD,
    BLOCK_LEAVES,
    BLOCK_WATER,
    BLOCK_SAND,
    BLOCK_GLASS,
    BLOCK_BRICK,
    BLOCK_COUNT
};

// Block colors (RGB)
COLORREF BlockColors[static_cast<int>(BlockType::BLOCK_COUNT)] = {
    RGB(135, 206, 235),    // Air/Sky
    RGB(95, 189, 87),      // Grass
    RGB(139, 69, 19),      // Dirt
    RGB(128, 128, 128),    // Stone
    RGB(139, 90, 43),      // Wood
    RGB(60, 179, 113),     // Leaves
    RGB(30, 144, 255),     // Water
    RGB(238, 214, 175),    // Sand
    RGB(220, 220, 220),    // Glass
    RGB(178, 34, 34)       // Brick
};

const char* BlockNames[static_cast<int>(BlockType::BLOCK_COUNT)] = {
    "Air", "Grass", "Dirt", "Stone", "Wood",
    "Leaves", "Water", "Sand", "Glass", "Brick"
};

// ==================== WORLD SETTINGS ====================
const int WORLD_WIDTH = 16;
const int WORLD_DEPTH = 16;
const int WORLD_HEIGHT = 8;
const float BLOCK_SIZE = 1.0f;

// ==================== CAMERA ====================
struct Camera {
    float x, y, z;      // Position
    float yaw, pitch;   // Rotation
    float fov;          // Field of view

    Camera() : x(8.0f), y(12.0f), z(8.0f), yaw(45.0f), pitch(-30.0f), fov(60.0f) {}

    void moveForward(float amount) {
        x += sinf(yaw * 3.14159f / 180.0f) * amount;
        z += cosf(yaw * 3.14159f / 180.0f) * amount;
    }

    void moveRight(float amount) {
        x += cosf(yaw * 3.14159f / 180.0f) * amount;
        z -= sinf(yaw * 3.14159f / 180.0f) * amount;
    }

    void moveUp(float amount) {
        y += amount;
    }
};

// ==================== WORLD DATA ====================
BlockType world[WORLD_WIDTH][WORLD_HEIGHT][WORLD_DEPTH];
Camera camera;
bool wireframeMode = false;
bool fogEnabled = true;
int selectedBlock = static_cast<int>(BlockType::BLOCK_GRASS);
bool showGrid = true;
bool dayNightCycle = true;
float timeOfDay = 12.0f; // 0-24 hours

// ==================== MOUSE LOOK ====================
bool mouseLookEnabled = false;
POINT mouseCenter;
int mouseSensitivity = 2;

// ==================== FPS COUNTER ====================
class FPSCounter {
private:
    int frameCount;
    float timePassed;
    float fps;
    LARGE_INTEGER frequency;
    LARGE_INTEGER lastTime;

public:
    FPSCounter() : frameCount(0), timePassed(0.0f), fps(0.0f) {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&lastTime);
    }

    void Update() {
        frameCount++;

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / static_cast<float>(frequency.QuadPart);
        timePassed += deltaTime;

        if (timePassed >= 0.5f) { // Update FPS every 0.5 seconds
            fps = static_cast<float>(frameCount) / timePassed;
            frameCount = 0;
            timePassed = 0.0f;
        }

        lastTime = currentTime;
    }

    float GetFPS() const { return fps; }
};

FPSCounter fpsCounter;

// ==================== DOUBLE BUFFERING ====================
HDC hBufferDC = NULL;
HBITMAP hBufferBitmap = NULL;
int bufferWidth = 800;
int bufferHeight = 600;
HWND g_hwnd = NULL;

// ==================== 3D MATH ====================
struct Vec3 {
    float x, y, z;

    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vec3 rotateY(float angle) const {
        float rad = angle * 3.14159f / 180.0f;
        float c = cosf(rad);
        float s = sinf(rad);
        return Vec3(x * c - z * s, y, x * s + z * c);
    }

    Vec3 rotateX(float angle) const {
        float rad = angle * 3.14159f / 180.0f;
        float c = cosf(rad);
        float s = sinf(rad);
        return Vec3(x, y * c - z * s, y * s + z * c);
    }

    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
};

// ==================== FACE STRUCTURE ====================
struct Face {
    Vec3 corners[4];
    COLORREF color;
    float depth;
    bool isTop;

    // Initialize members to fix warnings
    Face() : color(0), depth(0.0f), isTop(false) {
        corners[0] = Vec3();
        corners[1] = Vec3();
        corners[2] = Vec3();
        corners[3] = Vec3();
    }

    bool operator<(const Face& other) const {
        return depth > other.depth; // Sort back to front
    }
};

// ==================== INITIALIZATION ====================
void GenerateWorld() {
    srand(static_cast<unsigned int>(time(NULL)));

    // Initialize all to air first
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            for (int z = 0; z < WORLD_DEPTH; z++) {
                world[x][y][z] = BlockType::BLOCK_AIR;
            }
        }
    }

    // Generate simple flat terrain
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int z = 0; z < WORLD_DEPTH; z++) {
            // Flat ground at y = 3
            int groundHeight = 3;

            // Bedrock at bottom
            world[x][0][z] = BlockType::BLOCK_STONE;

            // Dirt layer
            for (int y = 1; y < groundHeight; y++) {
                world[x][y][z] = BlockType::BLOCK_DIRT;
            }

            // Grass on top
            world[x][groundHeight][z] = BlockType::BLOCK_GRASS;

            // Add some trees
            if (rand() % 10 == 0 && x > 1 && x < WORLD_WIDTH - 2 && z > 1 && z < WORLD_DEPTH - 2) {
                // Tree trunk
                for (int y = groundHeight + 1; y <= groundHeight + 4 && y < WORLD_HEIGHT; y++) {
                    world[x][y][z] = BlockType::BLOCK_WOOD;
                }

                // Tree leaves (simple cube)
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dz = -1; dz <= 1; dz++) {
                        for (int dy = 0; dy <= 2; dy++) {
                            int lx = x + dx;
                            int lz = z + dz;
                            int ly = groundHeight + 4 + dy;

                            if (lx >= 0 && lx < WORLD_WIDTH &&
                                lz >= 0 && lz < WORLD_DEPTH &&
                                ly >= 0 && ly < WORLD_HEIGHT) {
                                // Don't replace trunk
                                if (!(dx == 0 && dz == 0 && dy == 0)) {
                                    world[lx][ly][lz] = BlockType::BLOCK_LEAVES;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Add a simple house
    int houseX = WORLD_WIDTH / 2;
    int houseZ = WORLD_DEPTH / 2;
    int groundY = 3;

    // House foundation (3x3)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (houseX + dx >= 0 && houseX + dx < WORLD_WIDTH &&
                houseZ + dz >= 0 && houseZ + dz < WORLD_DEPTH) {
                world[houseX + dx][groundY][houseZ + dz] = BlockType::BLOCK_BRICK;
            }
        }
    }

    // House walls
    for (int y = groundY + 1; y <= groundY + 2; y++) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                if (abs(dx) == 1 || abs(dz) == 1) { // Only walls (perimeter)
                    if (houseX + dx >= 0 && houseX + dx < WORLD_WIDTH &&
                        houseZ + dz >= 0 && houseZ + dz < WORLD_DEPTH) {
                        // Windows on middle row
                        if (y == groundY + 2 && (dx == 0 || dz == 0)) {
                            world[houseX + dx][y][houseZ + dz] = BlockType::BLOCK_GLASS;
                        }
                        else {
                            world[houseX + dx][y][houseZ + dz] = BlockType::BLOCK_BRICK;
                        }
                    }
                }
            }
        }
    }

    // Roof
    if (groundY + 3 < WORLD_HEIGHT) {
        world[houseX][groundY + 3][houseZ] = BlockType::BLOCK_WOOD;
    }
}

// ==================== BUFFER MANAGEMENT ====================
void CreateBuffer(int width, int height) {
    if (hBufferDC) {
        DeleteDC(hBufferDC);
        hBufferDC = NULL;
    }
    if (hBufferBitmap) {
        DeleteObject(hBufferBitmap);
        hBufferBitmap = NULL;
    }

    HDC hdc = GetDC(g_hwnd);
    hBufferDC = CreateCompatibleDC(hdc);
    hBufferBitmap = CreateCompatibleBitmap(hdc, width, height);
    SelectObject(hBufferDC, hBufferBitmap);
    ReleaseDC(g_hwnd, hdc);

    bufferWidth = width;
    bufferHeight = height;
}

void ClearBuffer(COLORREF color) {
    if (!hBufferDC) return;

    RECT rc = { 0, 0, bufferWidth, bufferHeight };
    HBRUSH hBrush = CreateSolidBrush(color);
    FillRect(hBufferDC, &rc, hBrush);
    DeleteObject(hBrush);
}

// ==================== 3D PROJECTION ====================
POINT Project3DToScreen(float wx, float wy, float wz) {
    // Simple projection - directly map world coordinates to screen
    // First, calculate relative position to camera
    float relX = wx - camera.x;
    float relY = wy - camera.y;
    float relZ = wz - camera.z;

    // Apply yaw rotation (horizontal look)
    float yawRad = camera.yaw * 3.14159f / 180.0f;
    float tempX = relX * cosf(yawRad) - relZ * sinf(yawRad);
    float tempZ = relX * sinf(yawRad) + relZ * cosf(yawRad);
    relX = tempX;
    relZ = tempZ;

    // Apply pitch rotation (vertical look)
    float pitchRad = camera.pitch * 3.14159f / 180.0f;
    float tempY = relY * cosf(pitchRad) - relZ * sinf(pitchRad);
    tempZ = relY * sinf(pitchRad) + relZ * cosf(pitchRad);
    relY = tempY;
    relZ = tempZ;

    // Simple perspective projection
    if (relZ > 0.1f) {
        float scale = 400.0f / (relZ + 5.0f); // Add some offset to avoid division by small numbers

        int screenX = bufferWidth / 2 + static_cast<int>(relX * scale);
        int screenY = bufferHeight / 2 - static_cast<int>(relY * scale);

        return { screenX, screenY };
    }

    return { -10000, -10000 }; // Behind camera or too close
}

// ==================== RENDER FUNCTIONS ====================
void DrawFace(HDC hdc, const Face& face) {
    POINT points[4];
    bool allVisible = true;

    for (int i = 0; i < 4; i++) {
        points[i] = Project3DToScreen(face.corners[i].x, face.corners[i].y, face.corners[i].z);
        if (points[i].x == -10000) {
            allVisible = false;
            break;
        }
    }

    if (!allVisible) return;

    if (wireframeMode) {
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Polygon(hdc, points, 4);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hPen);
    }
    else {
        // Calculate lighting
        int brightness = face.isTop ? 220 : 180;
        if (dayNightCycle) {
            float timeFactor = sinf(timeOfDay * 3.14159f / 12.0f);
            brightness += static_cast<int>(50.0f * timeFactor);
        }

        // Clamp brightness
        if (brightness < 50) brightness = 50;
        if (brightness > 255) brightness = 255;

        int r = GetRValue(face.color) * brightness / 255;
        int g = GetGValue(face.color) * brightness / 255;
        int b = GetBValue(face.color) * brightness / 255;

        COLORREF shadedColor = RGB(r, g, b);

        HBRUSH hBrush = CreateSolidBrush(shadedColor);
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(r / 2, g / 2, b / 2));

        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

        Polygon(hdc, points, 4);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        DeleteObject(hBrush);
        DeleteObject(hPen);
    }
}

void CollectFaces(std::vector<Face>& faces, int x, int y, int z, BlockType type) {
    if (type == BlockType::BLOCK_AIR) return;

    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);
    float fz = static_cast<float>(z);

    int typeIndex = static_cast<int>(type);
    COLORREF color = BlockColors[typeIndex];

    // Check which faces are visible (adjacent to air)
    bool topVisible = (y == WORLD_HEIGHT - 1) || world[x][y + 1][z] == BlockType::BLOCK_AIR;
    bool frontVisible = (z == WORLD_DEPTH - 1) || world[x][y][z + 1] == BlockType::BLOCK_AIR;
    bool rightVisible = (x == WORLD_WIDTH - 1) || world[x + 1][y][z] == BlockType::BLOCK_AIR;
    bool backVisible = (z == 0) || world[x][y][z - 1] == BlockType::BLOCK_AIR;
    bool leftVisible = (x == 0) || world[x - 1][y][z] == BlockType::BLOCK_AIR;
    bool bottomVisible = (y == 0) || world[x][y - 1][z] == BlockType::BLOCK_AIR;

    // Calculate depth for sorting (distance from camera to block center)
    float blockCenterX = fx + 0.5f;
    float blockCenterY = fy + 0.5f;
    float blockCenterZ = fz + 0.5f;
    float dx = blockCenterX - camera.x;
    float dy = blockCenterY - camera.y;
    float dz = blockCenterZ - camera.z;
    float depth = sqrtf(dx * dx + dy * dy + dz * dz);

    // Add visible faces to the list
    if (topVisible) {
        Face face;
        face.corners[0] = Vec3(fx, fy + 1.0f, fz);
        face.corners[1] = Vec3(fx + 1.0f, fy + 1.0f, fz);
        face.corners[2] = Vec3(fx + 1.0f, fy + 1.0f, fz + 1.0f);
        face.corners[3] = Vec3(fx, fy + 1.0f, fz + 1.0f);
        face.color = color;
        face.depth = depth;
        face.isTop = true;
        faces.push_back(face);
    }

    if (frontVisible) {
        Face face;
        face.corners[0] = Vec3(fx, fy, fz + 1.0f);
        face.corners[1] = Vec3(fx + 1.0f, fy, fz + 1.0f);
        face.corners[2] = Vec3(fx + 1.0f, fy + 1.0f, fz + 1.0f);
        face.corners[3] = Vec3(fx, fy + 1.0f, fz + 1.0f);
        face.color = color;
        face.depth = depth;
        face.isTop = false;
        faces.push_back(face);
    }

    if (rightVisible) {
        Face face;
        face.corners[0] = Vec3(fx + 1.0f, fy, fz);
        face.corners[1] = Vec3(fx + 1.0f, fy, fz + 1.0f);
        face.corners[2] = Vec3(fx + 1.0f, fy + 1.0f, fz + 1.0f);
        face.corners[3] = Vec3(fx + 1.0f, fy + 1.0f, fz);
        face.color = color;
        face.depth = depth;
        face.isTop = false;
        faces.push_back(face);
    }

    if (backVisible) {
        Face face;
        face.corners[0] = Vec3(fx, fy, fz);
        face.corners[1] = Vec3(fx, fy + 1.0f, fz);
        face.corners[2] = Vec3(fx + 1.0f, fy + 1.0f, fz);
        face.corners[3] = Vec3(fx + 1.0f, fy, fz);
        face.color = color;
        face.depth = depth;
        face.isTop = false;
        faces.push_back(face);
    }

    if (leftVisible) {
        Face face;
        face.corners[0] = Vec3(fx, fy, fz);
        face.corners[1] = Vec3(fx, fy, fz + 1.0f);
        face.corners[2] = Vec3(fx, fy + 1.0f, fz + 1.0f);
        face.corners[3] = Vec3(fx, fy + 1.0f, fz);
        face.color = color;
        face.depth = depth;
        face.isTop = false;
        faces.push_back(face);
    }
}

// ==================== RENDER FRAME ====================
void RenderFrame() {
    if (!hBufferDC) return;

    // Update FPS counter
    fpsCounter.Update();

    // Calculate sky color based on time
    COLORREF skyColor;
    if (timeOfDay >= 6 && timeOfDay <= 18) {
        float t = (timeOfDay - 6) / 12.0f;
        int r = 135 + static_cast<int>(120.0f * (1.0f - t));
        int g = 206 + static_cast<int>(49.0f * (1.0f - t));
        int b = 235 + static_cast<int>(20.0f * (1.0f - t));
        skyColor = RGB(r, g, b);
    }
    else {
        skyColor = RGB(10, 20, 40);
    }

    ClearBuffer(skyColor);

    // Collect all visible faces
    std::vector<Face> faces;

    // Render from back to front for basic occlusion
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int z = 0; z < WORLD_DEPTH; z++) {
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                CollectFaces(faces, x, y, z, world[x][y][z]);
            }
        }
    }

    // Sort faces by depth (back to front)
    std::sort(faces.begin(), faces.end());

    // Draw all faces
    for (const auto& face : faces) {
        DrawFace(hBufferDC, face);
    }
}

// ==================== UI RENDERING ====================
void DrawUI(HDC hdc) {
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);

    // Draw semi-transparent HUD background
    HBRUSH hHudBrush = CreateSolidBrush(RGB(0, 0, 0));
    RECT hudRect = { 0, bufferHeight - 120, bufferWidth, bufferHeight };
    FillRect(hdc, &hudRect, hHudBrush);
    DeleteObject(hHudBrush);

    SetTextColor(hdc, RGB(255, 255, 255));

    // Draw FPS counter
    char fpsBuffer[64];
    sprintf_s(fpsBuffer, "FPS: %.1f", fpsCounter.GetFPS());
    TextOutA(hdc, bufferWidth - 150, 20, fpsBuffer, static_cast<int>(strlen(fpsBuffer)));

    // Draw mouse look status
    const char* mouseStatus = mouseLookEnabled ? "Mouse Look: ON (Right Click)" : "Mouse Look: OFF";
    TextOutA(hdc, bufferWidth - 150, 40, mouseStatus, static_cast<int>(strlen(mouseStatus)));

    // Draw selected block preview
    int blockSize = 40;
    int previewX = 20;
    int previewY = bufferHeight - 100;

    RECT blockRect = { previewX, previewY, previewX + blockSize, previewY + blockSize };
    HBRUSH hBlockBrush = CreateSolidBrush(BlockColors[selectedBlock]);
    FillRect(hdc, &blockRect, hBlockBrush);
    DeleteObject(hBlockBrush);

    // Draw block border
    HPEN hBorderPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, blockRect.left, blockRect.top, blockRect.right, blockRect.bottom);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBorderPen);

    // Draw info text
    char buffer[256];

    sprintf_s(buffer, "Block: %s", BlockNames[selectedBlock]);
    TextOutA(hdc, previewX + blockSize + 10, previewY, buffer, static_cast<int>(strlen(buffer)));

    sprintf_s(buffer, "Position: X=%.1f Y=%.1f Z=%.1f", camera.x, camera.y, camera.z);
    TextOutA(hdc, previewX + blockSize + 10, previewY + 20, buffer, static_cast<int>(strlen(buffer)));

    sprintf_s(buffer, "Look: Yaw=%.1f Pitch=%.1f", camera.yaw, camera.pitch);
    TextOutA(hdc, previewX + blockSize + 10, previewY + 40, buffer, static_cast<int>(strlen(buffer)));

    // Draw time
    sprintf_s(buffer, "Time: %02d:00", static_cast<int>(timeOfDay) % 24);
    TextOutA(hdc, bufferWidth - 100, 60, buffer, static_cast<int>(strlen(buffer)));

    // Draw controls
    const char* controls[] = {
        "CONTROLS:",
        "WASD - Move, QE - Up/Down",
        "Arrow Keys - Look around",
        "Right Click - Toggle Mouse Look",
        "1-9 - Select Block",
        "G - Toggle Grid, F - Toggle Fog",
        "R - Wireframe, T - Day/Night",
        "SPACE - Place, SHIFT - Destroy",
        "ESC - Exit"
    };

    for (int i = 0; i < 9; i++) {
        TextOutA(hdc, 20, 20 + i * 20, controls[i], static_cast<int>(strlen(controls[i])));
    }

    // Restore
    SetBkMode(hdc, oldBkMode);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

// ==================== MOUSE LOOK FUNCTIONS ====================
void EnableMouseLook(HWND hwnd) {
    mouseLookEnabled = true;

    // Hide cursor
    ShowCursor(FALSE);

    // Get window center
    RECT rc;
    GetClientRect(hwnd, &rc);
    mouseCenter.x = rc.left + (rc.right - rc.left) / 2;
    mouseCenter.y = rc.top + (rc.bottom - rc.top) / 2;

    // Set cursor to center
    ClientToScreen(hwnd, &mouseCenter);
    SetCursorPos(mouseCenter.x, mouseCenter.y);

    // Capture mouse
    SetCapture(hwnd);
}

void DisableMouseLook(HWND hwnd) {
    mouseLookEnabled = false;

    // Show cursor
    ShowCursor(TRUE);

    // Release mouse capture
    ReleaseCapture();
}

void UpdateMouseLook(HWND hwnd, int mouseX, int mouseY) {
    if (!mouseLookEnabled) return;

    // Get window center
    RECT rc;
    GetClientRect(hwnd, &rc);
    int centerX = rc.left + (rc.right - rc.left) / 2;
    int centerY = rc.top + (rc.bottom - rc.top) / 2;

    // Calculate mouse movement
    int deltaX = mouseX - centerX;
    int deltaY = mouseY - centerY;

    // Update camera rotation
    camera.yaw += deltaX * 0.1f * mouseSensitivity;
    camera.pitch -= deltaY * 0.1f * mouseSensitivity;

    // Clamp pitch
    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;

    // Wrap yaw
    if (camera.yaw > 360.0f) camera.yaw -= 360.0f;
    if (camera.yaw < 0.0f) camera.yaw += 360.0f;

    // Reset mouse to center
    POINT center;
    center.x = centerX;
    center.y = centerY;
    ClientToScreen(hwnd, &center);
    SetCursorPos(center.x, center.y);

    // Force redraw
    InvalidateRect(hwnd, NULL, FALSE);
}

// ==================== WINDOW PROCEDURE ====================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static bool mouseCaptured = false;
    static int lastMouseX = 0, lastMouseY = 0;

    switch (uMsg) {
    case WM_CREATE: {
        g_hwnd = hwnd;
        GenerateWorld();
        CreateBuffer(800, 600);
        SetTimer(hwnd, 1, 16, NULL); // ~60 FPS
        return 0;
    }

    case WM_SIZE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        if (width > 0 && height > 0) {
            CreateBuffer(width, height);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Render 3D world to buffer
        RenderFrame();

        // Draw UI on top
        DrawUI(hBufferDC);

        // Copy buffer to screen
        if (hBufferDC && hBufferBitmap) {
            BitBlt(hdc, 0, 0, bufferWidth, bufferHeight, hBufferDC, 0, 0, SRCCOPY);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_TIMER: {
        if (dayNightCycle) {
            timeOfDay += 0.016f;
            if (timeOfDay >= 24.0f) timeOfDay -= 24.0f;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_KEYDOWN: {
        float moveSpeed = 0.5f;

        switch (wParam) {
        case 'W': camera.moveForward(moveSpeed); break;
        case 'S': camera.moveForward(-moveSpeed); break;
        case 'A': camera.moveRight(-moveSpeed); break;
        case 'D': camera.moveRight(moveSpeed); break;
        case 'Q': camera.moveUp(moveSpeed); break;
        case 'E': camera.moveUp(-moveSpeed); break;

        case VK_LEFT: camera.yaw -= 5.0f; break;
        case VK_RIGHT: camera.yaw += 5.0f; break;
        case VK_UP: camera.pitch += 5.0f; break;
        case VK_DOWN: camera.pitch -= 5.0f; break;

        case '1': selectedBlock = static_cast<int>(BlockType::BLOCK_GRASS); break;
        case '2': selectedBlock = static_cast<int>(BlockType::BLOCK_DIRT); break;
        case '3': selectedBlock = static_cast<int>(BlockType::BLOCK_STONE); break;
        case '4': selectedBlock = static_cast<int>(BlockType::BLOCK_WOOD); break;
        case '5': selectedBlock = static_cast<int>(BlockType::BLOCK_LEAVES); break;
        case '6': selectedBlock = static_cast<int>(BlockType::BLOCK_WATER); break;
        case '7': selectedBlock = static_cast<int>(BlockType::BLOCK_SAND); break;
        case '8': selectedBlock = static_cast<int>(BlockType::BLOCK_GLASS); break;
        case '9': selectedBlock = static_cast<int>(BlockType::BLOCK_BRICK); break;

        case 'G': showGrid = !showGrid; break;
        case 'F': fogEnabled = !fogEnabled; break;
        case 'R': wireframeMode = !wireframeMode; break;
        case 'T': dayNightCycle = !dayNightCycle; break;

        case VK_SPACE: {
            int placeX = static_cast<int>(camera.x);
            int placeY = static_cast<int>(camera.y);
            int placeZ = static_cast<int>(camera.z);

            if (placeX >= 0 && placeX < WORLD_WIDTH &&
                placeY >= 0 && placeY < WORLD_HEIGHT &&
                placeZ >= 0 && placeZ < WORLD_DEPTH) {
                world[placeX][placeY][placeZ] = static_cast<BlockType>(selectedBlock);
            }
            break;
        }

        case VK_SHIFT: {
            int destroyX = static_cast<int>(camera.x);
            int destroyY = static_cast<int>(camera.y);
            int destroyZ = static_cast<int>(camera.z);

            if (destroyX >= 0 && destroyX < WORLD_WIDTH &&
                destroyY >= 0 && destroyY < WORLD_HEIGHT &&
                destroyZ >= 0 && destroyZ < WORLD_DEPTH) {
                world[destroyX][destroyY][destroyZ] = BlockType::BLOCK_AIR;
            }
            break;
        }

        case VK_ESCAPE:
            PostQuitMessage(0);
            break;
        }

        // Clamp pitch
        if (camera.pitch > 89.0f) camera.pitch = 89.0f;
        if (camera.pitch < -89.0f) camera.pitch = -89.0f;

        // Wrap yaw
        if (camera.yaw > 360.0f) camera.yaw -= 360.0f;
        if (camera.yaw < 0.0f) camera.yaw += 360.0f;

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_RBUTTONDOWN: {
        // Toggle mouse look on right click
        if (!mouseLookEnabled) {
            EnableMouseLook(hwnd);
        }
        else {
            DisableMouseLook(hwnd);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        // FIXED: Use LOWORD and HIWORD macros instead of GET_X_LPARAM/GET_Y_LPARAM
        int x = LOWORD(lParam);  // Get X coordinate
        int y = HIWORD(lParam);  // Get Y coordinate

        if (mouseLookEnabled) {
            UpdateMouseLook(hwnd, x, y);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        // Use mouse wheel to change selected block
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0) {
            // Scroll up - next block
            selectedBlock = (selectedBlock + 1) % static_cast<int>(BlockType::BLOCK_COUNT);
        }
        else {
            // Scroll down - previous block
            selectedBlock = (selectedBlock - 1 + static_cast<int>(BlockType::BLOCK_COUNT)) % static_cast<int>(BlockType::BLOCK_COUNT);
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_KILLFOCUS: {
        // Disable mouse look if window loses focus
        if (mouseLookEnabled) {
            DisableMouseLook(hwnd);
        }
        return 0;
    }

    case WM_DESTROY: {
        KillTimer(hwnd, 1);
        if (hBufferDC) DeleteDC(hBufferDC);
        if (hBufferBitmap) DeleteObject(hBufferBitmap);
        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// ==================== MAIN ENTRY POINT ====================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MinecraftRenderer";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc)) {
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0,
        L"MinecraftRenderer",
        L"Minecraft-Style Block World - GDI Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}