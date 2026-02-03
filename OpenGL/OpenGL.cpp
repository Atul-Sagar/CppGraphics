#include <GL/glut.h> // OpenGL and GLUT libraries

float cameraZ = 10.0f;

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Simple camera
    gluLookAt(0, 5, cameraZ,  // Eye position
        0, 0, 0,        // Look at point
        0, 1, 0);       // Up vector

// Draw ground
    glColor3f(0.3f, 0.7f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-10, 0, -10);
    glVertex3f(10, 0, -10);
    glVertex3f(10, 0, 10);
    glVertex3f(-10, 0, 10);
    glEnd();

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 'w') cameraZ -= 0.5f;
    if (key == 's') cameraZ += 0.5f;
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Simple 3D Ground");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc([](int w, int h) {
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45, (float)w / h, 0.1, 100);
        glMatrixMode(GL_MODELVIEW);
        });

    glutMainLoop();
    return 0;
}