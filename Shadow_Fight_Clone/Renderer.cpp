#include "Renderer.h"
#include <windows.h>
#include <GL/glut.h>
#include <cmath>

Renderer::Renderer(int worldWidth, int worldHeight)
    : worldW(worldWidth), worldH(worldHeight) {
}

void Renderer::init() {
    glClearColor(0.015f, 0.02f, 0.045f, 1.0f); // Dark midnight sky clear color

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

void Renderer::drawRectAlpha(float x, float y, float width, float height,
                             float r, float g, float b, float alpha) const {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r, g, b, alpha);
    glBegin(GL_QUADS);
        glVertex2f(x,         y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x,         y + height);
    glEnd();
    glDisable(GL_BLEND);
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

void Renderer::drawGradientRect(float x, float y, float width, float height,
                                float rBottom, float gBottom, float bBottom, float aBottom,
                                float rTop, float gTop, float bTop, float aTop) const {
    bool hasAlpha = (aBottom < 0.999f || aTop < 0.999f);
    if (hasAlpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glBegin(GL_QUADS);
        glColor4f(rBottom, gBottom, bBottom, aBottom);
        glVertex2f(x,         y);
        glVertex2f(x + width, y);
        glColor4f(rTop, gTop, bTop, aTop);
        glVertex2f(x + width, y + height);
        glVertex2f(x,         y + height);
    glEnd();

    if (hasAlpha) {
        glDisable(GL_BLEND);
    }
}

void Renderer::drawCircle(float cx, float cy, float radius,
                          float r, float g, float b, float alpha,
                          int segments) const {
    bool hasAlpha = (alpha < 0.999f);
    if (hasAlpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glColor4f(r, g, b, alpha);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.14159265f * (float)i / (float)segments;
            glVertex2f(cx + radius * std::cos(angle), cy + radius * std::sin(angle));
        }
    glEnd();

    if (hasAlpha) {
        glDisable(GL_BLEND);
    }
}

void Renderer::drawCircleOutline(float cx, float cy, float radius,
                                 float r, float g, float b, float alpha,
                                 float lineWidth, int segments) const {
    bool hasAlpha = (alpha < 0.999f);
    if (hasAlpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glLineWidth(lineWidth);
    glColor4f(r, g, b, alpha);
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; ++i) {
            float angle = 2.0f * 3.14159265f * (float)i / (float)segments;
            glVertex2f(cx + radius * std::cos(angle), cy + radius * std::sin(angle));
        }
    glEnd();

    if (hasAlpha) {
        glDisable(GL_BLEND);
    }
}

void Renderer::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                            float r, float g, float b, float alpha) const {
    bool hasAlpha = (alpha < 0.999f);
    if (hasAlpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glColor4f(r, g, b, alpha);
    glBegin(GL_TRIANGLES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
    glEnd();

    if (hasAlpha) {
        glDisable(GL_BLEND);
    }
}

void Renderer::drawQuad(float x1, float y1, float x2, float y2,
                        float x3, float y3, float x4, float y4,
                        float r, float g, float b, float alpha) const {
    bool hasAlpha = (alpha < 0.999f);
    if (hasAlpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glColor4f(r, g, b, alpha);
    glBegin(GL_QUADS);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
        glVertex2f(x4, y4);
    glEnd();

    if (hasAlpha) {
        glDisable(GL_BLEND);
    }
}

void Renderer::drawLine(float x1, float y1, float x2, float y2,
                        float r, float g, float b, float alpha,
                        float width) const {
    bool hasAlpha = (alpha < 0.999f);
    if (hasAlpha) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glLineWidth(width);
    glColor4f(r, g, b, alpha);
    glBegin(GL_LINES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
    glEnd();

    if (hasAlpha) {
        glDisable(GL_BLEND);
    }
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
