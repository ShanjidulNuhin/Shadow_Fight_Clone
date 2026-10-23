#include "UI.h"
#include "Fighter.h"
#include "Enemy.h"
#include "Config.h"
#include <cstdio>

UI::UI(const Renderer& rendererToUse)
    : renderer(rendererToUse) {
}

// Green outline = hurtbox (where it can be hit). Red outline = hitbox (where it hits).
void UI::drawDebugBoxes(const Fighter& fighter) const {
    Rect hurtbox = fighter.getHurtbox();
    renderer.drawRectOutline(hurtbox.x, hurtbox.y, hurtbox.width, hurtbox.height,
                             0.2f, 1.0f, 0.2f);

    Rect hitbox;
    if (fighter.getHitbox(hitbox)) {
        renderer.drawRectOutline(hitbox.x, hitbox.y, hitbox.width, hitbox.height,
                                 1.0f, 0.2f, 0.2f);
    }
}

// A simple health bar with a name and numbers.
void UI::drawHealthBar(float x, float y, float width, float height,
                       const Fighter& fighter, const char* name) const {
    float fraction = (float)fighter.getHealth() / (float)fighter.getMaxHealth();

    renderer.drawRect(x, y, width, height, 0.35f, 0.05f, 0.05f);           // empty part: dark red
    renderer.drawRect(x, y, width * fraction, height, 0.2f, 0.8f, 0.2f);   // remaining health: green
    renderer.drawRectOutline(x, y, width, height, 1.0f, 1.0f, 1.0f);

    char label[64];
    std::snprintf(label, sizeof(label), "%s  %d / %d",
                  name, fighter.getHealth(), fighter.getMaxHealth());
    renderer.drawText(x, y + height + 6.0f, label, 1.0f, 1.0f, 1.0f);
}

// A thin energy bar. It turns gold when there is enough energy for a special attack.
void UI::drawEnergyBar(float x, float y, float width, float height,
                       const Fighter& fighter) const {
    float fraction = fighter.getEnergy() / fighter.getMaxEnergy();
    bool specialReady = fighter.getEnergy() >= Attacks::SPECIAL.energyCost;

    renderer.drawRect(x, y, width, height, 0.05f, 0.10f, 0.25f);   // empty part: dark blue
    if (specialReady) {
        renderer.drawRect(x, y, width * fraction, height, 1.0f, 0.8f, 0.2f);   // gold: special ready
    } else {
        renderer.drawRect(x, y, width * fraction, height, 0.3f, 0.6f, 1.0f);   // blue: still charging
    }
    renderer.drawRectOutline(x, y, width, height, 1.0f, 1.0f, 1.0f);
}

// One little square per round needed to win; the ones already won are filled gold.
void UI::drawRoundPips(float x, float y, int wins) const {
    const float size = 14.0f;
    const float spacing = 20.0f;

    for (int i = 0; i < Config::ROUNDS_TO_WIN; i++) {
        float px = x + i * spacing;
        renderer.drawRect(px, y, size, size, 0.15f, 0.15f, 0.20f);
        if (i < wins) {
            renderer.drawRect(px, y, size, size, 1.0f, 0.8f, 0.2f);
        }
        renderer.drawRectOutline(px, y, size, size, 1.0f, 1.0f, 1.0f);
    }
}

void UI::drawHUD(const Fighter& player, const Enemy& enemy,
                 int roundNumber, int playerWins, int enemyWins) const {
    const float worldW = (float)Config::WORLD_WIDTH;
    const float worldH = (float)Config::WORLD_HEIGHT;

    // debug labels: what is each side doing right now?
    char label[64];
    std::snprintf(label, sizeof(label), "State: %s", player.getStateName());
    renderer.drawText(10.0f, worldH - 25.0f, label, 1.0f, 1.0f, 1.0f);

    std::snprintf(label, sizeof(label), "AI: %s (%s)",
                  enemy.getAIStateName(), enemy.getStateName());
    renderer.drawText(worldW - 310.0f, worldH - 25.0f, label, 1.0f, 1.0f, 1.0f);

    // health bars: player on the left, enemy on the right
    drawHealthBar(10.0f, worldH - 75.0f, 300.0f, 20.0f, player, "PLAYER");
    drawHealthBar(worldW - 310.0f, worldH - 75.0f, 300.0f, 20.0f, enemy, "ENEMY");

    // energy bars, just under the health bars
    drawEnergyBar(10.0f, worldH - 92.0f, 300.0f, 10.0f, player);
    drawEnergyBar(worldW - 310.0f, worldH - 92.0f, 300.0f, 10.0f, enemy);

    // rounds won so far, under the energy bars
    drawRoundPips(10.0f, worldH - 114.0f, playerWins);
    drawRoundPips(worldW - 310.0f, worldH - 114.0f, enemyWins);

    // which round is this?
    std::snprintf(label, sizeof(label), "ROUND %d", roundNumber);
    renderer.drawTextCentered(worldW / 2.0f, worldH - 52.0f, label, 1.0f, 1.0f, 1.0f, false);

    // controls reminder, on the ground strip
    renderer.drawText(10.0f, 62.0f, "A/D move  W/Space jump  S crouch  J punch  K kick  L special  I block",
                      1.0f, 1.0f, 1.0f);
    renderer.drawText(10.0f, 36.0f, "P / ESC: pause   H: debug boxes   T: refill health and energy (test)",
                      1.0f, 1.0f, 0.6f);
    renderer.drawText(10.0f, 10.0f, "Kicks can't be blocked: jump! Bodies are solid: jump over to cross.",
                      1.0f, 0.8f, 0.4f);
}

