#include "UI.h"
#include "Fighter.h"
#include "Enemy.h"
#include "Config.h"
#include <cstdio>
#include <cmath>

UI::UI(const Renderer& rendererToUse)
    : renderer(rendererToUse) {
}

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

void UI::drawButton(const RectButton& btn, bool hovered, bool active, const char* customLabel) const {
    const char* textToDraw = (customLabel != nullptr) ? customLabel : btn.label;

    // Drop shadow
    renderer.drawRectAlpha(btn.x + 3.0f, btn.y - 3.0f, btn.width, btn.height,
                           0.0f, 0.0f, 0.0f, 0.45f);

    // Plate background
    if (active) {
        // Selected / Active state: rich warm gold/bronze
        renderer.drawRectAlpha(btn.x, btn.y, btn.width, btn.height,
                               0.28f, 0.22f, 0.08f, 0.95f);
        renderer.drawRectOutline(btn.x, btn.y, btn.width, btn.height,
                                 1.0f, 0.85f, 0.25f);
        renderer.drawRectOutline(btn.x + 2.0f, btn.y + 2.0f, btn.width - 4.0f, btn.height - 4.0f,
                                 0.8f, 0.65f, 0.15f);
    } else if (hovered) {
        // Hover state: glowing ember obsidian
        renderer.drawRectAlpha(btn.x, btn.y, btn.width, btn.height,
                               0.20f, 0.14f, 0.12f, 0.95f);
        renderer.drawRectOutline(btn.x, btn.y, btn.width, btn.height,
                                 1.0f, 0.75f, 0.25f);
    } else {
        // Default state: dark ninja obsidian slate
        renderer.drawRectAlpha(btn.x, btn.y, btn.width, btn.height,
                               0.09f, 0.09f, 0.12f, 0.88f);
        renderer.drawRectOutline(btn.x, btn.y, btn.width, btn.height,
                                 0.42f, 0.42f, 0.48f);
    }

    // Text formatting
    float textY = btn.y + btn.height * 0.5f - 6.0f;
    float centerX = btn.x + btn.width * 0.5f;

    if (active) {
        char formatted[64];
        std::snprintf(formatted, sizeof(formatted), "[ %s ]", textToDraw);
        renderer.drawTextCentered(centerX, textY, formatted, 1.0f, 0.90f, 0.35f, false);
    } else if (hovered) {
        char formatted[64];
        std::snprintf(formatted, sizeof(formatted), ">  %s  <", textToDraw);
        renderer.drawTextCentered(centerX, textY, formatted, 1.0f, 0.92f, 0.40f, false);
    } else {
        renderer.drawTextCentered(centerX, textY, textToDraw, 0.85f, 0.85f, 0.90f, false);
    }
}

void UI::drawSettingsIcon(bool hovered) const {
    const RectButton& btn = UIButtons::BTN_SETTINGS_ICON;

    // Backing plate
    renderer.drawRectAlpha(btn.x + 2.0f, btn.y - 2.0f, btn.width, btn.height,
                           0.0f, 0.0f, 0.0f, 0.45f);
    renderer.drawRectAlpha(btn.x, btn.y, btn.width, btn.height,
                           0.10f, 0.10f, 0.14f, hovered ? 0.95f : 0.75f);
    renderer.drawRectOutline(btn.x, btn.y, btn.width, btn.height,
                             hovered ? 1.0f : 0.50f,
                             hovered ? 0.85f : 0.50f,
                             hovered ? 0.30f : 0.55f);

    float cx = btn.x + btn.width * 0.5f;
    float cy = btn.y + btn.height * 0.5f;

    float iconR = hovered ? 1.0f : 0.82f;
    float iconG = hovered ? 0.85f : 0.82f;
    float iconB = hovered ? 0.30f : 0.88f;

    // Stylized Gear Wheel ⚙️
    for (int i = 0; i < 8; i++) {
        float angle = i * (3.14159265f / 4.0f);
        float tx = cx + std::cos(angle) * 11.0f;
        float ty = cy + std::sin(angle) * 11.0f;
        renderer.drawCircle(tx, ty, 2.5f, iconR, iconG, iconB, 1.0f, 8);
    }
    renderer.drawCircle(cx, cy, 8.5f, iconR, iconG, iconB, 1.0f, 16);
    renderer.drawCircle(cx, cy, 3.8f, 0.10f, 0.10f, 0.14f, 1.0f, 12);
}

