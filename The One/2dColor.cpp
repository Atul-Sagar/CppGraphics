// ==================== INCLUDES ====================
#include <windows.h>      // Windows API for window creation, drawing, and input
#include <sstream>        // String streams for text display
#include <vector>         // Dynamic arrays for game objects
#include <string>         // String handling
#include <random>         // Random number generation for particles
#include <ctime>          // Time functions for random seed

// ==================== LIBRARY LINKS ====================
#pragma comment(lib, "user32.lib")  // Windows user interface functions
#pragma comment(lib, "gdi32.lib")   // Graphics drawing functions

// ==================== GAME CONSTANTS ====================
// Screen dimensions
const int SCREEN_W = 800;           // Window width in pixels
const int SCREEN_H = 600;           // Window height in pixels

// Player dimensions
const int PLAYER_W = 40;            // Player character width
const int PLAYER_H = 50;            // Player character height

// Level constants
const int GROUND_Y = 400;           // Y-coordinate where ground begins
const int LEVEL_END_X = 2200;       // X-coordinate where level ends

// Physics constants
const float GRAVITY = 0.8f;         // Downward acceleration per frame
const float MOVE_SPEED = 5.0f;      // Horizontal movement speed per frame
const float JUMP_FORCE = -15.0f;    // Upward velocity when jumping (negative = up)
const float MAX_FALL_SPEED = 20.0f; // Maximum downward velocity
const float CAMERA_SMOOTHNESS = 0.1f; // Camera follow smoothness (0-1, higher = smoother)

// ==================== GAME STATE ENUM ====================
// Different states the game can be in
enum GameState {
    STATE_START,     // Start screen showing title and controls
    STATE_PLAYING,   // Game is actively running
    STATE_FINISH,    // Player reached the end of the level
    STATE_DEAD,      // Player died
    STATE_PAUSED     // Game is paused
};

// ==================== DATA STRUCTURES ====================
// Represents a segment of ground/platform
struct Ground {
    int x1;          // Left X coordinate
    int x2;          // Right X coordinate
    bool hasSpike = false; // Whether this ground has spikes
};

// Represents an enemy that patrols back and forth
struct Enemy {
    float x;         // Current X position
    float y;         // Current Y position
    float speed;     // Movement speed in pixels per frame
    int dir;         // Direction: 1 = right, -1 = left
    int patrolStart; // Left boundary of patrol area
    int patrolEnd;   // Right boundary of patrol area
    bool isActive = true; // Whether enemy is active
};

// Represents a visual particle effect
struct Particle {
    float x, y;      // Position
    float vx, vy;    // Velocity (movement per frame)
    int lifetime;    // Frames remaining before disappearing
    COLORREF color;  // Color of the particle
};

// Represents a collectible item
struct Collectible {
    float x, y;      // Position
    bool collected;  // Whether player has collected it
    int type;        // 0 = coin, 1 = health, 2 = powerup
};

// ==================== GLOBAL VARIABLES ====================
// Player variables
float playerX = 100;      // Player's X position
float playerY = 300;      // Player's Y position
float velY = 0;           // Player's vertical velocity (for jumping/falling)

// Camera variables
float cameraX = 0;        // Camera's current X position
float cameraTargetX = 0;   // Where camera wants to be (smooth following target)

// Input variables
bool leftKey = false;     // A key pressed
bool rightKey = false;    // D key pressed
bool onGround = false;    // Is player touching the ground?

// Game state variables
GameState gameState = STATE_START; // Current game state
int score = 0;            // Player's score
int lives = 3;            // Player's remaining lives
int coinsCollected = 0;   // Number of coins collected
float gameTime = 0.0f;    // Time since level started (in seconds)

// Player power-up variables
bool playerInvincible = false; // Is player temporarily invincible?
float invincibleTimer = 0.0f;  // Time remaining for invincibility

// ==================== LEVEL DATA ====================
// Ground segments that make up the level
std::vector<Ground> groundSegments = {
    {0, 500},                     // First platform
    {650, 1000, true},           // Second platform with spikes
    {1150, 1500},                // Third platform
    {1650, 2100}                 // Fourth platform
};

// Enemies in the level
std::vector<Enemy> enemies = {
    {350, 350, 2.0f, 1, 300, 500},   // Enemy 1
    {800, 350, 2.5f, -1, 700, 900},  // Enemy 2 (moves left initially)
    {1300, 350, 2.0f, 1, 1250, 1350}, // Enemy 3
    {1800, 350, 2.5f, -1, 1700, 1900} // Enemy 4
};

