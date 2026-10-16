#include "Renderer.h"
#include <windows.h>
#include <GL/glut.h>

Renderer::Renderer(int worldWidth, int worldHeight)
    : worldW(worldWidth), worldH(worldHeight) {
}

void Renderer::init() {
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

    // Ask the driver to sync buffer swaps to the monitor refresh rate (VSync).
    typedef BOOL (WINAPI *SwapIntervalFunc)(int);
    SwapIntervalFunc wglSwapInterval =
        (SwapIntervalFunc)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapInterval) {
        wglSwapInterval(1);
    }
}

void Renderer::resize(int windowW, int windowH) {
    glViewport(0, 0, windowW, windowH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, worldW, 0, worldH);
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::beginFrame() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::endFrame() {
    glutSwapBuffers();
}

void Renderer::drawRect(float x, float y, float width, float height,
                        float r, float g, float b) const {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x,         y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x,         y + height);
    glEnd();
}