void UI::drawHealthBar(float x, float y, float width, float height,
                       const Fighter& fighter, const char* name) const {
    float fraction = (float)fighter.getHealth() / (float)fighter.getMaxHealth();

    renderer.drawRectAlpha(x + 2.0f, y - 2.0f, width, height, 0.0f, 0.0f, 0.0f, 0.5f);
    renderer.drawRect(x, y, width, height, 0.28f, 0.06f, 0.06f);
    renderer.drawRect(x, y, width * fraction, height, 0.20f, 0.82f, 0.25f);
    renderer.drawRectOutline(x, y, width, height, 0.9f, 0.9f, 0.95f);

    char label[64];
    std::snprintf(label, sizeof(label), "%s   %d / %d",
                  name, fighter.getHealth(), fighter.getMaxHealth());
    renderer.drawText(x, y + height + 6.0f, label, 1.0f, 1.0f, 1.0f);
}

void UI::drawEnergyBar(float x, float y, float width, float height,
                       const Fighter& fighter) const {
    float fraction = fighter.getEnergy() / fighter.getMaxEnergy();
    bool specialReady = fighter.getEnergy() >= Attacks::SPECIAL.energyCost;

    renderer.drawRect(x, y, width, height, 0.05f, 0.10f, 0.25f);
    if (specialReady) {
        renderer.drawRect(x, y, width * fraction, height, 1.0f, 0.82f, 0.20f);
    } else {
        renderer.drawRect(x, y, width * fraction, height, 0.25f, 0.65f, 1.0f);
    }
    renderer.drawRectOutline(x, y, width, height, 0.85f, 0.85f, 0.90f);
}

void UI::drawRoundPips(float x, float y, int wins) const {
    const float size = 14.0f;
    const float spacing = 20.0f;

    for (int i = 0; i < Config::ROUNDS_TO_WIN; i++) {
        float px = x + i * spacing;
        renderer.drawRect(px, y, size, size, 0.15f, 0.15f, 0.20f);
        if (i < wins) {
            renderer.drawRect(px, y, size, size, 1.0f, 0.85f, 0.25f);
        }
        renderer.drawRectOutline(px, y, size, size, 0.85f, 0.85f, 0.90f);
    }
}

void UI::drawHUD(const Fighter& player, const Enemy& enemy,
                 int roundNumber, int playerWins, int enemyWins,
                 float mouseX, float mouseY) const {
    const float worldW = (float)Config::WORLD_WIDTH;
    const float worldH = (float)Config::WORLD_HEIGHT;

    drawHealthBar(15.0f, worldH - 55.0f, 300.0f, 20.0f, player, "SHADOW (YOU)");
    drawHealthBar(worldW - 315.0f, worldH - 55.0f, 300.0f, 20.0f, enemy, "WARRIOR (ENEMY)");

    drawEnergyBar(15.0f, worldH - 72.0f, 300.0f, 10.0f, player);
    drawEnergyBar(worldW - 315.0f, worldH - 72.0f, 300.0f, 10.0f, enemy);

    drawRoundPips(15.0f, worldH - 94.0f, playerWins);
    drawRoundPips(worldW - 315.0f, worldH - 94.0f, enemyWins);

    char label[64];
    std::snprintf(label, sizeof(label), "ROUND %d", roundNumber);
    renderer.drawTextCentered(worldW / 2.0f, worldH - 42.0f, label, 1.0f, 0.90f, 0.35f, false);

    bool iconHovered = UIButtons::BTN_SETTINGS_ICON.contains(mouseX, mouseY);
    drawSettingsIcon(iconHovered);
}