// Collectible items in the level
std::vector<Collectible> collectibles = {
    {200, 300, false, 0},   // Coin
    {400, 250, false, 0},   // Coin
    {750, 300, false, 1},   // Health
    {1200, 280, false, 0},  // Coin
    {1400, 250, false, 2},  // Powerup
    {1900, 300, false, 0}   // Coin
};

// Particle effects
std::vector<Particle> particles;

// ==================== RANDOM NUMBER GENERATOR ====================
// Used for particle effects
std::mt19937 rng(static_cast<unsigned>(time(nullptr))); // Random engine
std::uniform_real_distribution<float> dist(-2.0f, 2.0f); // Distribution for particle velocities

// ==================== GAME FUNCTIONS ====================

/**
 * Resets all game variables to their initial state
 * Called when starting a new game or restarting after death
 */
void resetGame() {
    // Reset player position and velocity
    playerX = 100;
    playerY = 300;
    velY = 0;

    // Reset camera
    cameraX = 0;
    cameraTargetX = 0;

    // Reset game stats
    score = 0;
    coinsCollected = 0;
    lives = 3;
    gameTime = 0.0f;
    playerInvincible = false;

    // Reset all collectibles to uncollected state
    for (auto& col : collectibles) {
        col.collected = false;
    }

    // Reactivate all enemies
    for (auto& enemy : enemies) {
        enemy.isActive = true;
    }

    // Clear all particle effects
    particles.clear();

    // If game was finished or dead, go back to start screen
    if (gameState == STATE_DEAD || gameState == STATE_FINISH) {
        gameState = STATE_START;
    }
}

/**
 * Creates particle effects at specified position
 * @param x, y - Where to create particles
 * @param count - How many particles to create
 * @param color - Color of the particles
 */
void addParticles(float x, float y, int count, COLORREF color) {
    for (int i = 0; i < count; i++) {
        // Create particle with random velocity and lifetime
        particles.push_back({
            x, y,                      // Position
            dist(rng),                 // Random horizontal velocity
            dist(rng) - 2.0f,          // Random vertical velocity (biased upward)
            30 + rand() % 30,          // Random lifetime (30-60 frames)
            color                      // Particle color
            });
    }
}

/**
 * Checks if player is standing on ground
 * @param x, y - Position to check
 * @return true if position is on ground, false otherwise
 */
bool isOnGround(float x, float y) {
    // Check against all ground segments
    for (const auto& ground : groundSegments) {
        // Check if player rectangle overlaps ground rectangle horizontally
        if (x + PLAYER_W > ground.x1 &&      // Player's right side > ground's left side
            x < ground.x2) {                 // Player's left side < ground's right side

            // Check if player's bottom is close to ground level
            if (y + PLAYER_H >= GROUND_Y - 1 &&    // Player bottom at or below ground
                y + PLAYER_H <= GROUND_Y + 10) {   // But not too far below ground
                return true;
            }
        }
    }
    return false;
}

/**
 * Checks if player is colliding with spikes
 * @param x, y - Position to check
 * @return true if colliding with spikes, false otherwise
 */
bool checkSpikeCollision(float x, float y) {
    for (const auto& ground : groundSegments) {
        // Only check ground segments that have spikes
        if (ground.hasSpike &&
            // Check horizontal overlap
            x + PLAYER_W > ground.x1 &&
            x < ground.x2 &&
            // Check vertical overlap with spike area
            y + PLAYER_H >= GROUND_Y - 20 &&  // Player bottom in spike zone
            y < GROUND_Y + 10) {              // Player top above ground
            return true;
        }
    }
    return false;
}

/**
 * Updates camera position to follow player smoothly
 * Camera tries to center on player but stays within level boundaries
 */
void updateCamera() {
    // Target position: center camera on player
    cameraTargetX = playerX - SCREEN_W / 2;

    // Don't show area before level start
    if (cameraTargetX < 0) cameraTargetX = 0;

    // Don't show area beyond level end
    if (cameraTargetX > LEVEL_END_X - SCREEN_W)
        cameraTargetX = LEVEL_END_X - SCREEN_W;

    // Smoothly move camera toward target position
    // Linear interpolation: current + (target - current) * smoothness
    cameraX += (cameraTargetX - cameraX) * CAMERA_SMOOTHNESS;
}

