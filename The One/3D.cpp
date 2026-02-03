#include <GL/glut.h>  // OpenGL and GLUT libraries
#include <cmath>      // Math functions

// ==================== CAMERA VARIABLES ====================
float cameraX = 0.0f;      // Camera X position
float cameraY = 5.0f;      // Camera Y position (height)
float cameraZ = 10.0f;     // Camera Z position (distance)
float cameraAngleX = 0.0f; // Camera horizontal rotation
float cameraAngleY = 0.0f; // Camera vertical rotation

// ==================== GROUND VARIABLES ====================
const float GROUND_SIZE = 20.0f;    // Size of ground (20x20 units)
const int GRID_SIZE = 20;           // Grid divisions (20x20)

// ==================== LIGHTING VARIABLES ====================
float lightPosition[] = { 5.0f, 10.0f, 5.0f, 1.0f };  // Light position (x,y,z,w)
float lightAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };   // Ambient light color
float lightDiffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };   // Diffuse light color

// ==================== INITIALIZATION ====================
/**
 * Initializes OpenGL settings
 */
void init() {
    // Set background color (sky blue)
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    // Enable depth testing (for proper 3D rendering)
    glEnable(GL_DEPTH_TEST);

    // Enable lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Set up light
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);

    // Enable smooth shading
    glShadeModel(GL_SMOOTH);

    // Enable color material (so objects have color with lighting)
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

// ==================== DRAWING FUNCTIONS ====================
/**
 * Draws a simple ground plane with grid lines
 */
void drawGround() {
    // Set ground color (green)
    glColor3f(0.3f, 0.7f, 0.3f);

    // Draw ground as a quad (flat rectangle)
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);  // Normal pointing up (for lighting)
    glVertex3f(-GROUND_SIZE, 0.0f, -GROUND_SIZE);  // Back-left
    glVertex3f(GROUND_SIZE, 0.0f, -GROUND_SIZE);   // Back-right
    glVertex3f(GROUND_SIZE, 0.0f, GROUND_SIZE);    // Front-right
    glVertex3f(-GROUND_SIZE, 0.0f, GROUND_SIZE);   // Front-left
    glEnd();

    // Draw grid lines on ground
    glColor3f(0.5f, 0.5f, 0.5f);  // Gray color for grid
    glBegin(GL_LINES);
    // Draw horizontal grid lines
    for (int i = -GRID_SIZE; i <= GRID_SIZE; i++) {
        float pos = i * (GROUND_SIZE / GRID_SIZE);

        // Line from left to right
        glVertex3f(-GROUND_SIZE, 0.01f, pos);  // Slightly above ground to avoid z-fighting
        glVertex3f(GROUND_SIZE, 0.01f, pos);

        // Line from back to front
        glVertex3f(pos, 0.01f, -GROUND_SIZE);
        glVertex3f(pos, 0.01f, GROUND_SIZE);
    }
    glEnd();
}

/**
 * Draws coordinate axes (X, Y, Z) for reference
 */
void drawAxes() {
    // Disable lighting for axes
    glDisable(GL_LIGHTING);

    glLineWidth(2.0f);  // Thicker lines for axes

    glBegin(GL_LINES);
    // X-axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(5.0f, 0.0f, 0.0f);

    // Y-axis (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 5.0f, 0.0f);

    // Z-axis (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 5.0f);
    glEnd();

    // Re-enable lighting
    glEnable(GL_LIGHTING);
}

/**
 * Main display function - called when window needs to be redrawn
 */
void display() {
    // Clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset transformation matrix
    glLoadIdentity();

    // ===== SET UP CAMERA =====
    // Position camera
    glTranslatef(0.0f, 0.0f, -cameraZ);  // Move camera back
    glRotatef(cameraAngleY, 1.0f, 0.0f, 0.0f);  // Rotate up/down
    glRotatef(cameraAngleX, 0.0f, 1.0f, 0.0f);  // Rotate left/right
    glTranslatef(-cameraX, -cameraY, 0.0f);  // Move camera to position

    // ===== DRAW SCENE =====
    drawAxes();    // Draw coordinate axes
    drawGround();  // Draw ground plane

    // Swap buffers (double buffering)
    glutSwapBuffers();
}

// ==================== RESHAPE FUNCTION ====================
/**
 * Called when window is resized
 * @param width - New window width
 * @param height - New window height
 */
void reshape(int width, int height) {
    // Prevent division by zero
    if (height == 0) height = 1;

    // Set viewport to cover entire window
    glViewport(0, 0, width, height);

    // Switch to projection matrix mode
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Set up perspective projection
    // Parameters: FOV angle, aspect ratio, near clip, far clip
    gluPerspective(45.0f, (float)width / height, 0.1f, 100.0f);

    // Switch back to modelview matrix mode
    glMatrixMode(GL_MODELVIEW);
}