void UI::drawMainMenu(float mouseX, float mouseY, int selectedIndex) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.65f);

    renderer.drawTextCentered(centerX, 480.0f, "SHADOW FIGHT", 1.0f, 1.0f, 1.0f, true);
    renderer.drawTextCentered(centerX, 440.0f, "- THE SHADOW CLONE -", 1.0f, 0.85f, 0.35f, false);

    renderer.drawLine(centerX - 180.0f, 415.0f, centerX + 180.0f, 415.0f,
                      0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    bool h0 = UIButtons::BTN_MAIN_START.contains(mouseX, mouseY)   || (selectedIndex == 0);
    bool h1 = UIButtons::BTN_MAIN_OPTIONS.contains(mouseX, mouseY) || (selectedIndex == 1);
    bool h2 = UIButtons::BTN_MAIN_EXIT.contains(mouseX, mouseY)    || (selectedIndex == 2);

    drawButton(UIButtons::BTN_MAIN_START, h0);
    drawButton(UIButtons::BTN_MAIN_OPTIONS, h1);
    drawButton(UIButtons::BTN_MAIN_EXIT, h2);

    renderer.drawTextCentered(centerX, 120.0f, "Select with Mouse or use Arrow Keys + ENTER",
                              0.60f, 0.60f, 0.65f, false);
}

void UI::drawOptionsMenu(float mouseX, float mouseY, int selectedIndex, GameMode currentMode) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.70f);

    renderer.drawTextCentered(centerX, 460.0f, "OPTIONS", 1.0f, 1.0f, 1.0f, true);
    renderer.drawLine(centerX - 140.0f, 415.0f, centerX + 140.0f, 415.0f,
                      0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    bool h0 = UIButtons::BTN_OPT_CONTROLS.contains(mouseX, mouseY) || (selectedIndex == 0);
    bool h1 = UIButtons::BTN_OPT_MODE.contains(mouseX, mouseY)     || (selectedIndex == 1);
    bool h2 = UIButtons::BTN_OPT_BACK.contains(mouseX, mouseY)     || (selectedIndex == 2);

    drawButton(UIButtons::BTN_OPT_CONTROLS, h0);

    const char* modeStr = "GAME MODE : NORMAL";
    if (currentMode == GameMode::EASY) modeStr = "GAME MODE : EASY";
    else if (currentMode == GameMode::HARD) modeStr = "GAME MODE : HARD";

    drawButton(UIButtons::BTN_OPT_MODE, h1, false, modeStr);
    drawButton(UIButtons::BTN_OPT_BACK, h2);
}

void UI::drawControlsScreen(float mouseX, float mouseY, int selectedIndex) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.75f);

    renderer.drawTextCentered(centerX, 515.0f, "GAME CONTROLS", 1.0f, 1.0f, 1.0f, true);
    renderer.drawLine(centerX - 160.0f, 480.0f, centerX + 160.0f, 480.0f,
                      0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    float cardX = 100.0f;
    float cardY = 135.0f;
    float cardW = 600.0f;
    float cardH = 325.0f;

    renderer.drawRectAlpha(cardX + 4.0f, cardY - 4.0f, cardW, cardH, 0.0f, 0.0f, 0.0f, 0.45f);
    renderer.drawRectAlpha(cardX, cardY, cardW, cardH, 0.08f, 0.08f, 0.12f, 0.90f);
    renderer.drawRectOutline(cardX, cardY, cardW, cardH, 0.45f, 0.45f, 0.52f);

    float startY = 425.0f;
    float stepY  = 35.0f;

    auto drawRow = [&](int row, const char* key, const char* desc, float kr, float kg, float kb) {
        float ry = startY - row * stepY;
        renderer.drawText(cardX + 35.0f, ry, key, kr, kg, kb);
        renderer.drawText(cardX + 220.0f, ry, desc, 0.92f, 0.92f, 0.95f);
    };

    drawRow(0, "A  /  D", "Move Left and Right", 1.0f, 0.85f, 0.35f);
    drawRow(1, "W  or  SPACE", "Jump (Leap into air / cross over)", 1.0f, 0.85f, 0.35f);
    drawRow(2, "S", "Crouch (Duck under high attacks)", 1.0f, 0.85f, 0.35f);
    drawRow(3, "J", "Punch / Martial Strike (Fast speed)", 0.35f, 0.85f, 1.0f);
    drawRow(4, "K", "Kick (Unblockable strike! Jump to evade)", 1.0f, 0.55f, 0.25f);
    drawRow(5, "L", "Special Dragon Strike (Costs 50 Energy)", 1.0f, 0.85f, 0.20f);
    drawRow(6, "I", "Block Guard (Hold to absorb incoming damage)", 0.50f, 0.80f, 1.0f);
    drawRow(7, "P  or  ESC", "Pause Game / In-Fight Settings Menu", 0.80f, 0.80f, 0.80f);

    bool hBack = UIButtons::BTN_CTRL_BACK.contains(mouseX, mouseY) || (selectedIndex == 0);
    drawButton(UIButtons::BTN_CTRL_BACK, hBack);
}

