#ifndef UI_H
#define UI_H

#include "Renderer.h"
#include "Config.h"
#include "CharacterData.h"
#include "StatsManager.h"
#include "InputManager.h"

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
    const RectButton BTN_MAIN_START    = { 260.0f, 340.0f, 280.0f, 46.0f, "START GAME" };
    const RectButton BTN_MAIN_STATS    = { 260.0f, 282.0f, 280.0f, 46.0f, "STATS & RECORDS" };
    const RectButton BTN_MAIN_OPTIONS  = { 260.0f, 224.0f, 280.0f, 46.0f, "OPTIONS" };
    const RectButton BTN_MAIN_EXIT     = { 260.0f, 166.0f, 280.0f, 46.0f, "EXIT" };

    // Stats screen buttons
    const RectButton BTN_STATS_BACK    = { 190.0f, 34.0f,  190.0f, 44.0f, "BACK" };
    const RectButton BTN_STATS_RESET   = { 420.0f, 34.0f,  190.0f, 44.0f, "RESET STATS" };

    // Options menu buttons
    const RectButton BTN_OPT_CONTROLS  = { 240.0f, 330.0f, 320.0f, 48.0f, "GAME CONTROLS" };
    const RectButton BTN_OPT_MODE      = { 240.0f, 260.0f, 320.0f, 48.0f, "GAME MODE" };
    const RectButton BTN_OPT_BACK      = { 240.0f, 180.0f, 320.0f, 48.0f, "BACK" };

    // Controls screen buttons & row geometry
    const RectButton BTN_CTRL_TAB_KB   = { 215.0f, 502.0f, 175.0f, 30.0f, "KEYBOARD" };
    const RectButton BTN_CTRL_TAB_PAD  = { 410.0f, 502.0f, 175.0f, 30.0f, "GAMEPAD" };
    const RectButton BTN_CTRL_BACK     = { 175.0f, 44.0f,  195.0f, 44.0f, "BACK" };
    const RectButton BTN_CTRL_RESET    = { 410.0f, 44.0f,  220.0f, 44.0f, "RESET DEFAULTS" };

    inline RectButton controlRowButton(int actionIdx) {
        float rowY = 442.0f - actionIdx * 29.0f;
        return { 62.0f, rowY - 6.0f, 676.0f, 26.0f, "" };
    }

    // Difficulty screen buttons (OK placed beside BACK)
    const RectButton BTN_DIFF_EASY     = { 80.0f,  330.0f, 190.0f, 50.0f, "EASY" };
    const RectButton BTN_DIFF_NORMAL   = { 305.0f, 330.0f, 190.0f, 50.0f, "NORMAL" };
    const RectButton BTN_DIFF_HARD     = { 530.0f, 330.0f, 190.0f, 50.0f, "HARD" };
    const RectButton BTN_DIFF_BACK     = { 190.0f, 80.0f,  190.0f, 46.0f, "BACK" };
    const RectButton BTN_DIFF_OK       = { 420.0f, 80.0f,  190.0f, 46.0f, "OK" };

    // Character select screen buttons (5 player left, 5 opponent right, back/continue bottom)
    const RectButton BTN_CHAR_PLAYER[5] = {
        { 45.0f, 442.0f, 330.0f, 32.0f, "KAI" },
        { 45.0f, 406.0f, 330.0f, 32.0f, "KURO" },
        { 45.0f, 370.0f, 330.0f, 32.0f, "AKARI" },
        { 45.0f, 334.0f, 330.0f, 32.0f, "MEI-LING" },
        { 45.0f, 298.0f, 330.0f, 32.0f, "TAKEDA" }
    };
    const RectButton BTN_CHAR_OPPONENT[5] = {
        { 425.0f, 442.0f, 330.0f, 32.0f, "KAI" },
        { 425.0f, 406.0f, 330.0f, 32.0f, "KURO" },
        { 425.0f, 370.0f, 330.0f, 32.0f, "AKARI" },
        { 425.0f, 334.0f, 330.0f, 32.0f, "MEI-LING" },
        { 425.0f, 298.0f, 330.0f, 32.0f, "TAKEDA" }
    };
    const RectButton BTN_CHAR_BACK = { 190.0f, 24.0f, 190.0f, 44.0f, "BACK" };
    const RectButton BTN_CHAR_NEXT = { 420.0f, 24.0f, 190.0f, 44.0f, "CONTINUE" };

    // Alert dialog OK button
    const RectButton BTN_ALERT_OK      = { 335.0f, 215.0f, 130.0f, 42.0f, "OK" };
    const RectButton BTN_CONFIRM_YES   = { 245.0f, 215.0f, 140.0f, 42.0f, "YES" };
    const RectButton BTN_CONFIRM_NO    = { 415.0f, 215.0f, 140.0f, 42.0f, "NO" };

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

    // The in-fight HUD with health, energy, rounds, live score, and settings gear icon
    void drawHUD(const Fighter& player, const Enemy& enemy,
                 int roundNumber, int playerWins, int enemyWins,
                 float mouseX = -1.0f, float mouseY = -1.0f,
                 int currentScore = 0, int highScore = 0) const;

    void drawButton(const RectButton& btn, bool hovered, bool active = false,
                    const char* customLabel = nullptr, bool disabled = false) const;
    void drawSettingsIcon(bool hovered) const;

    // Menu Screens
    void drawMainMenu(float mouseX, float mouseY, int selectedIndex = -1,
                      const CareerStats* career = nullptr,
                      bool gamepadConnected = false, const char* gamepadName = nullptr) const;
    void drawStatsScreen(float mouseX, float mouseY, const CareerStats& career,
                         bool confirmReset = false) const;
    void drawCharacterSelect(float mouseX, float mouseY,
                             CharacterId playerChar, CharacterId enemyChar,
                             bool gamepadMode = false, int gamepadColumn = 0) const;
    void drawOptionsMenu(float mouseX, float mouseY, int selectedIndex, GameMode currentMode) const;
    void drawControlsScreen(float mouseX, float mouseY, int selectedIndex = -1,
                            const InputManager* input = nullptr,
                            bool showGamepadTab = false) const;
    void drawGameModeScreen(float mouseX, float mouseY, int selectedIndex, GameMode displayedMode, bool isStartGame = false) const;
    void drawAlertDialog(const char* title, const char* line1, const char* line2,
                         float mouseX, float mouseY, const char* okLabel = "OK") const;
    void drawConfirmDialog(const char* title, const char* line1, const char* line2,
                           float mouseX, float mouseY,
                           const char* yesLabel = "YES", const char* noLabel = "NO") const;

    // Pause Screen
    void drawPauseMenu(float mouseX, float mouseY, int selectedIndex = -1) const;

    void drawRoundIntro(int roundNumber, bool showFight, const char* stageInfo = nullptr) const;
    void drawRoundOver(const char* detail) const;
    void drawGameOver(bool playerWon, int playerWins, int enemyWins,
                      const MatchStats& match, const CareerStats& career) const;

private:
    void drawHealthBar(float x, float y, float width, float height,
                       const Fighter& fighter, const char* name) const;
    void drawEnergyBar(float x, float y, float width, float height,
                       const Fighter& fighter) const;
    void drawRoundPips(float x, float y, int wins) const;
    void drawCharacterDetailsCard(float x, float y, float width, float height,
                                  const CharacterProfile& profile, const char* roleTag) const;

    const Renderer& renderer;
};

#endif
