#ifndef RENDERER_H
#define RENDERER_H

// Owns all drawing. The rest of the game asks the Renderer to draw things
// instead of calling OpenGL directly.
//
// Coordinate system: origin (0,0) is the BOTTOM-LEFT corner,
// x grows to the right, y grows upward, measured in "world" pixels.
class Renderer {
public:
    Renderer(int worldWidth, int worldHeight);

    void init();                           // one-time OpenGL setup (call after the window exists)
    void resize(int windowW, int windowH); // viewport + projection (call from GLUT reshape)

    void beginFrame();                     // clear the screen
    void endFrame();                       // show what was drawn

    // Camera / screen offset for cinematic screen shake
    void setCameraOffset(float offsetX, float offsetY);
    void resetCamera();

    void drawRect(float x, float y, float width, float height,
                  float r, float g, float b) const;

    // Semi-transparent rectangle
    void drawRectAlpha(float x, float y, float width, float height,
                       float r, float g, float b, float alpha) const;

    // Just the border of a rectangle (used for debug boxes).
    void drawRectOutline(float x, float y, float width, float height,
                         float r, float g, float b) const;

    // Vertical gradient rectangle (bottom to top)
    void drawGradientRect(float x, float y, float width, float height,
                          float rBottom, float gBottom, float bBottom, float aBottom,
                          float rTop, float gTop, float bTop, float aTop) const;

    // Horizontal gradient rectangle (left to right)
    void drawGradientRectH(float x, float y, float width, float height,
                           float rLeft, float gLeft, float bLeft, float aLeft,
                           float rRight, float gRight, float bRight, float aRight) const;

    // Filled circle (used for moon, halos, lantern glow, particles, joints)
    void drawCircle(float cx, float cy, float radius,
                    float r, float g, float b, float alpha = 1.0f,
                    int segments = 28) const;

    // Outlined circle
    void drawCircleOutline(float cx, float cy, float radius,
                           float r, float g, float b, float alpha = 1.0f,
                           float lineWidth = 1.5f, int segments = 28) const;

    // Triangle (used for mountain peaks, roof eaves, and particles)
    void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                      float r, float g, float b, float alpha = 1.0f) const;

    // Arbitrary convex quad (used for mountain ridges, angled limbs, and trapezoids)
    void drawQuad(float x1, float y1, float x2, float y2,
                  float x3, float y3, float x4, float y4,
                  float r, float g, float b, float alpha = 1.0f) const;

    // 2D Line
    void drawLine(float x1, float y1, float x2, float y2,
                  float r, float g, float b, float alpha = 1.0f,
                  float width = 1.0f) const;

    // Tapered rounded limb connecting (x1, y1) to (x2, y2) with radii r1 and r2
    void drawLimb(float x1, float y1, float x2, float y2,
                  float r1, float r2,
                  float r, float g, float b, float alpha = 1.0f) const;

    // (x, y) is the bottom-left of the first letter.
    void drawText(float x, float y, const char* text,
                  float r, float g, float b) const;

    // Text centred horizontally on centerX. large = a bigger font for titles.
    void drawTextCentered(float centerX, float y, const char* text,
                          float r, float g, float b, bool large) const;

    // A see-through colour laid over the whole screen (to dim the scene behind a menu).
    void drawOverlay(float r, float g, float b, float alpha) const;

private:
    int worldW;
    int worldH;
};

#endif
