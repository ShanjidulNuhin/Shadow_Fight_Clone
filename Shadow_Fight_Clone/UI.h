#ifndef UI_H
#define UI_H

#include "Renderer.h"

class Fighter;
class Enemy;

// Everything the player reads on screen that isn't the fighters themselves:
// health and energy bars, labels, round markers, and all the full-screen messages.
class UI {
public:
    explicit UI(const Renderer& renderer);

    void drawDebugBoxes(const Fighter& fighter) const;           // hurtbox (green) and hitbox (red)

    // The in-fight display. roundNumber is 1, 2, 3...; the wins are rounds won so far.
    void drawHUD(const Fighter& player, const Enemy& enemy,
                 int roundNumber, int playerWins, int enemyWins) const;

    void drawMenu() const;                                        // title screen
    void drawPauseScreen() const;
    void drawRoundIntro(int roundNumber, bool showFight) const;   // "ROUND 2", then "FIGHT!"
    void drawRoundOver(const char* detail) const;                 // "K.O." plus a message
    void drawGameOver(bool playerWon, int playerWins, int enemyWins) const;

private:
    void drawHealthBar(float x, float y, float width, float height,
                       const Fighter& fighter, const char* name) const;
    void drawEnergyBar(float x, float y, float width, float height,
                       const Fighter& fighter) const;
    void drawRoundPips(float x, float y, int wins) const;         // little squares: rounds won

    const Renderer& renderer;
};

#endif
