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

void UI::drawButton(const RectButton& btn, bool hovered, bool active,
                    const char* customLabel, bool disabled) const {
    const char* textToDraw = (customLabel != nullptr) ? customLabel : btn.label;

    // Drop shadow
    renderer.drawRectAlpha(btn.x + 3.0f, btn.y - 3.0f, btn.width, btn.height,
                           0.0f, 0.0f, 0.0f, 0.45f);

    // Plate background
    if (disabled) {
        renderer.drawRectAlpha(btn.x, btn.y, btn.width, btn.height,
                               0.05f, 0.05f, 0.07f, 0.72f);
        renderer.drawRectOutline(btn.x, btn.y, btn.width, btn.height,
                                 0.24f, 0.24f, 0.28f);
    } else if (active) {
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

    if (disabled) {
        renderer.drawTextCentered(centerX, textY, textToDraw, 0.42f, 0.42f, 0.48f, false);
    } else if (active) {
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
    bool specialReady = fighter.getEnergy() >= fighter.getSpecialEnergyCost();

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
                 float mouseX, float mouseY,
                 int currentScore, int highScore) const {
    const float worldW = (float)Config::WORLD_WIDTH;
    const float worldH = (float)Config::WORLD_HEIGHT;

    char playerLabel[48];
    char enemyLabel[48];
    std::snprintf(playerLabel, sizeof(playerLabel), "%s (YOU)", player.getProfile().name);
    std::snprintf(enemyLabel, sizeof(enemyLabel), "%s (ENEMY)", enemy.getProfile().name);

    drawHealthBar(15.0f, worldH - 55.0f, 300.0f, 20.0f, player, playerLabel);
    drawHealthBar(worldW - 315.0f, worldH - 55.0f, 300.0f, 20.0f, enemy, enemyLabel);

    drawEnergyBar(15.0f, worldH - 72.0f, 300.0f, 10.0f, player);
    drawEnergyBar(worldW - 315.0f, worldH - 72.0f, 300.0f, 10.0f, enemy);

    drawRoundPips(15.0f, worldH - 94.0f, playerWins);
    drawRoundPips(worldW - 315.0f, worldH - 94.0f, enemyWins);

    char label[64];
    std::snprintf(label, sizeof(label), "ROUND %d", roundNumber);
    renderer.drawTextCentered(worldW / 2.0f, worldH - 42.0f, label, 1.0f, 0.90f, 0.35f, false);

    std::snprintf(label, sizeof(label), "SCORE  %d", currentScore);
    renderer.drawTextCentered(worldW / 2.0f, worldH - 64.0f, label, 0.95f, 0.95f, 1.0f, false);

    int displayedBest = (currentScore > highScore) ? currentScore : highScore;
    std::snprintf(label, sizeof(label), "BEST  %d", displayedBest);
    renderer.drawTextSmall(worldW / 2.0f - 34.0f, worldH - 83.0f, label, 0.65f, 0.90f, 0.68f);

    bool iconHovered = UIButtons::BTN_SETTINGS_ICON.contains(mouseX, mouseY);
    drawSettingsIcon(iconHovered);
}

void UI::drawCharacterDetailsCard(float x, float y, float width, float height,
                                  const CharacterProfile& profile, const char* roleTag) const {
    renderer.drawRectAlpha(x + 3.0f, y - 3.0f, width, height, 0.0f, 0.0f, 0.0f, 0.50f);
    renderer.drawRectAlpha(x, y, width, height, 0.08f, 0.08f, 0.12f, 0.94f);
    renderer.drawRectOutline(x, y, width, height, 0.55f, 0.50f, 0.35f);

    // Character accent stripe along the left border of the details card
    renderer.drawRect(x + 2.0f, y + 2.0f, 5.0f, height - 4.0f,
                      profile.accentR, profile.accentG, profile.accentB);

    char header[64];
    std::snprintf(header, sizeof(header), "%s  -  %s", profile.name, profile.title);
    renderer.drawText(x + 14.0f, y + height - 24.0f, header,
                      profile.accentR, profile.accentG, profile.accentB);
    renderer.drawTextSmall(x + 14.0f, y + height - 42.0f, roleTag, 0.72f, 0.72f, 0.78f);

    renderer.drawLine(x + 14.0f, y + height - 50.0f, x + width - 12.0f, y + height - 50.0f,
                      0.55f, 0.45f, 0.25f, 0.65f, 1.0f);

    char lineBuf[128];
    std::snprintf(lineBuf, sizeof(lineBuf), "Weapon: %s", profile.weaponName);
    renderer.drawTextSmall(x + 14.0f, y + height - 66.0f, lineBuf, 0.95f, 0.95f, 0.98f);

    std::snprintf(lineBuf, sizeof(lineBuf), "Style: %s", profile.styleName);
    renderer.drawTextSmall(x + 14.0f, y + height - 84.0f, lineBuf, 0.85f, 0.90f, 1.0f);

    renderer.drawTextSmall(x + 14.0f, y + height - 102.0f, profile.skillDesc, 0.55f, 0.90f, 1.0f);

    std::snprintf(lineBuf, sizeof(lineBuf), "Critical (L): %s", profile.specialName);
    renderer.drawTextSmall(x + 14.0f, y + height - 122.0f, lineBuf, 1.0f, 0.85f, 0.30f);

    renderer.drawTextSmall(x + 14.0f, y + height - 139.0f, profile.specialDesc, 0.82f, 0.82f, 0.88f);

    renderer.drawLine(x + 14.0f, y + 32.0f, x + width - 12.0f, y + 32.0f,
                      0.35f, 0.35f, 0.42f, 0.55f, 1.0f);
    renderer.drawTextSmall(x + 14.0f, y + 13.0f, profile.statsSummary, 0.55f, 0.92f, 0.65f);
}

void UI::drawCharacterSelect(float mouseX, float mouseY,
                             CharacterId playerChar, CharacterId enemyChar) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.80f);

    renderer.drawTextCentered(centerX, 556.0f, "CHOOSE YOUR FIGHTERS", 1.0f, 1.0f, 1.0f, true);
    renderer.drawTextCentered(centerX, 530.0f,
                              "Pick your fighter on the left, then select a different opponent on the right",
                              1.0f, 0.85f, 0.35f, false);
    renderer.drawLine(45.0f, 514.0f, 755.0f, 514.0f, 0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    // Center divider between Your Fighter (left) and Opponent (right)
    renderer.drawLine(centerX, 86.0f, centerX, 504.0f, 0.55f, 0.48f, 0.28f, 0.55f, 1.5f);

    renderer.drawTextCentered(210.0f, 486.0f, "YOUR FIGHTER", 0.35f, 0.90f, 1.0f, false);
    renderer.drawTextCentered(590.0f, 486.0f, "OPPONENT", 1.0f, 0.48f, 0.30f, false);

    for (int i = 0; i < Characters::COUNT; i++) {
        const CharacterProfile& prof = Characters::getByIndex(i);
        char label[64];
        std::snprintf(label, sizeof(label), "%s  -  %s", prof.name, prof.title);

        bool pSelected = (i == (int)playerChar);
        bool pHovered  = UIButtons::BTN_CHAR_PLAYER[i].contains(mouseX, mouseY);
        drawButton(UIButtons::BTN_CHAR_PLAYER[i], pHovered, pSelected, label, false);

        bool eDisabled = (i == (int)playerChar);
        bool eSelected = (i == (int)enemyChar) && !eDisabled;
        bool eHovered  = !eDisabled && UIButtons::BTN_CHAR_OPPONENT[i].contains(mouseX, mouseY);
        if (eDisabled) {
            std::snprintf(label, sizeof(label), "%s  (YOUR FIGHTER)", prof.name);
        }
        drawButton(UIButtons::BTN_CHAR_OPPONENT[i], eHovered, eSelected, label, eDisabled);
    }

    drawCharacterDetailsCard(45.0f,  92.0f, 330.0f, 194.0f,
                             Characters::get(playerChar), "SELECTED AS YOUR FIGHTER");
    drawCharacterDetailsCard(425.0f, 92.0f, 330.0f, 194.0f,
                             Characters::get(enemyChar), "SELECTED AS OPPONENT");

    bool hBack = UIButtons::BTN_CHAR_BACK.contains(mouseX, mouseY);
    bool hNext = UIButtons::BTN_CHAR_NEXT.contains(mouseX, mouseY);
    drawButton(UIButtons::BTN_CHAR_BACK, hBack);
    drawButton(UIButtons::BTN_CHAR_NEXT, hNext);
}

void UI::drawMainMenu(float mouseX, float mouseY, int selectedIndex,
                      const CareerStats* career) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.65f);

    renderer.drawTextCentered(centerX, 495.0f, "SHADOW FIGHT", 1.0f, 1.0f, 1.0f, true);
    renderer.drawTextCentered(centerX, 455.0f, "- THE SHADOW CLONE -", 1.0f, 0.85f, 0.35f, false);

    renderer.drawLine(centerX - 180.0f, 430.0f, centerX + 180.0f, 430.0f,
                      0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    bool h0 = UIButtons::BTN_MAIN_START.contains(mouseX, mouseY)   || (selectedIndex == 0);
    bool h1 = UIButtons::BTN_MAIN_STATS.contains(mouseX, mouseY)   || (selectedIndex == 1);
    bool h2 = UIButtons::BTN_MAIN_OPTIONS.contains(mouseX, mouseY) || (selectedIndex == 2);
    bool h3 = UIButtons::BTN_MAIN_EXIT.contains(mouseX, mouseY)    || (selectedIndex == 3);

    drawButton(UIButtons::BTN_MAIN_START, h0);
    drawButton(UIButtons::BTN_MAIN_STATS, h1);
    drawButton(UIButtons::BTN_MAIN_OPTIONS, h2);
    drawButton(UIButtons::BTN_MAIN_EXIT, h3);

    if (career != nullptr) {
        char banner[96];
        std::snprintf(banner, sizeof(banner), "HIGH SCORE: %d   |   WINS: %d / %d   |   BEST STREAK: %d",
                      career->highScore, career->matchesWon, career->matchesPlayed, career->bestWinStreak);
        renderer.drawTextCentered(centerX, 122.0f, banner, 0.60f, 0.92f, 0.68f, false);
    }

    renderer.drawTextCentered(centerX, 90.0f, "Select with Mouse or Press ENTER to Fight",
                              0.60f, 0.60f, 0.65f, false);
}

