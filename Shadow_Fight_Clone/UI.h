#ifndef UI_H
#define UI_H

#include "Renderer.h"
#include "Config.h"

class Fighter;
class Enemy;

struct RectButton {
    float x;
    float y;
    float width;
    float height;
    const char* label;

    bool contains(float mx, float my) const {
        return mx >= x && mx <= x + width && my >= y && my <= y + height;
    }
};

namespace UIButtons {
    // Main menu buttons
    const RectButton BTN_MAIN_START    = { 260.0f, 320.0f, 280.0f, 48.0f, "START GAME" };
    const RectButton BTN_MAIN_OPTIONS  = { 260.0f, 250.0f, 280.0f, 48.0f, "OPTIONS" };
    const RectButton BTN_MAIN_EXIT     = { 260.0f, 180.0f, 280.0f, 48.0f, "EXIT" };

    // Options menu buttons
    const RectButton BTN_OPT_CONTROLS  = { 240.0f, 330.0f, 320.0f, 48.0f, "GAME CONTROLS" };
    const RectButton BTN_OPT_MODE      = { 240.0f, 260.0f, 320.0f, 48.0f, "GAME MODE" };
    const RectButton BTN_OPT_BACK      = { 240.0f, 180.0f, 320.0f, 48.0f, "BACK" };

    // Controls screen back button
    const RectButton BTN_CTRL_BACK     = { 300.0f, 75.0f,  200.0f, 44.0f, "BACK" };

    // Difficulty screen buttons (OK placed beside BACK)
    const RectButton BTN_DIFF_EASY     = { 80.0f,  330.0f, 190.0f, 50.0f, "EASY" };
    const RectButton BTN_DIFF_NORMAL   = { 305.0f, 330.0f, 190.0f, 50.0f, "NORMAL" };
    const RectButton BTN_DIFF_HARD     = { 530.0f, 330.0f, 190.0f, 50.0f, "HARD" };
    const RectButton BTN_DIFF_BACK     = { 190.0f, 80.0f,  190.0f, 46.0f, "BACK" };
    const RectButton BTN_DIFF_OK       = { 420.0f, 80.0f,  190.0f, 46.0f, "OK" };

    // Alert dialog OK button
    const RectButton BTN_ALERT_OK      = { 335.0f, 215.0f, 130.0f, 42.0f, "OK" };

    // Top-right in-game settings gear button
    const RectButton BTN_SETTINGS_ICON = { 745.0f, 545.0f, 45.0f,  42.0f, "SETTINGS" };

    // Pause menu buttons
    const RectButton BTN_PAUSE_RESUME  = { 260.0f, 345.0f, 280.0f, 46.0f, "CONTINUE" };
    const RectButton BTN_PAUSE_RESTART = { 260.0f, 285.0f, 280.0f, 46.0f, "RESTART GAME" };
    const RectButton BTN_PAUSE_OPTIONS = { 260.0f, 225.0f, 280.0f, 46.0f, "OPTIONS" };
    const RectButton BTN_PAUSE_EXIT    = { 260.0f, 165.0f, 280.0f, 46.0f, "EXIT TO MENU" };
}

// Everything the player reads or clicks on screen that isn't the arena scene
class UI {
public:
    explicit UI(const Renderer& renderer);

    void drawDebugBoxes(const Fighter& fighter) const;

    // The in-fight HUD with health, energy, rounds, stage banner, and settings gear icon
    void drawHUD(const Fighter& player, const Enemy& enemy,
                 int roundNumber, int playerWins, int enemyWins,
                 float mouseX = -1.0f, float mouseY = -1.0f) const;

    void drawButton(const RectButton& btn, bool hovered, bool active = false, const char* customLabel = nullptr) const;
    void drawSettingsIcon(bool hovered) const;

    // Menu Screens
    void drawMainMenu(float mouseX, float mouseY, int selectedIndex = -1) const;
    void drawOptionsMenu(float mouseX, float mouseY, int selectedIndex, GameMode currentMode) const;
    void drawControlsScreen(float mouseX, float mouseY, int selectedIndex = -1) const;
    void drawGameModeScreen(float mouseX, float mouseY, int selectedIndex, GameMode displayedMode, bool isStartGame = false) const;
    void drawAlertDialog(const char* message, float mouseX, float mouseY) const;

    // Pause Screen
    void drawPauseMenu(float mouseX, float mouseY, int selectedIndex = -1) const;

    void drawRoundIntro(int roundNumber, bool showFight, const char* stageInfo = nullptr) const;
    void drawRoundOver(const char* detail) const;
    void drawGameOver(bool playerWon, int playerWins, int enemyWins) const;

private:
    void drawHealthBar(float x, float y, float width, float height,
                       const Fighter& fighter, const char* name) const;
    void drawEnergyBar(float x, float y, float width, float height,
                       const Fighter& fighter) const;
    void drawRoundPips(float x, float y, int wins) const;

    const Renderer& renderer;
};

#endif