void UI::drawGameModeScreen(float mouseX, float mouseY, int selectedIndex, GameMode displayedMode, bool isStartGame) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.75f);

    if (isStartGame) {
        renderer.drawTextCentered(centerX, 515.0f, "START GAME", 1.0f, 1.0f, 1.0f, true);
        renderer.drawTextCentered(centerX, 470.0f, "Select difficulty to start your martial fight",
                                  1.0f, 0.85f, 0.35f, false);
    } else {
        renderer.drawTextCentered(centerX, 515.0f, "GAME MODE DIFFICULTY", 1.0f, 1.0f, 1.0f, true);
        renderer.drawTextCentered(centerX, 470.0f, "Select enemy combat intensity and reflexes",
                                  0.85f, 0.85f, 0.90f, false);
    }
    renderer.drawLine(centerX - 180.0f, 440.0f, centerX + 180.0f, 440.0f,
                      0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    bool h0 = UIButtons::BTN_DIFF_EASY.contains(mouseX, mouseY)   || (selectedIndex == 0);
    bool h1 = UIButtons::BTN_DIFF_NORMAL.contains(mouseX, mouseY) || (selectedIndex == 1);
    bool h2 = UIButtons::BTN_DIFF_HARD.contains(mouseX, mouseY)   || (selectedIndex == 2);

    drawButton(UIButtons::BTN_DIFF_EASY, h0, displayedMode == GameMode::EASY);
    drawButton(UIButtons::BTN_DIFF_NORMAL, h1, displayedMode == GameMode::NORMAL);
    drawButton(UIButtons::BTN_DIFF_HARD, h2, displayedMode == GameMode::HARD);

    // Description info box
    float boxX = 80.0f;
    float boxY = 155.0f;
    float boxW = 640.0f;
    float boxH = 140.0f;

    renderer.drawRectAlpha(boxX + 4.0f, boxY - 4.0f, boxW, boxH, 0.0f, 0.0f, 0.0f, 0.45f);
    renderer.drawRectAlpha(boxX, boxY, boxW, boxH, 0.08f, 0.08f, 0.12f, 0.92f);
    renderer.drawRectOutline(boxX, boxY, boxW, boxH, 0.45f, 0.45f, 0.52f);

    if (displayedMode == GameMode::EASY) {
        renderer.drawText(boxX + 25.0f, boxY + 105.0f, "MODE: EASY (RELAXED COMBAT)", 0.35f, 0.95f, 0.45f);
        renderer.drawText(boxX + 25.0f, boxY + 75.0f,  "* Enemy deals 35% less damage (0.65x multiplier).", 0.90f, 0.90f, 0.95f);
        renderer.drawText(boxX + 25.0f, boxY + 50.0f,  "* Slower reaction time when attacked; enemy rarely blocks.", 0.90f, 0.90f, 0.95f);
        renderer.drawText(boxX + 25.0f, boxY + 25.0f,  "* Generous recovery windows between attacks. Ideal for beginners.", 0.80f, 0.80f, 0.85f);
    } else if (displayedMode == GameMode::NORMAL) {
        renderer.drawText(boxX + 25.0f, boxY + 105.0f, "MODE: NORMAL (STANDARD BALANCE)", 1.0f, 0.85f, 0.35f);
        renderer.drawText(boxX + 25.0f, boxY + 75.0f,  "* Enemy deals full standard damage (1.0x multiplier).", 0.90f, 0.90f, 0.95f);
        renderer.drawText(boxX + 25.0f, boxY + 50.0f,  "* Balanced reaction speed with tactical blocks and dodges.", 0.90f, 0.90f, 0.95f);
        renderer.drawText(boxX + 25.0f, boxY + 25.0f,  "* Authentic, challenging martial arts experience.", 0.80f, 0.80f, 0.85f);
    } else {
        renderer.drawText(boxX + 25.0f, boxY + 105.0f, "MODE: HARD (MASTER TRIAL)", 1.0f, 0.35f, 0.25f);
        renderer.drawText(boxX + 25.0f, boxY + 75.0f,  "* Enemy deals 50% more damage (1.5x multiplier)!", 0.90f, 0.90f, 0.95f);
        renderer.drawText(boxX + 25.0f, boxY + 50.0f,  "* Lightning-fast reflexes; punishes attacks with rapid counter-strikes.", 0.90f, 0.90f, 0.95f);
        renderer.drawText(boxX + 25.0f, boxY + 25.0f,  "* Relentless spacing pressure and frequent guard defense.", 0.80f, 0.80f, 0.85f);
    }

    // Both BACK and OK buttons placed beside each other as requested
    bool hBack = UIButtons::BTN_DIFF_BACK.contains(mouseX, mouseY) || (selectedIndex == 3);
    bool hOk   = UIButtons::BTN_DIFF_OK.contains(mouseX, mouseY)   || (selectedIndex == 4);

    drawButton(UIButtons::BTN_DIFF_BACK, hBack);
    drawButton(UIButtons::BTN_DIFF_OK, hOk, false, isStartGame ? "START FIGHT" : "OK");
}