// ==================== INPUT HANDLING ====================
/**
 * Keyboard callback function
 * @param key - Key that was pressed
 * @param x, y - Mouse position when key was pressed
 */
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        // Camera movement
    case 'w':
    case 'W':
        cameraZ -= 0.5f;  // Move camera forward
        break;
    case 's':
    case 'S':
        cameraZ += 0.5f;  // Move camera backward
        break;
    case 'a':
    case 'A':
        cameraX -= 0.5f;  // Move camera left
        break;
    case 'd':
    case 'D':
        cameraX += 0.5f;  // Move camera right
        break;
    case 'q':
    case 'Q':
        cameraY += 0.5f;  // Move camera up
        break;
    case 'e':
    case 'E':
        cameraY -= 0.5f;  // Move camera down
        break;

        // Camera rotation
    case 'i':
    case 'I':
        cameraAngleY += 2.0f;  // Look up
        break;
    case 'k':
    case 'K':
        cameraAngleY -= 2.0f;  // Look down
        break;
    case 'j':
    case 'J':
        cameraAngleX -= 2.0f;  // Look left
        break;
    case 'l':
    case 'L':
        cameraAngleX += 2.0f;  // Look right
        break;

        // Reset view
    case 'r':
    case 'R':
        cameraX = 0.0f;
        cameraY = 5.0f;
        cameraZ = 10.0f;
        cameraAngleX = 0.0f;
        cameraAngleY = 0.0f;
        break;

        // Exit
    case 27:  // Escape key
        exit(0);
        break;
    }

    // Request redisplay
    glutPostRedisplay();
}

// ==================== SPECIAL KEYS ====================
/**
 * Special keys callback (arrow keys, function keys, etc.)
 */
void specialKeys(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:
        cameraAngleY += 2.0f;  // Look up
        break;
    case GLUT_KEY_DOWN:
        cameraAngleY -= 2.0f;  // Look down
        break;
    case GLUT_KEY_LEFT:
        cameraAngleX -= 2.0f;  // Look left
        break;
    case GLUT_KEY_RIGHT:
        cameraAngleX += 2.0f;  // Look right
        break;
    case GLUT_KEY_PAGE_UP:
        cameraY += 0.5f;  // Move up
        break;
    case GLUT_KEY_PAGE_DOWN:
        cameraY -= 0.5f;  // Move down
        break;
    }

    glutPostRedisplay();
}

// ==================== MOUSE INPUT ====================
int lastMouseX = 0;
int lastMouseY = 0;
bool mouseDown = false;

/**
 * Mouse button callback
 */
void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseDown = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else {
            mouseDown = false;
        }
    }
}

/**
 * Mouse motion callback
 */
void mouseMove(int x, int y) {
    if (mouseDown) {
        // Calculate mouse movement
        int deltaX = x - lastMouseX;
        int deltaY = y - lastMouseY;

        // Update camera rotation based on mouse movement
        cameraAngleX += deltaX * 0.5f;
        cameraAngleY += deltaY * 0.5f;

        // Clamp vertical rotation
        if (cameraAngleY > 90.0f) cameraAngleY = 90.0f;
        if (cameraAngleY < -90.0f) cameraAngleY = -90.0f;

        lastMouseX = x;
        lastMouseY = y;

        glutPostRedisplay();
    }
}

// ==================== TIMER FUNCTION ====================
/**
 * Timer callback for animation (not used in this simple scene)
 */
void timer(int value) {
    // Request redisplay
    glutPostRedisplay();

    // Set next timer
    glutTimerFunc(16, timer, 0);  // 60 FPS
}

// ==================== MAIN FUNCTION ====================
/**
 * Program entry point
 */
int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);

    // Set display mode:
    // - Double buffer for smooth animation
    // - RGB color mode
    // - Depth buffer for 3D
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Set window size and position
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);

    // Create window
    glutCreateWindow("Simple 3D Scene - Just Ground");

    // Initialize OpenGL
    init();

    // Register callback functions
    glutDisplayFunc(display);      // For rendering
    glutReshapeFunc(reshape);      // For window resizing
    glutKeyboardFunc(keyboard);    // For keyboard input
    glutSpecialFunc(specialKeys);  // For special keys
    glutMouseFunc(mouseButton);    // For mouse buttons
    glutMotionFunc(mouseMove);     // For mouse movement
    glutTimerFunc(0, timer, 0);    // For animation timer

    // Print controls
    printf("=== CONTROLS ===\n");
    printf("W/S/A/D: Move camera\n");
    printf("Q/E: Move camera up/down\n");
    printf("I/K/J/L: Rotate camera\n");
    printf("Arrow Keys: Rotate camera\n");
    printf("Page Up/Down: Move camera up/down\n");
    printf("Left Mouse Drag: Rotate camera\n");
    printf("R: Reset view\n");
    printf("ESC: Exit\n");

    // Start main loop
    glutMainLoop();

    return 0;
}