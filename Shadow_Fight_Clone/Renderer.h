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

    void drawRect(float x, float y, float width, float height,
                  float r, float g, float b) const;

    // Just the border of a rectangle (used for debug boxes).
    void drawRectOutline(float x, float y, float width, float height,
                         float r, float g, float b) const;

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
