#ifndef UI_H
#define UI_H

#include "Renderer.h"

class Fighter;
class Enemy;

// Everything the player reads on screen that isn't the fighters themselves:
// health and energy bars, labels, the main menu and the pause screen.
class UI {
public:
    explicit UI(const Renderer& renderer);

    void drawDebugBoxes(const Fighter& fighter) const;           // hurtbox (green) and hitbox (red)
    void drawHUD(const Fighter& player, const Enemy& enemy) const; // the in-fight display
    void drawMenu() const;                                        // title screen
    void drawPauseScreen() const;

private:
    void drawHealthBar(float x, float y, float width, float height,
                       const Fighter& fighter, const char* name) const;
    void drawEnergyBar(float x, float y, float width, float height,
                       const Fighter& fighter) const;

    const Renderer& renderer;
};

#endif