/**
 * Updates all particle effects
 * Moves particles, applies gravity, and removes dead particles
 */
void updateParticles() {
    // Iterate through all particles
    for (auto it = particles.begin(); it != particles.end();) {
        // Update position based on velocity
        it->x += it->vx;
        it->y += it->vy;

        // Apply gravity to vertical velocity
        it->vy += 0.1f;

        // Decrease lifetime
        it->lifetime--;

        // Remove particle if lifetime expired
        if (it->lifetime <= 0) {
            it = particles.erase(it);  // Remove from vector
        }
        else {
            ++it;  // Move to next particle
        }
    }
}

/**
 * Updates all collectible items
 * Checks for collision with player and applies effects
 */
void updateCollectibles() {
    for (auto& col : collectibles) {
        // Skip already collected items
        if (!col.collected) {
            // Create rectangles for collision detection
            RECT playerRect = {
                (LONG)playerX,
                (LONG)playerY,
                (LONG)(playerX + PLAYER_W),
                (LONG)(playerY + PLAYER_H)
            };

            RECT collectibleRect = {
                (LONG)col.x,
                (LONG)col.y,
                (LONG)(col.x + 20),
                (LONG)(col.y + 20)
            };

            RECT intersection;
            // Check if rectangles intersect
            if (IntersectRect(&intersection, &playerRect, &collectibleRect)) {
                // Mark as collected
                col.collected = true;
                score += 100;  // Base score
                coinsCollected++;

                COLORREF particleColor = RGB(255, 215, 0); // Gold for coins

                // Apply type-specific effects
                if (col.type == 1) {  // Health collectible
                    particleColor = RGB(0, 255, 0); // Green
                    lives = min(lives + 1, 5);  // Add life, max 5
                }
                else if (col.type == 2) {  // Powerup collectible
                    particleColor = RGB(255, 0, 255); // Purple
                    playerInvincible = true;
                    invincibleTimer = 5.0f;  // 5 seconds of invincibility
                }

                // Create particle effect
                addParticles(col.x + 10, col.y + 10, 15, particleColor);
            }
        }
    }
}

/**
 * Updates all enemies
 * Moves enemies, checks patrol boundaries, and detects collisions with player
 */
void updateEnemies() {
    for (auto& enemy : enemies) {
        // Skip inactive enemies
        if (!enemy.isActive) continue;

        // Move enemy based on direction and speed
        enemy.x += enemy.speed * enemy.dir;

        // Reverse direction if at patrol boundary
        if (enemy.x < enemy.patrolStart || enemy.x > enemy.patrolEnd) {
            enemy.dir *= -1;
        }

        // Keep enemy on ground
        enemy.y = GROUND_Y - PLAYER_H;

        // Check collision with player (if player is not invincible)
        if (!playerInvincible) {
            RECT playerRect = {
                (LONG)playerX,
                (LONG)playerY,
                (LONG)(playerX + PLAYER_W),
                (LONG)(playerY + PLAYER_H)
            };

            RECT enemyRect = {
                (LONG)enemy.x,
                (LONG)enemy.y,
                (LONG)(enemy.x + PLAYER_W),
                (LONG)(enemy.y + PLAYER_H)
            };

            RECT intersection;
            if (IntersectRect(&intersection, &playerRect, &enemyRect)) {
                // Player takes damage
                lives--;
                playerInvincible = true;
                invincibleTimer = 2.0f;  // 2 seconds of invincibility after hit

                // Check for game over
                if (lives <= 0) {
                    gameState = STATE_DEAD;
                }

                // Create blood particle effect
                addParticles(playerX + PLAYER_W / 2, playerY + PLAYER_H / 2, 20, RGB(255, 50, 50));
            }
        }
    }
}

/**
 * Main game update function - called every frame
 * Updates all game logic: input, physics, collisions, game state
 */
