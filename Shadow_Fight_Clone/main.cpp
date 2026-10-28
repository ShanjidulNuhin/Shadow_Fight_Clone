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
    glutCreateWindow("Shadow Fight");

    game.init();                 // (needs the window to exist)
    glutIgnoreKeyRepeat(1);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouseClick);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(mousePassiveMotion);
    glutIdleFunc(idle);

    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutMainLoop();
    return 0;
}