void UI::drawStatsScreen(float mouseX, float mouseY, const CareerStats& career,
                         bool confirmReset) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.82f);

    renderer.drawTextCentered(centerX, 554.0f, "CAREER STATS & HIGH SCORES", 1.0f, 1.0f, 1.0f, true);
    renderer.drawLine(40.0f, 526.0f, 760.0f, 526.0f, 0.85f, 0.68f, 0.25f, 0.75f, 1.5f);

    // Left Card: Lifetime Combat Records
    float lx = 36.0f, ly = 98.0f, lw = 360.0f, lh = 412.0f;
    renderer.drawRectAlpha(lx + 4.0f, ly - 4.0f, lw, lh, 0.0f, 0.0f, 0.0f, 0.55f);
    renderer.drawRectAlpha(lx, ly, lw, lh, 0.08f, 0.08f, 0.12f, 0.94f);
    renderer.drawRectOutline(lx, ly, lw, lh, 0.65f, 0.52f, 0.25f);

    renderer.drawText(lx + 18.0f, ly + lh - 30.0f, "LIFETIME RECORDS", 1.0f, 0.85f, 0.30f);
    renderer.drawLine(lx + 16.0f, ly + lh - 40.0f, lx + lw - 16.0f, ly + lh - 40.0f, 0.55f, 0.45f, 0.25f, 0.7f, 1.0f);

    auto drawStatRow = [&](float yPos, const char* label, const char* val,
                           float vr = 0.95f, float vg = 0.95f, float vb = 1.0f) {
        renderer.drawTextSmall(lx + 18.0f, yPos, label, 0.75f, 0.78f, 0.85f);
        renderer.drawTextSmall(lx + 215.0f, yPos, val, vr, vg, vb);
    };

    char buf[64];
    float rowY = ly + lh - 68.0f;
    float step = 28.0f;

    std::snprintf(buf, sizeof(buf), "%d", career.highScore);
    drawStatRow(rowY, "Best Match Score:", buf, 1.0f, 0.88f, 0.28f); rowY -= step;

    std::snprintf(buf, sizeof(buf), "%lld", career.totalScore);
    drawStatRow(rowY, "Cumulative Score:", buf, 0.60f, 0.92f, 1.0f); rowY -= step;

    int winPct = (career.matchesPlayed > 0) ? (career.matchesWon * 100 / career.matchesPlayed) : 0;
    std::snprintf(buf, sizeof(buf), "%dW - %dL  (%d%%)", career.matchesWon, career.matchesLost, winPct);
    drawStatRow(rowY, "Matches Record:", buf, 0.55f, 0.95f, 0.65f); rowY -= step;

    std::snprintf(buf, sizeof(buf), "%d   (Best: %d)", career.currentWinStreak, career.bestWinStreak);
    drawStatRow(rowY, "Win Streak:", buf, 1.0f, 0.80f, 0.35f); rowY -= step;

    std::snprintf(buf, sizeof(buf), "%dW - %dL", career.roundsWon, career.roundsLost);
    drawStatRow(rowY, "Rounds Record:", buf); rowY -= step;

    std::snprintf(buf, sizeof(buf), "%d   (%d Perfect)", career.totalKnockouts, career.perfectRounds);
    drawStatRow(rowY, "Total K.O.s:", buf, 1.0f, 0.55f, 0.35f); rowY -= step;

    if (career.fastestRoundWinSec > 0.0f) {
        std::snprintf(buf, sizeof(buf), "%.2f sec", career.fastestRoundWinSec);
    } else {
        std::snprintf(buf, sizeof(buf), "--");
    }
    drawStatRow(rowY, "Fastest Round Win:", buf, 0.65f, 0.90f, 1.0f); rowY -= step;

    std::snprintf(buf, sizeof(buf), "%d", career.totalHitsLanded);
    drawStatRow(rowY, "Hits Landed:", buf); rowY -= step;

    std::snprintf(buf, sizeof(buf), "%d", career.totalSpecialsLanded);
    drawStatRow(rowY, "Special Hits Landed:", buf, 1.0f, 0.72f, 0.25f); rowY -= step;

    std::snprintf(buf, sizeof(buf), "%d   |   %d Blk", career.totalDodges, career.totalBlocks);
    drawStatRow(rowY, "Roll Dodges / Blocks:", buf, 0.55f, 0.88f, 1.0f); rowY -= step;

    std::snprintf(buf, sizeof(buf), "%lld", career.totalDamageDealt);
    drawStatRow(rowY, "Total Damage Dealt:", buf, 0.95f, 0.55f, 0.50f); rowY -= step;

    std::snprintf(buf, sizeof(buf), "%lld", career.totalDamageTaken);
    drawStatRow(rowY, "Total Damage Taken:", buf, 0.78f, 0.78f, 0.85f);

    // Right Card: Per-Character Fighter Mastery
    float rx = 412.0f, ry = 98.0f, rw = 352.0f, rh = 412.0f;
    renderer.drawRectAlpha(rx + 4.0f, ry - 4.0f, rw, rh, 0.0f, 0.0f, 0.0f, 0.55f);
    renderer.drawRectAlpha(rx, ry, rw, rh, 0.08f, 0.08f, 0.12f, 0.94f);
    renderer.drawRectOutline(rx, ry, rw, rh, 0.65f, 0.52f, 0.25f);

    renderer.drawText(rx + 18.0f, ry + rh - 30.0f, "FIGHTER MASTERY", 0.38f, 0.90f, 1.0f);
    renderer.drawLine(rx + 16.0f, ry + rh - 40.0f, rx + rw - 16.0f, ry + rh - 40.0f, 0.35f, 0.55f, 0.70f, 0.7f, 1.0f);

    float cy = ry + rh - 64.0f;
    for (int i = 0; i < Characters::COUNT; ++i) {
        const CharacterProfile& prof = Characters::getByIndex(i);
        int m = career.charMatches[i];
        int w = career.charWins[i];
        int pct = (m > 0) ? (w * 100 / m) : 0;

        // Character card box
        float boxH = 60.0f;
        float boxY = cy - boxH + 14.0f;
        renderer.drawRectAlpha(rx + 16.0f, boxY, rw - 32.0f, boxH, 0.12f, 0.13f, 0.18f, 0.85f);
        renderer.drawRect(rx + 16.0f, boxY, 5.0f, boxH, prof.accentR, prof.accentG, prof.accentB);
        renderer.drawRectOutline(rx + 16.0f, boxY, rw - 32.0f, boxH, 0.30f, 0.32f, 0.40f);

        char nameLine[64];
        std::snprintf(nameLine, sizeof(nameLine), "%s  -  %s", prof.name, prof.title);
        renderer.drawTextSmall(rx + 28.0f, boxY + 38.0f, nameLine, prof.accentR, prof.accentG, prof.accentB);

        char recLine[64];
        std::snprintf(recLine, sizeof(recLine), "Matches: %d   Wins: %d   Win Rate: %d%%", m, w, pct);
        renderer.drawTextSmall(rx + 28.0f, boxY + 20.0f, recLine, 0.86f, 0.88f, 0.92f);

        // Win rate fill bar
        float barX = rx + 28.0f, barY = boxY + 7.0f, barW = rw - 56.0f, barH = 6.0f;
        renderer.drawRect(barX, barY, barW, barH, 0.06f, 0.07f, 0.10f);
        if (m > 0 && w > 0) {
            renderer.drawRect(barX, barY, barW * ((float)w / (float)m), barH, prof.accentR, prof.accentG, prof.accentB);
        }

        cy -= 69.0f;
    }

    bool hBack  = !confirmReset && UIButtons::BTN_STATS_BACK.contains(mouseX, mouseY);
    bool hReset = !confirmReset && UIButtons::BTN_STATS_RESET.contains(mouseX, mouseY);
    drawButton(UIButtons::BTN_STATS_BACK, hBack);
    drawButton(UIButtons::BTN_STATS_RESET, hReset);

    if (confirmReset) {
        drawConfirmDialog("RESET ALL STATS?",
                          "This will erase your high scores and combat records.",
                          "Are you sure you want to reset everything?",
                          mouseX, mouseY, "RESET", "CANCEL");
    }
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

    float cardX = 80.0f;
    float cardY = 135.0f;
    float cardW = 640.0f;
    float cardH = 335.0f;

    renderer.drawRectAlpha(cardX + 4.0f, cardY - 4.0f, cardW, cardH, 0.0f, 0.0f, 0.0f, 0.45f);
    renderer.drawRectAlpha(cardX, cardY, cardW, cardH, 0.08f, 0.08f, 0.12f, 0.90f);
    renderer.drawRectOutline(cardX, cardY, cardW, cardH, 0.45f, 0.45f, 0.52f);

    float startY = 438.0f;
    float stepY  = 29.0f;

    auto drawRow = [&](int row, const char* key, const char* desc, float kr, float kg, float kb) {
        float ry = startY - row * stepY;
        renderer.drawText(cardX + 30.0f, ry, key, kr, kg, kb);
        renderer.drawText(cardX + 210.0f, ry, desc, 0.92f, 0.92f, 0.95f);
    };

    drawRow(0, "A  /  D", "Move Left and Right", 1.0f, 0.85f, 0.35f);
    drawRow(1, "W  or  SPACE", "Jump (Leap into air / cross over)", 1.0f, 0.85f, 0.35f);
    drawRow(2, "S", "Crouch (Duck under high attacks)", 1.0f, 0.85f, 0.35f);
    drawRow(3, "J  (Tap x3)", "Attack / 3-Hit Martial Combo", 0.35f, 0.85f, 1.0f);
    drawRow(4, "K / W+K", "Kick (Unblockable) / Air Flying Kick", 1.0f, 0.55f, 0.25f);
    drawRow(5, "U", "Dodge Roll (Ch 1,3,4 evade Punch/Chain)", 0.45f, 0.95f, 0.70f);
    drawRow(6, "O", "Long Chain Grab + 2s Stun (Ch 2 Kuro)", 0.35f, 0.82f, 1.0f);
    drawRow(7, "L", "Character Critical / Special Attack", 1.0f, 0.85f, 0.20f);
    drawRow(8, "I", "Block Guard (Hold to absorb attacks)", 0.50f, 0.80f, 1.0f);
    drawRow(9, "P  or  ESC", "Pause Game / In-Fight Settings Menu", 0.80f, 0.80f, 0.80f);

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