void updateGame() {
    // Only update if game is in playing state
    if (gameState != STATE_PLAYING) return;

    // Update game timer (assuming 60 FPS, each frame is ~0.016 seconds)
    gameTime += 0.016f;

    // Update invincibility timer
    if (playerInvincible) {
        invincibleTimer -= 0.016f;
        if (invincibleTimer <= 0) {
            playerInvincible = false;  // End invincibility
        }
    }

    // ===== INPUT HANDLING =====
    if (leftKey)  playerX -= MOVE_SPEED;  // Move left
    if (rightKey) playerX += MOVE_SPEED;  // Move right

    // ===== PHYSICS =====
    velY += GRAVITY;  // Apply gravity
    if (velY > MAX_FALL_SPEED) velY = MAX_FALL_SPEED;  // Limit fall speed
    playerY += velY;  // Update vertical position

    // ===== GROUND COLLISION =====
    if (isOnGround(playerX, playerY)) {
        // Snap player to ground level
        playerY = GROUND_Y - PLAYER_H;
        velY = 0;  // Stop falling
        onGround = true;  // Player can jump
    }
    else {
        onGround = false;  // Player is in air
    }

    // ===== SPIKE COLLISION =====
    if (checkSpikeCollision(playerX, playerY) && !playerInvincible) {
        lives--;  // Take damage
        playerInvincible = true;
        invincibleTimer = 2.0f;

        // Check for game over
        if (lives <= 0) {
            gameState = STATE_DEAD;
        }

        // Create spike hit particle effect
        addParticles(playerX + PLAYER_W / 2, playerY + PLAYER_H / 2, 25, RGB(255, 100, 0));
    }

    // ===== DEATH BOUNDARY =====
    // Player dies if they fall too far
    if (playerY > SCREEN_H + 200) {
        gameState = STATE_DEAD;
    }

    // ===== LEVEL COMPLETION =====
    if (playerX >= LEVEL_END_X) {
        gameState = STATE_FINISH;
        // Calculate final score: time bonus + coin bonus
        score += (int)(10000 / gameTime) + coinsCollected * 500;
    }

    // ===== UPDATE SUBSYSTEMS =====
    updateCamera();        // Move camera
    updateEnemies();       // Update enemy AI and collisions
    updateCollectibles();  // Check collectible collisions
    updateParticles();     // Update particle effects
}

// ==================== DRAWING FUNCTIONS ====================

/**
 * Draws ASCII art text at specified position
 * @param hdc - Drawing context
 * @param x, y - Top-left position
 * @param art - Vector of strings representing ASCII art lines
 * @param color - Text color
 */
void drawAscii(HDC hdc, int x, int y, const std::vector<std::wstring>& art, COLORREF color = RGB(0, 0, 0)) {
    int lineHeight = 18;  // Space between lines
    COLORREF oldColor = SetTextColor(hdc, color);  // Save old color

    // Draw each line of ASCII art
    for (size_t i = 0; i < art.size(); i++) {
        TextOutW(hdc, x, y + i * lineHeight, art[i].c_str(), art[i].length());
    }

    SetTextColor(hdc, oldColor);  // Restore original color
}

/**
 * Draws all particle effects
 * @param hdc - Drawing context
 */
void drawParticles(HDC hdc) {
    for (const auto& p : particles) {
        // Calculate alpha (transparency) based on lifetime
        int alpha = p.lifetime * 255 / 60;

        // Extract color components
        int r = GetRValue(p.color);
        int g = GetGValue(p.color);
        int b = GetBValue(p.color);

        // Create brush and pen for drawing particle
        HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(r, g, b));

        // Select drawing tools
        SelectObject(hdc, brush);
        SelectObject(hdc, pen);

        // Calculate particle size (shrinks as it ages)
        int size = 3 + p.lifetime / 20;

        // Draw particle as a circle
        Ellipse(hdc,
            p.x - cameraX - size,  // Left (adjusted for camera)
            p.y - size,            // Top
            p.x - cameraX + size,  // Right (adjusted for camera)
            p.y + size             // Bottom
        );

        // Clean up GDI objects to prevent memory leaks
        DeleteObject(brush);
        DeleteObject(pen);
    }
}

/**
 * Draws the player's health bar
 * @param hdc - Drawing context
 */
