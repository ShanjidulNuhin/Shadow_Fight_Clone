#include <GL/glut.h>
#include <cstdlib>
#include <cstdio>
#include "Config.h"
#include "InputManager.h"
#include "Renderer.h"
#include "Player.h"

int lastTime = 0;
int fpsTimer = 0;
int fpsFrames = 0;

// GLUT callbacks can't carry objects, so these are shared instances for now.
InputManager input;
Renderer renderer(Config::WORLD_WIDTH, Config::WORLD_HEIGHT);
Player player(100.0f, 400.0f);

void display() {
    renderer.beginFrame();

    // ground
    renderer.drawRect(0, 0, Config::WORLD_WIDTH, Config::GROUND_HEIGHT,
                      0.30f, 0.20f, 0.12f);

    player.draw(renderer);

    // debug label: what is the player doing right now?
    char label[64];
    std::snprintf(label, sizeof(label), "State: %s", player.getStateName());
    renderer.drawText(10.0f, Config::WORLD_HEIGHT - 25.0f, label, 1.0f, 1.0f, 1.0f);

    renderer.endFrame();
}

void idle() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    int elapsedMs = now - lastTime;
    float deltaTime = elapsedMs / 1000.0f;
    lastTime = now;
    if (deltaTime > 0.25f) deltaTime = 0.25f;

    player.update(input, deltaTime);

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
    if (key == 27) std::exit(0);   // ESC
    input.keyDown(key);
}

void keyboardUp(unsigned char key, int, int) {
    input.keyUp(key);
}

void reshape(int w, int h) {
    renderer.resize(w, h);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(Config::WORLD_WIDTH, Config::WORLD_HEIGHT);
    glutCreateWindow("Shadow Fight");

    renderer.init();
    glutIgnoreKeyRepeat(1);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutIdleFunc(idle);

    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutMainLoop();
    return 0;
}
