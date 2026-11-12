#include <windows.h>
#include <GL/glut.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include "Config.h"
#include "Game.h"

int lastTime = 0;
int fpsTimer = 0;
int fpsFrames = 0;
int currentWindowW = Config::WORLD_WIDTH;
int currentWindowH = Config::WORLD_HEIGHT;

// GLUT callbacks can't carry objects, so the one Game lives here.
Game game;

void display() {
    game.draw();
}

void idle() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    int elapsedMs = now - lastTime;
    float deltaTime = elapsedMs / 1000.0f;
    lastTime = now;
    if (deltaTime > 0.25f) deltaTime = 0.25f;

    game.update(deltaTime);
    if (game.shouldQuit()) std::exit(0);

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

void keyboardDown(unsigned char key, int, int) {
    game.onKeyDown(key);
    if (game.shouldQuit()) std::exit(0);
}

void keyboardUp(unsigned char key, int, int) {
    game.onKeyUp(key);
}

void specialDown(int key, int, int) {
    game.onSpecialKey(key, false);
}

void specialUp(int key, int, int) {
    game.onSpecialKey(key, true);
}

void mouseClick(int button, int state, int x, int y) {
    float worldX = (float)x * (float)Config::WORLD_WIDTH / (float)currentWindowW;
    float worldY = (float)(currentWindowH - y) * (float)Config::WORLD_HEIGHT / (float)currentWindowH;
    game.onMouseClick(button, state, worldX, worldY);
    if (game.shouldQuit()) std::exit(0);
}

void mouseMotion(int x, int y) {
    float worldX = (float)x * (float)Config::WORLD_WIDTH / (float)currentWindowW;
    float worldY = (float)(currentWindowH - y) * (float)Config::WORLD_HEIGHT / (float)currentWindowH;
    game.onMouseMove(worldX, worldY);
}

void mousePassiveMotion(int x, int y) {
    float worldX = (float)x * (float)Config::WORLD_WIDTH / (float)currentWindowW;
    float worldY = (float)(currentWindowH - y) * (float)Config::WORLD_HEIGHT / (float)currentWindowH;
    game.onMouseMove(worldX, worldY);
}

void reshape(int w, int h) {
    currentWindowW = (w > 0) ? w : 1;
    currentWindowH = (h > 0) ? h : 1;
    game.onResize(w, h);
}

int main(int argc, char** argv) {
    std::srand((unsigned)std::time(nullptr));   // seed the random numbers the enemy uses

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(Config::WORLD_WIDTH, Config::WORLD_HEIGHT);

    RECT workArea = { 0, 0, 1280, 720 };
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
    int workW = workArea.right - workArea.left;
    int workH = workArea.bottom - workArea.top;

    RECT desired = { 0, 0, Config::WORLD_WIDTH, Config::WORLD_HEIGHT };
    AdjustWindowRect(&desired, WS_OVERLAPPEDWINDOW, FALSE);
    int frameW = desired.right - desired.left;
    int frameH = desired.bottom - desired.top;
    int initX = workArea.left + (workW - frameW) / 2;
    int initY = workArea.top  + (workH - frameH) / 2;
    if (initX < workArea.left) initX = workArea.left;
    if (initY < workArea.top)  initY = workArea.top;
    glutInitWindowPosition(initX, initY);

    glutCreateWindow("Shadow Fight");

    HWND hwnd = GetActiveWindow();
    if (hwnd == nullptr) {
        hwnd = FindWindowA(nullptr, "Shadow Fight");
    }
    if (hwnd != nullptr) {
        RECT winRect = {};
        if (GetWindowRect(hwnd, &winRect)) {
            int actualW = winRect.right - winRect.left;
            int actualH = winRect.bottom - winRect.top;
            int cx = workArea.left + (workW - actualW) / 2;
            int cy = workArea.top  + (workH - actualH) / 2;
            if (cx < workArea.left) cx = workArea.left;
            if (cy < workArea.top)  cy = workArea.top;
            SetWindowPos(hwnd, nullptr, cx, cy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    game.init();                 // (needs the window to exist)
    glutIgnoreKeyRepeat(1);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutMouseFunc(mouseClick);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(mousePassiveMotion);
    glutIdleFunc(idle);

    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutMainLoop();
    return 0;
}