void UI::drawMenu() const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.70f);   // dim the arena behind the menu

    renderer.drawTextCentered(centerX, 440.0f, "SHADOW FIGHT", 1.0f, 1.0f, 1.0f, true);
    renderer.drawTextCentered(centerX, 395.0f, "Press ENTER to start", 1.0f, 0.85f, 0.3f, false);

    renderer.drawTextCentered(centerX, 330.0f, "A / D   move", 0.8f, 0.8f, 0.8f, false);
    renderer.drawTextCentered(centerX, 305.0f, "W or SPACE   jump", 0.8f, 0.8f, 0.8f, false);
    renderer.drawTextCentered(centerX, 280.0f, "S   crouch", 0.8f, 0.8f, 0.8f, false);
    renderer.drawTextCentered(centerX, 255.0f, "J   punch      K   kick (can't be blocked)", 0.8f, 0.8f, 0.8f, false);
    renderer.drawTextCentered(centerX, 230.0f, "L   special (costs 50 energy)", 0.8f, 0.8f, 0.8f, false);
    renderer.drawTextCentered(centerX, 205.0f, "I   block (hold)", 0.8f, 0.8f, 0.8f, false);
    renderer.drawTextCentered(centerX, 180.0f, "P or ESC   pause", 0.8f, 0.8f, 0.8f, false);

    renderer.drawTextCentered(centerX, 140.0f, "Best of 3 rounds: win 2 to win the match", 1.0f, 0.85f, 0.3f, false);
    renderer.drawTextCentered(centerX, 105.0f, "ESC   quit", 0.6f, 0.6f, 0.6f, false);
}

void UI::drawPauseScreen() const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.55f);

    renderer.drawTextCentered(centerX, 360.0f, "PAUSED", 1.0f, 1.0f, 1.0f, true);
    renderer.drawTextCentered(centerX, 315.0f, "P or ESC   resume", 1.0f, 0.85f, 0.3f, false);
    renderer.drawTextCentered(centerX, 285.0f, "R   restart the match", 0.8f, 0.8f, 0.8f, false);
    renderer.drawTextCentered(centerX, 260.0f, "M   main menu", 0.8f, 0.8f, 0.8f, false);
}

void UI::drawRoundIntro(int roundNumber, bool showFight) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    if (showFight) {
        renderer.drawTextCentered(centerX, 360.0f, "FIGHT!", 1.0f, 0.4f, 0.2f, true);
    } else {
        char text[32];
        std::snprintf(text, sizeof(text), "ROUND %d", roundNumber);
        renderer.drawTextCentered(centerX, 360.0f, text, 1.0f, 1.0f, 1.0f, true);
    }
}

void UI::drawRoundOver(const char* detail) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.25f);
    renderer.drawTextCentered(centerX, 380.0f, "K.O.", 1.0f, 0.2f, 0.2f, true);
    renderer.drawTextCentered(centerX, 340.0f, detail, 1.0f, 1.0f, 1.0f, false);
}

void UI::drawGameOver(bool playerWon, int playerWins, int enemyWins) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.65f);

    if (playerWon) {
        renderer.drawTextCentered(centerX, 400.0f, "YOU WIN!", 1.0f, 0.85f, 0.3f, true);
    } else {
        renderer.drawTextCentered(centerX, 400.0f, "YOU LOSE", 1.0f, 0.3f, 0.3f, true);
    }

    char score[64];
    std::snprintf(score, sizeof(score), "Rounds won:   %d  -  %d", playerWins, enemyWins);
    renderer.drawTextCentered(centerX, 350.0f, score, 1.0f, 1.0f, 1.0f, false);

    renderer.drawTextCentered(centerX, 300.0f, "ENTER   play again", 1.0f, 0.85f, 0.3f, false);
    renderer.drawTextCentered(centerX, 270.0f, "M or ESC   main menu", 0.8f, 0.8f, 0.8f, false);
}