void drawHealthBar(HDC hdc) {
    int barWidth = 100;   // Width of health bar
    int barHeight = 15;   // Height of health bar
    int barX = 10;        // X position
    int barY = 120;       // Y position

    // Draw background (empty health)
    HBRUSH bgBrush = CreateSolidBrush(RGB(100, 0, 0));  // Dark red
    SelectObject(hdc, bgBrush);
    Rectangle(hdc, barX, barY, barX + barWidth, barY + barHeight);
    DeleteObject(bgBrush);

    // Draw current health (filled portion)
    float healthPercent = lives / 3.0f;  // Calculate percentage (3 = max lives)
    HBRUSH healthBrush = CreateSolidBrush(RGB(0, 255, 0));  // Green
    SelectObject(hdc, healthBrush);
    Rectangle(hdc, barX, barY,
        barX + (int)(barWidth * healthPercent),  // Width based on health
        barY + barHeight);
    DeleteObject(healthBrush);

    // Draw border around health bar
    HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));  // Black border
    SelectObject(hdc, borderPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));  // No fill
    Rectangle(hdc, barX, barY, barX + barWidth, barY + barHeight);
    DeleteObject(borderPen);
}

/**
 * Main drawing function - renders entire game scene
 * @param hdc - Drawing context
 */
void drawGame(HDC hdc) {
    // ===== BACKGROUND =====
    HBRUSH skyBrush = CreateSolidBrush(RGB(135, 206, 235));  // Sky blue
    RECT skyRect = { 0, 0, SCREEN_W, GROUND_Y };  // Area above ground
    FillRect(hdc, &skyRect, skyBrush);  // Fill with sky color
    DeleteObject(skyBrush);

    // ===== GROUND SEGMENTS =====
    for (const auto& ground : groundSegments) {
        // Draw ground platform
        HBRUSH groundBrush = CreateSolidBrush(RGB(100, 70, 40));  // Brown
        SelectObject(hdc, groundBrush);

        Rectangle(
            hdc,
            ground.x1 - cameraX,  // Left (adjusted for camera scrolling)
            GROUND_Y,             // Top (ground level)
            ground.x2 - cameraX,  // Right (adjusted for camera)
            GROUND_Y + 50         // Bottom (50 pixels thick)
        );

        // Draw spikes if this ground has them
        if (ground.hasSpike) {
            HBRUSH spikeBrush = CreateSolidBrush(RGB(200, 50, 50));  // Red
            SelectObject(hdc, spikeBrush);

            // Draw multiple spike triangles along the platform
            for (int x = ground.x1 + 10; x < ground.x2; x += 30) {
                POINT spike[3] = {
                    {x - cameraX, GROUND_Y},                // Bottom left
                    {x + 10 - cameraX, GROUND_Y - 20},      // Top point
                    {x + 20 - cameraX, GROUND_Y}            // Bottom right
                };
                Polygon(hdc, spike, 3);  // Draw triangle
            }
            DeleteObject(spikeBrush);
        }

        DeleteObject(groundBrush);
    }

    // ===== COLLECTIBLE ITEMS =====
    for (const auto& col : collectibles) {
        // Only draw uncollected items
        if (!col.collected) {
            HBRUSH colBrush;
            // Set color based on collectible type
            switch (col.type) {
            case 0: colBrush = CreateSolidBrush(RGB(255, 215, 0)); break; // Gold - coin
            case 1: colBrush = CreateSolidBrush(RGB(0, 255, 0)); break;   // Green - health
            case 2: colBrush = CreateSolidBrush(RGB(255, 0, 255)); break; // Purple - powerup
            default: colBrush = CreateSolidBrush(RGB(255, 255, 255));     // White - fallback
            }

            SelectObject(hdc, colBrush);
            // Draw collectible as a circle
            Ellipse(hdc,
                col.x - cameraX,      // Left (camera adjusted)
                col.y,                // Top
                col.x - cameraX + 20, // Right (20 pixels wide)
                col.y + 20            // Bottom (20 pixels tall)
            );
            DeleteObject(colBrush);
        }
    }

    // ===== FINISH LINE =====
    HBRUSH finishBrush = CreateSolidBrush(RGB(0, 255, 0));  // Green
    SelectObject(hdc, finishBrush);
    Rectangle(
        hdc,
        LEVEL_END_X - cameraX,   // Left side of finish line
        GROUND_Y - 60,           // Top (60 pixels above ground)
        LEVEL_END_X + 10 - cameraX, // Right side
        GROUND_Y                 // Bottom (at ground level)
    );
    DeleteObject(finishBrush);

    // ===== ENEMIES =====
    for (const auto& enemy : enemies) {
        if (!enemy.isActive) continue;  // Skip inactive enemies

        // Draw enemy body
        HBRUSH enemyBrush = CreateSolidBrush(RGB(200, 60, 60));  // Red
        SelectObject(hdc, enemyBrush);
        Rectangle(
            hdc,
            enemy.x - cameraX,           // Left
            enemy.y,                     // Top
            enemy.x - cameraX + PLAYER_W, // Right
            enemy.y + PLAYER_H           // Bottom
        );
        DeleteObject(enemyBrush);

        // Draw enemy eyes
        HBRUSH eyeBrush = CreateSolidBrush(RGB(255, 255, 255));  // White
        SelectObject(hdc, eyeBrush);
        // Left eye
        Ellipse(hdc,
            enemy.x - cameraX + 10,  // Left
            enemy.y + 10,            // Top
            enemy.x - cameraX + 20,  // Right
            enemy.y + 20             // Bottom
        );
        // Right eye
        Ellipse(hdc,
            enemy.x - cameraX + 25,  // Left
            enemy.y + 10,            // Top
            enemy.x - cameraX + 35,  // Right
            enemy.y + 20             // Bottom
        );
        DeleteObject(eyeBrush);
    }

    // ===== PLAYER CHARACTER =====
    // Determine player color (flashes when invincible)
    COLORREF playerColor;
    if (playerInvincible) {
        // Flash between white and blue every 0.2 seconds
        playerColor = (fmod(gameTime, 0.2f) < 0.1f) ?
            RGB(255, 255, 255) :  // White
            RGB(100, 150, 255);   // Blue
    }
    else {
        playerColor = RGB(100, 150, 255);  // Normal blue
    }

    HBRUSH playerBrush = CreateSolidBrush(playerColor);
    SelectObject(hdc, playerBrush);

    // Draw player body (rectangle for torso)
    Rectangle(
        hdc,
        playerX - cameraX,           // Left (camera adjusted)
        playerY + 10,                // Top (head starts at playerY)
        playerX - cameraX + PLAYER_W, // Right
        playerY + PLAYER_H           // Bottom
    );

    // Draw player head (ellipse on top of body)
    Ellipse(hdc,
        playerX - cameraX + 5,       // Left (slightly inset)
        playerY,                     // Top
        playerX - cameraX + PLAYER_W - 5, // Right (slightly inset)
        playerY + 20                 // Bottom (20 pixels tall)
    );

    DeleteObject(playerBrush);

    // ===== PARTICLE EFFECTS =====
    drawParticles(hdc);

    // ===== HUD (HEADS-UP DISPLAY) =====
    // Draw ASCII art player icon
    std::vector<std::wstring> playerArt = {
        L"  O  ",  // Head
        L" /|\\ ", // Arms and torso
        L" / \\ "  // Legs
    };
    drawAscii(hdc, 10, 10, playerArt, RGB(50, 100, 200));

    // Draw health bar
    drawHealthBar(hdc);

    // Draw score and coin count
    std::wstringstream ss;
    ss << L"Score: " << score << L"  Coins: " << coinsCollected;
    TextOutW(hdc, 10, 140, ss.str().c_str(), ss.str().length());

    // Draw game time
    ss.str(L"");
    ss << L"Time: " << (int)gameTime << "s";
    TextOutW(hdc, 10, 160, ss.str().c_str(), ss.str().length());

    // ===== GAME STATE SCREENS =====
    // START SCREEN
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

    // DEATH SCREEN
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

    // FINISH SCREEN
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

    // PAUSE SCREEN
    if (gameState == STATE_PAUSED) {
        drawAscii(hdc, 300, 200, {
            L" PAUSED ",
            L"",
            L" PRESS P TO CONTINUE "
            }, RGB(255, 255, 0));
    }
}

