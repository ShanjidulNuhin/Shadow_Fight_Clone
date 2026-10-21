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

void Renderer::drawRectOutline(float x, float y, float width, float height,
                               float r, float g, float b) const {
    glColor3f(r, g, b);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,         y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x,         y + height);
    glEnd();
}

void Renderer::drawTextCentered(float centerX, float y, const char* text,
                                float r, float g, float b, bool large) const {
    void* font = large ? GLUT_BITMAP_TIMES_ROMAN_24 : GLUT_BITMAP_HELVETICA_18;
    int textWidth = glutBitmapLength(font, (const unsigned char*)text);

    glColor3f(r, g, b);
    glRasterPos2f(centerX - textWidth / 2.0f, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

void Renderer::drawOverlay(float r, float g, float b, float alpha) const {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, alpha);
    glBegin(GL_QUADS);
        glVertex2f(0,                  0);
        glVertex2f((float)worldW,      0);
        glVertex2f((float)worldW,      (float)worldH);
        glVertex2f(0,                  (float)worldH);
    glEnd();
    glDisable(GL_BLEND);
}

void Renderer::drawText(float x, float y, const char* text,
                        float r, float g, float b) const {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}