void UI::drawAlertDialog(const char* title, const char* line1, const char* line2,
                         float mouseX, float mouseY, const char* okLabel) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    // Dark backdrop overlay for modal alert
    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.72f);

    float boxX = 120.0f;
    float boxY = 180.0f;
    float boxW = 560.0f;
    float boxH = 230.0f;

    renderer.drawRectAlpha(boxX + 6.0f, boxY - 6.0f, boxW, boxH, 0.0f, 0.0f, 0.0f, 0.70f);
    renderer.drawRectAlpha(boxX, boxY, boxW, boxH, 0.12f, 0.10f, 0.14f, 0.98f);
    renderer.drawRectOutline(boxX, boxY, boxW, boxH, 1.0f, 0.80f, 0.30f);
    renderer.drawRectOutline(boxX + 2.0f, boxY + 2.0f, boxW - 4.0f, boxH - 4.0f, 0.65f, 0.45f, 0.15f);

    renderer.drawTextCentered(centerX, boxY + 175.0f, title ? title : "! NOTICE !", 1.0f, 0.85f, 0.35f, true);
    renderer.drawLine(centerX - 120.0f, boxY + 160.0f, centerX + 120.0f, boxY + 160.0f,
                      0.85f, 0.70f, 0.25f, 0.8f, 1.5f);

    if (line1) {
        renderer.drawTextCentered(centerX, boxY + 122.0f, line1, 1.0f, 0.95f, 0.95f, false);
    }
    if (line2) {
        renderer.drawTextCentered(centerX, boxY + 92.0f, line2, 0.80f, 0.80f, 0.85f, false);
    }

    bool hOk = UIButtons::BTN_ALERT_OK.contains(mouseX, mouseY);
    drawButton(UIButtons::BTN_ALERT_OK, hOk, false, okLabel ? okLabel : "OK");
}

void UI::drawConfirmDialog(const char* title, const char* line1, const char* line2,
                           float mouseX, float mouseY,
                           const char* yesLabel, const char* noLabel) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    // Dark backdrop overlay for modal alert
    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.75f);

    float boxX = 110.0f;
    float boxY = 175.0f;
    float boxW = 580.0f;
    float boxH = 240.0f;

    renderer.drawRectAlpha(boxX + 6.0f, boxY - 6.0f, boxW, boxH, 0.0f, 0.0f, 0.0f, 0.70f);
    renderer.drawRectAlpha(boxX, boxY, boxW, boxH, 0.14f, 0.09f, 0.10f, 0.98f);
    renderer.drawRectOutline(boxX, boxY, boxW, boxH, 1.0f, 0.40f, 0.30f);
    renderer.drawRectOutline(boxX + 2.0f, boxY + 2.0f, boxW - 4.0f, boxH - 4.0f, 0.75f, 0.25f, 0.20f);

    renderer.drawTextCentered(centerX, boxY + 185.0f, title ? title : "! NOTICE !", 1.0f, 0.45f, 0.30f, true);
    renderer.drawLine(centerX - 130.0f, boxY + 170.0f, centerX + 130.0f, boxY + 170.0f,
                      1.0f, 0.40f, 0.25f, 0.8f, 1.5f);

    if (line1) {
        renderer.drawTextCentered(centerX, boxY + 132.0f, line1, 1.0f, 0.95f, 0.95f, false);
    }
    if (line2) {
        renderer.drawTextCentered(centerX, boxY + 100.0f, line2, 1.0f, 0.85f, 0.35f, false);
    }

    bool hYes = UIButtons::BTN_CONFIRM_YES.contains(mouseX, mouseY);
    bool hNo  = UIButtons::BTN_CONFIRM_NO.contains(mouseX, mouseY);

    drawButton(UIButtons::BTN_CONFIRM_YES, hYes, false, yesLabel ? yesLabel : "YES");
    drawButton(UIButtons::BTN_CONFIRM_NO, hNo, false, noLabel ? noLabel : "NO");
}