void UI::drawGameOver(bool playerWon, int playerWins, int enemyWins,
                      const MatchStats& match, const CareerStats& career) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.72f);

    if (playerWon) {
        renderer.drawTextCentered(centerX, 495.0f, "VICTORY!", 1.0f, 0.85f, 0.30f, true);
        renderer.drawTextCentered(centerX, 460.0f, "You proved your shadow supremacy", 0.90f, 0.90f, 0.95f, false);
    } else {
        renderer.drawTextCentered(centerX, 495.0f, "DEFEATED", 1.0f, 0.30f, 0.30f, true);
        renderer.drawTextCentered(centerX, 460.0f, "The imperial warrior stands victorious", 0.85f, 0.85f, 0.90f, false);
    }

    char roundScore[64];
    std::snprintf(roundScore, sizeof(roundScore), "Rounds:   %d  -  %d", playerWins, enemyWins);
    renderer.drawTextCentered(centerX, 428.0f, roundScore, 1.0f, 1.0f, 1.0f, false);

    // Match Stats & Score Summary Card
    float boxW = 460.0f, boxH = 228.0f;
    float boxX = centerX - boxW * 0.5f;
    float boxY = 178.0f;
    renderer.drawRectAlpha(boxX + 4.0f, boxY - 4.0f, boxW, boxH, 0.0f, 0.0f, 0.0f, 0.60f);
    renderer.drawRectAlpha(boxX, boxY, boxW, boxH, 0.09f, 0.09f, 0.14f, 0.95f);
    renderer.drawRectOutline(boxX, boxY, boxW, boxH, 0.95f, 0.78f, 0.30f);

    if (match.isNewHighScore) {
        renderer.drawTextCentered(centerX, boxY + boxH - 26.0f, "NEW HIGH SCORE!", 1.0f, 0.90f, 0.25f, false);
    } else {
        renderer.drawTextCentered(centerX, boxY + boxH - 26.0f, "MATCH SUMMARY & STATS", 0.45f, 0.90f, 1.0f, false);
    }
    renderer.drawLine(boxX + 20.0f, boxY + boxH - 38.0f, boxX + boxW - 20.0f, boxY + boxH - 38.0f,
                      0.65f, 0.55f, 0.28f, 0.75f, 1.2f);

    char line[80];
    std::snprintf(line, sizeof(line), "Match Score: %d", match.score);
    renderer.drawText(boxX + 28.0f, boxY + 158.0f, line, 1.0f, 0.88f, 0.30f);
    std::snprintf(line, sizeof(line), "All-Time Best: %d", career.highScore);
    renderer.drawText(boxX + 245.0f, boxY + 158.0f, line, 0.60f, 0.95f, 0.65f);

    std::snprintf(line, sizeof(line), "Hits Landed: %d", match.hitsLanded);
    renderer.drawTextSmall(boxX + 28.0f, boxY + 126.0f, line, 0.90f, 0.92f, 0.98f);
    std::snprintf(line, sizeof(line), "Specials Landed: %d", match.specialsLanded);
    renderer.drawTextSmall(boxX + 245.0f, boxY + 126.0f, line, 1.0f, 0.75f, 0.30f);

    std::snprintf(line, sizeof(line), "Damage Dealt: %d", match.damageDealt);
    renderer.drawTextSmall(boxX + 28.0f, boxY + 100.0f, line, 0.95f, 0.55f, 0.50f);
    std::snprintf(line, sizeof(line), "Damage Taken: %d", match.damageTaken);
    renderer.drawTextSmall(boxX + 245.0f, boxY + 100.0f, line, 0.78f, 0.80f, 0.88f);

    std::snprintf(line, sizeof(line), "Dodges: %d   |   Blocks: %d", match.dodges, match.blocks);
    renderer.drawTextSmall(boxX + 28.0f, boxY + 74.0f, line, 0.55f, 0.88f, 1.0f);
    if (match.fastestRoundSec > 0.0f) {
        std::snprintf(line, sizeof(line), "Fastest Win: %.2fs", match.fastestRoundSec);
    } else {
        std::snprintf(line, sizeof(line), "Fastest Win: --");
    }
    renderer.drawTextSmall(boxX + 245.0f, boxY + 74.0f, line, 0.75f, 0.90f, 1.0f);

    std::snprintf(line, sizeof(line), "Current Win Streak: %d   (Best Streak: %d)   |   Lifetime K.O.s: %d",
                  career.currentWinStreak, career.bestWinStreak, career.totalKnockouts);
    renderer.drawTextSmall(boxX + 28.0f, boxY + 34.0f, line, 0.72f, 0.90f, 0.75f);

    renderer.drawTextCentered(centerX, 125.0f, "ENTER / Click : Rematch     ESC / M : Home Screen", 1.0f, 0.85f, 0.35f, false);
}