// ==================== WINDOW PROCEDURE ====================
/**
 * Main message handler for the window
 * Processes Windows messages like key presses, paint requests, timer events
 * @param hwnd - Window handle
 * @param msg - Message type
 * @param wParam, lParam - Message parameters
 */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

        // ===== TIMER EVENT =====
        // Called every 16ms (approximately 60 times per second)
    case WM_TIMER:
        if (gameState == STATE_PLAYING) {
            updateGame();  // Update game logic
        }
        // Force window to redraw
        InvalidateRect(hwnd, NULL, FALSE);
        break;

        // ===== KEY PRESSED =====
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_RETURN:  // Enter key
            if (gameState == STATE_START)
                gameState = STATE_PLAYING;  // Start game
            break;

        case 'R':  // R key
            resetGame();  // Restart game
            break;

        case 'P':  // P key
            // Toggle pause state
            if (gameState == STATE_PLAYING)
                gameState = STATE_PAUSED;
            else if (gameState == STATE_PAUSED)
                gameState = STATE_PLAYING;
            break;

        case 'A':  // A key
            leftKey = true;  // Set left movement flag
            break;

        case 'D':  // D key
            rightKey = true;  // Set right movement flag
            break;

        case VK_SPACE:  // Spacebar
            if (gameState == STATE_PLAYING && onGround) {
                velY = JUMP_FORCE;  // Make player jump
                // Create jump dust particles
                addParticles(playerX + PLAYER_W / 2, playerY + PLAYER_H,
                    10, RGB(200, 200, 200));
            }
            break;
        }
        break;

        // ===== KEY RELEASED =====
    case WM_KEYUP:
        switch (wParam) {
        case 'A':
            leftKey = false;  // Clear left movement flag
            break;

        case 'D':
            rightKey = false;  // Clear right movement flag
            break;
        }
        break;

        // ===== BACKGROUND ERASE =====
        // Prevent Windows from erasing background (we draw everything ourselves)
    case WM_ERASEBKGND:
        return 1;  // Tell Windows we handled background erasing

    // ===== PAINT EVENT =====
    // Window needs to be drawn/redrawn
    case WM_PAINT: {
        PAINTSTRUCT ps;  // Paint structure
        HDC hdc = BeginPaint(hwnd, &ps);  // Get drawing context

        RECT rc;
        GetClientRect(hwnd, &rc);  // Get window dimensions

        // ===== DOUBLE BUFFERING SETUP =====
        // Create memory DC (off-screen buffer) to prevent flickering
        HDC memDC = CreateCompatibleDC(hdc);  // Compatible with screen
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        SelectObject(memDC, memBmp);  // Select bitmap into memory DC

        // ===== DRAW GAME TO BUFFER =====
        drawGame(memDC);

        // ===== COPY BUFFER TO SCREEN =====
        // Fast copy from memory buffer to screen
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        // ===== CLEANUP =====
        DeleteObject(memBmp);  // Delete bitmap
        DeleteDC(memDC);       // Delete memory DC
        EndPaint(hwnd, &ps);   // End painting
        break;
    }

                 // ===== WINDOW CLOSING =====
    case WM_DESTROY:
        PostQuitMessage(0);  // Exit application
        break;

        // ===== DEFAULT MESSAGE HANDLING =====
    default:
        // Let Windows handle any messages we don't process
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ==================== MAIN ENTRY POINT ====================
/**
 * Windows application entry point
 * Creates window, sets up timer, runs message loop
 * @param hInst - Application instance handle
 * @param - Unused parameters
 * @param - Unused parameters
 * @param nCmdShow - How to show window
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    // ===== REGISTER WINDOW CLASS =====
    // Define properties of our window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;            // Message handler function
    wc.hInstance = hInst;                // Application instance
    wc.lpszClassName = L"PitsPlatformer"; // Class name
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); // Standard arrow cursor
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Default window background

    // Register class with Windows
    RegisterClass(&wc);

    // ===== CALCULATE WINDOW SIZE =====
    // Account for window borders and title bar
    RECT rc = { 0, 0, SCREEN_W, SCREEN_H };  // Desired client area
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);  // Add borders

    // ===== CREATE WINDOW =====
    HWND hwnd = CreateWindow(
        L"PitsPlatformer",               // Class name
        L"2D Platformer - Enhanced Edition", // Window title
        WS_OVERLAPPEDWINDOW,             // Window style
        100, 100,                        // Window position (x, y)
        rc.right - rc.left,              // Window width
        rc.bottom - rc.top,              // Window height
        NULL, NULL, hInst, NULL          // Parent, menu, instance, data
    );

    // ===== SHOW WINDOW =====
    ShowWindow(hwnd, nCmdShow);   // Make window visible
    UpdateWindow(hwnd);           // Force initial paint

    // ===== SET UP GAME TIMER =====
    // Timer ID = 1, interval = 16ms (approximately 60 FPS)
    SetTimer(hwnd, 1, 16, NULL);

    // ===== MESSAGE LOOP =====
    // Main loop that keeps application running
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);  // Convert key messages
        DispatchMessage(&msg);   // Send to WndProc
    }

    return 0;
}