void UI::drawPauseMenu(float mouseX, float mouseY, int selectedIndex) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.70f);

    renderer.drawTextCentered(centerX, 460.0f, "PAUSED", 1.0f, 1.0f, 1.0f, true);
    renderer.drawLine(centerX - 120.0f, 415.0f, centerX + 120.0f, 415.0f,
                      0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    bool h0 = UIButtons::BTN_PAUSE_RESUME.contains(mouseX, mouseY)  || (selectedIndex == 0);
    bool h1 = UIButtons::BTN_PAUSE_RESTART.contains(mouseX, mouseY) || (selectedIndex == 1);
    bool h2 = UIButtons::BTN_PAUSE_OPTIONS.contains(mouseX, mouseY) || (selectedIndex == 2);
    bool h3 = UIButtons::BTN_PAUSE_EXIT.contains(mouseX, mouseY)    || (selectedIndex == 3);

    drawButton(UIButtons::BTN_PAUSE_RESUME, h0);
    drawButton(UIButtons::BTN_PAUSE_RESTART, h1);
    drawButton(UIButtons::BTN_PAUSE_OPTIONS, h2);
    drawButton(UIButtons::BTN_PAUSE_EXIT, h3);

    renderer.drawTextCentered(centerX, 120.0f, "P / ESC to Resume  -  Exit returns to Home Screen",
                              0.60f, 0.60f, 0.65f, false);
}

void UI::drawRoundIntro(int roundNumber, bool showFight, const char* stageInfo) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    if (showFight) {
        renderer.drawTextCentered(centerX, 360.0f, "FIGHT!", 1.0f, 0.40f, 0.20f, true);
    } else {
        char text[32];
        std::snprintf(text, sizeof(text), "ROUND %d", roundNumber);
        renderer.drawTextCentered(centerX, 370.0f, text, 1.0f, 1.0f, 1.0f, true);
        if (stageInfo != nullptr && stageInfo[0] != '\0') {
            renderer.drawTextCentered(centerX, 330.0f, stageInfo, 1.0f, 0.85f, 0.35f, false);
        }
    }
}

void UI::drawRoundOver(const char* detail) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.25f);
    renderer.drawTextCentered(centerX, 380.0f, "K.O.", 1.0f, 0.20f, 0.20f, true);
    renderer.drawTextCentered(centerX, 340.0f, detail, 1.0f, 1.0f, 1.0f, false);
}

void UI::drawGameOver(bool playerWon, int playerWins, int enemyWins) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.65f);

    if (playerWon) {
        renderer.drawTextCentered(centerX, 420.0f, "VICTORY!", 1.0f, 0.85f, 0.30f, true);
        renderer.drawTextCentered(centerX, 380.0f, "You proved your shadow supremacy", 0.90f, 0.90f, 0.95f, false);
    } else {
        renderer.drawTextCentered(centerX, 420.0f, "DEFEATED", 1.0f, 0.30f, 0.30f, true);
        renderer.drawTextCentered(centerX, 380.0f, "The imperial warrior stands victorious", 0.85f, 0.85f, 0.90f, false);
    }

    char score[64];
    std::snprintf(score, sizeof(score), "Final Match Score:   %d  -  %d", playerWins, enemyWins);
    renderer.drawTextCentered(centerX, 330.0f, score, 1.0f, 1.0f, 1.0f, false);

    renderer.drawTextCentered(centerX, 270.0f, "ENTER : Rematch     ESC / M : Home Screen", 1.0f, 0.85f, 0.35f, false);
}
