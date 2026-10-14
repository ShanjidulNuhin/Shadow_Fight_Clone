#include <windows.h>
#include <GL/glut.h>
#include <cstdlib>
#include <cstdio>

const int WIN_W = 800;
const int WIN_H = 600;
const float SQUARE_SIZE = 50.0f;

float squareX = 100.0f;
float squareSpeed = 250.0f;   // pixels per second

int lastTime = 0;             // time of the previous frame (ms)
int fpsTimer = 0;             // ms accumulated since last FPS update
int fpsFrames = 0;            // frames counted since last FPS update

// Asks the graphics driver to sync buffer swaps to the monitor refresh rate.
// If the driver doesn't support it, nothing happens (the game still works).
void enableVSync() {
    typedef BOOL (WINAPI *SwapIntervalFunc)(int);
    SwapIntervalFunc wglSwapInterval =
        (SwapIntervalFunc)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapInterval) {
        wglSwapInterval(1);
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glVertex2f(squareX,               280.0f);
        glVertex2f(squareX + SQUARE_SIZE, 280.0f);
        glVertex2f(squareX + SQUARE_SIZE, 280.0f + SQUARE_SIZE);
        glVertex2f(squareX,               280.0f + SQUARE_SIZE);
    glEnd();

    glutSwapBuffers();
}

// Called by GLUT whenever it has nothing else to do (i.e. constantly).
void idle() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    int elapsedMs = now - lastTime;
    float deltaTime = elapsedMs / 1000.0f;
    lastTime = now;

    if (deltaTime > 0.25f) deltaTime = 0.25f;

    // --- game update ---
    squareX += squareSpeed * deltaTime;

    if (squareX + SQUARE_SIZE > WIN_W) {
        squareX = WIN_W - SQUARE_SIZE;
        squareSpeed = -squareSpeed;
    }
    if (squareX < 0) {
        squareX = 0;
        squareSpeed = -squareSpeed;
    }

    // --- FPS counter shown in the window title ---
    fpsFrames++;
    fpsTimer += elapsedMs;
    if (fpsTimer >= 1000) {
        char title[64];
        std::snprintf(title, sizeof(title), "Shadow Fight - FPS: %d", fpsFrames);
        glutSetWindowTitle(title);
        fpsFrames = 0;
        fpsTimer = 0;
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) std::exit(0);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Shadow Fight");

    enableVSync();   // must come AFTER the window (OpenGL context) exists

    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);

    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutMainLoop();
    return 0;
}
