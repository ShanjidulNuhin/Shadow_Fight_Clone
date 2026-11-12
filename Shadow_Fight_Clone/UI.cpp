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
    float textY = btn.y + btn.height * 0.5f - renderer.getTextBaselineOffset(false);
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
    renderer.drawTextSmallCentered(worldW / 2.0f, worldH - 83.0f, label, 0.65f, 0.90f, 0.68f);

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
                             CharacterId playerChar, CharacterId enemyChar,
                             bool gamepadMode, int gamepadColumn) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.80f);

    renderer.drawTextCentered(centerX, 556.0f, "CHOOSE YOUR FIGHTERS", 1.0f, 1.0f, 1.0f, true);
    if (gamepadMode) {
        renderer.drawTextCentered(centerX, 530.0f,
                                  "Gamepad: Up/Down Pick  |  Left/Right Switch Side  |  A Fight Mode  |  B Back",
                                  1.0f, 0.85f, 0.35f, false);
    } else {
        renderer.drawTextCentered(centerX, 530.0f,
                                  "Pick your fighter on the left, then select a different opponent on the right",
                                  1.0f, 0.85f, 0.35f, false);
    }
    renderer.drawLine(45.0f, 514.0f, 755.0f, 514.0f, 0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    // Center divider between Your Fighter (left) and Opponent (right)
    renderer.drawLine(centerX, 86.0f, centerX, 504.0f, 0.55f, 0.48f, 0.28f, 0.55f, 1.5f);

    if (gamepadMode && gamepadColumn == 0) {
        renderer.drawTextCentered(210.0f, 486.0f, "> YOUR FIGHTER <", 1.0f, 0.92f, 0.35f, false);
    } else {
        renderer.drawTextCentered(210.0f, 486.0f, "YOUR FIGHTER", 0.35f, 0.90f, 1.0f, false);
    }
    if (gamepadMode && gamepadColumn == 1) {
        renderer.drawTextCentered(590.0f, 486.0f, "> OPPONENT <", 1.0f, 0.92f, 0.35f, false);
    } else {
        renderer.drawTextCentered(590.0f, 486.0f, "OPPONENT", 1.0f, 0.48f, 0.30f, false);
    }

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
                      const CareerStats* career,
                      bool gamepadConnected, const char* gamepadName) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.65f);

    renderer.drawTextCentered(centerX, 495.0f, "SHADOW FIGHT", 1.0f, 1.0f, 1.0f, true);
    renderer.drawTextCentered(centerX, 455.0f, "- THE SHADOW CLONE -", 1.0f, 0.85f, 0.35f, false);

    renderer.drawLine(centerX - 180.0f, 430.0f, centerX + 180.0f, 430.0f,
                      0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    bool h0 = UIButtons::BTN_MAIN_START.contains(mouseX, mouseY)      || (selectedIndex == 0);
    bool h1 = UIButtons::BTN_MAIN_CHARACTERS.contains(mouseX, mouseY) || (selectedIndex == 1);
    bool h2 = UIButtons::BTN_MAIN_STATS.contains(mouseX, mouseY)      || (selectedIndex == 2);
    bool h3 = UIButtons::BTN_MAIN_OPTIONS.contains(mouseX, mouseY)    || (selectedIndex == 3);
    bool h4 = UIButtons::BTN_MAIN_EXIT.contains(mouseX, mouseY)       || (selectedIndex == 4);

    drawButton(UIButtons::BTN_MAIN_START, h0);
    drawButton(UIButtons::BTN_MAIN_CHARACTERS, h1);
    drawButton(UIButtons::BTN_MAIN_STATS, h2);
    drawButton(UIButtons::BTN_MAIN_OPTIONS, h3);
    drawButton(UIButtons::BTN_MAIN_EXIT, h4);

    if (career != nullptr) {
        char banner[96];
        std::snprintf(banner, sizeof(banner), "HIGH SCORE: %d   |   WINS: %d / %d   |   BEST STREAK: %d",
                      career->highScore, career->matchesWon, career->matchesPlayed, career->bestWinStreak);
        renderer.drawTextCentered(centerX, 110.0f, banner, 0.60f, 0.92f, 0.68f, false);
    }

    if (gamepadConnected) {
        char gpText[96];
        std::snprintf(gpText, sizeof(gpText), "GAMEPAD CONNECTED (%s)  -  Use D-Pad / Stick + A Button",
                      gamepadName ? gamepadName : "Ready");
        renderer.drawTextCentered(centerX, 80.0f, gpText, 0.45f, 0.90f, 1.0f, false);
    } else {
        renderer.drawTextCentered(centerX, 80.0f, "Mouse / Keyboard or Plug in Any Gamepad Controller",
                                  0.60f, 0.60f, 0.65f, false);
    }
}

void UI::drawCharactersPage(float mouseX, float mouseY, CharacterId selectedChar,
                            float animTimer, bool combatPose, bool gamepadMode) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;
    const CharacterProfile& prof = Characters::get(selectedChar);

    // Dim background overlay
    renderer.drawOverlay(0.01f, 0.015f, 0.03f, 0.82f);

    // Header Title
    renderer.drawTextCentered(centerX, 568.0f, "CHARACTER SHOWCASE & ROSTER", 1.0f, 1.0f, 1.0f, true);
    if (gamepadMode) {
        renderer.drawTextCentered(centerX, 544.0f,
                                  "D-Pad / Stick / LB / RB : Switch Character   |   Y : Stance   |   B : Back",
                                  0.45f, 0.90f, 1.0f, false);
    } else {
        renderer.drawTextCentered(centerX, 544.0f,
                                  "Click tabs or use A / D / Arrows to inspect full body avatar, fighting style & critical moves",
                                  0.78f, 0.82f, 0.90f, false);
    }
    renderer.drawLine(38.0f, 532.0f, 762.0f, 532.0f, 0.80f, 0.65f, 0.25f, 0.70f, 1.5f);

    // 5 Character Selector Tabs
    for (int i = 0; i < Characters::COUNT; ++i) {
        const CharacterProfile& p = Characters::getByIndex(i);
        bool isSel = (i == (int)selectedChar);
        bool isHov = UIButtons::BTN_ROSTER_TABS[i].contains(mouseX, mouseY);
        drawButton(UIButtons::BTN_ROSTER_TABS[i], isHov, isSel, p.name);
    }

    // =========================================================================
    // LEFT CARD: CHARACTER NAME & COMPREHENSIVE DETAILS
    // =========================================================================
    float cardX = 38.0f;
    float cardY = 70.0f;
    float cardW = 424.0f;
    float cardH = 412.0f;

    // Drop shadow & plate
    renderer.drawRectAlpha(cardX + 4.0f, cardY - 4.0f, cardW, cardH, 0.0f, 0.0f, 0.0f, 0.55f);
    renderer.drawRectAlpha(cardX, cardY, cardW, cardH, 0.08f, 0.08f, 0.12f, 0.95f);
    renderer.drawRectOutline(cardX, cardY, cardW, cardH, prof.accentR, prof.accentG, prof.accentB);
    renderer.drawRect(cardX + 2.0f, cardY + 2.0f, 5.0f, cardH - 4.0f,
                      prof.accentR, prof.accentG, prof.accentB);

    // Header: Large Name + Title
    char nameHeader[64];
    std::snprintf(nameHeader, sizeof(nameHeader), "%s", prof.name);
    renderer.drawTextLarge(cardX + 18.0f, cardY + cardH - 30.0f, nameHeader,
                           prof.accentR, prof.accentG, prof.accentB);

    char titleBuf[64];
    std::snprintf(titleBuf, sizeof(titleBuf), "- %s -", prof.title);
    renderer.drawText(cardX + 160.0f, cardY + cardH - 28.0f, titleBuf, 1.0f, 0.88f, 0.35f);

    // Archetype / Gender / Physique
    const char* buildDesc = "Male Striker  |  Athletic Muscular Build  |  Unarmed Combat";
    if (selectedChar == CharacterId::CHAIN_WARRIOR) {
        buildDesc = "Male Reaver  |  Muscular Ninja Frame  |  Chains & Spikes";
    } else if (selectedChar == CharacterId::ARCHER) {
        buildDesc = "Female Huntress  |  Agile Athletic Figure  |  High Mobility Archery";
    } else if (selectedChar == CharacterId::FAN_DANCER) {
        buildDesc = "Female War Dancer  |  Graceful Sinuous Figure  |  Twin Bladed Fans";
    } else if (selectedChar == CharacterId::SPEAR_WARRIOR) {
        buildDesc = "Male Imperial Colossus  |  Heavy Samurai Musculature & Armor  |  Pure Spear";
    }
    renderer.drawTextSmall(cardX + 18.0f, cardY + cardH - 48.0f, buildDesc, 0.75f, 0.85f, 0.95f);

    renderer.drawLine(cardX + 16.0f, cardY + cardH - 58.0f, cardX + cardW - 16.0f, cardY + cardH - 58.0f,
                      prof.accentR * 0.8f, prof.accentG * 0.8f, prof.accentB * 0.8f, 0.70f, 1.2f);

    float textY = cardY + cardH - 78.0f;

    // 1. WEAPON & COMBAT STYLE
    renderer.drawTextSmall(cardX + 18.0f, textY, "WEAPON & COMBAT STYLE:", 1.0f, 0.85f, 0.30f);
    textY -= 17.0f;
    char line[128];
    std::snprintf(line, sizeof(line), "Weapon: %s", prof.weaponName);
    renderer.drawTextSmall(cardX + 22.0f, textY, line, 0.96f, 0.96f, 0.98f);
    textY -= 16.0f;
    std::snprintf(line, sizeof(line), "Style:  %s", prof.styleName);
    renderer.drawTextSmall(cardX + 22.0f, textY, line, 0.82f, 0.88f, 1.0f);
    textY -= 22.0f;

    // 2. COMBAT SKILLS & PASSIVES
    renderer.drawTextSmall(cardX + 18.0f, textY, "COMBAT ABILITIES & COMBOS:", 1.0f, 0.85f, 0.30f);
    textY -= 17.0f;
    if (selectedChar == CharacterId::BRAWLER) {
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Flying Jump-Kick: Leap into air (W) + Kick (K) for high airborne damage", 0.92f, 0.92f, 0.96f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Roll Dodge (U): Ducking roll slips under high punches & chain attacks", 0.92f, 0.92f, 0.96f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Fist Flurry: Rapid 3-hit unarmed martial combo (J -> J -> J)", 0.82f, 0.88f, 1.0f);
    } else if (selectedChar == CharacterId::CHAIN_WARRIOR) {
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Chain Grab (LB / O): Hurls spiked chain across the screen to snag opponent", 0.92f, 0.92f, 0.96f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* 2.0s Shock Stun: Drags snagged opponent close and stuns them for 2 seconds!", 1.0f, 0.85f, 0.35f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Whip Lash: Wide sweeping chain arcs control mid and long range spacing", 0.82f, 0.88f, 1.0f);
    } else if (selectedChar == CharacterId::ARCHER) {
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Tactical Back-Roll: Evades backward to establish lethal sniping range", 0.92f, 0.92f, 0.96f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Aerial Jump-Kick: Agile flying kick with high aerial repositioning", 0.92f, 0.92f, 0.96f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Kyudo Bow-Fu: Combines bow strikes, arrow slashes and point-blank shots", 0.82f, 0.88f, 1.0f);
    } else if (selectedChar == CharacterId::FAN_DANCER) {
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Dual Fan Flurry: Rapid slicing arcs with high attack frequency", 0.92f, 0.92f, 0.96f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Acrobatic Roll (U): Sinuous ground roll ducking incoming high attacks", 0.92f, 0.92f, 0.96f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Dashing Velocity: Exceptional movement speed and evasive agility", 0.82f, 0.88f, 1.0f);
    } else { // SPEAR_WARRIOR
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Pure Spear Discipline: All primary attacks use polearm (zero hand strikes)", 0.92f, 0.92f, 0.96f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Colossal Reach: Thrusts, sweeps & cleaves out-range all enemy melee weapons", 0.92f, 0.92f, 0.96f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "* Heavy Poise: Highest health (125 HP) and crushing guard pressure", 0.82f, 0.88f, 1.0f);
    }
    textY -= 22.0f;

    // 3. SIGNATURE CRITICAL HIT
    char critHeader[80];
    std::snprintf(critHeader, sizeof(critHeader), "SIGNATURE CRITICAL (RT+X / L) : %s", prof.specialName);
    renderer.drawTextSmall(cardX + 18.0f, textY, critHeader, 1.0f, 0.45f, 0.30f);
    textY -= 17.0f;
    if (selectedChar == CharacterId::BRAWLER) {
        renderer.drawTextSmall(cardX + 22.0f, textY, "Focuses raw shadow chi into fist and unleashes an explosive high-power strike", 1.0f, 0.90f, 0.85f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "that penetrates block defense and launches the enemy across the arena.", 0.85f, 0.85f, 0.90f);
    } else if (selectedChar == CharacterId::CHAIN_WARRIOR) {
        renderer.drawTextSmall(cardX + 22.0f, textY, "Infuses the spiked chain with crackling lightning, ensnares the opponent", 1.0f, 0.90f, 0.85f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "and shocks them with high-voltage electricity for massive damage.", 0.85f, 0.85f, 0.90f);
    } else if (selectedChar == CharacterId::ARCHER) {
        renderer.drawTextSmall(cardX + 22.0f, textY, "Executes a backward roll to open distance, draws bow to maximum tension", 1.0f, 0.90f, 0.85f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "and fires a giant high-velocity piercing war arrow across the screen.", 0.85f, 0.85f, 0.90f);
    } else if (selectedChar == CharacterId::FAN_DANCER) {
        renderer.drawTextSmall(cardX + 22.0f, textY, "Hurls twin folding fans sequentially; on contact, dashes in at top speed,", 1.0f, 0.90f, 0.85f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "leaps, grabs enemy's head with both hands, and slams a lethal knee to their face!", 0.85f, 0.85f, 0.90f);
    } else { // SPEAR_WARRIOR
        renderer.drawTextSmall(cardX + 22.0f, textY, "Raises spear skyward summoning 10 iron spears crashing from the heavens,", 1.0f, 0.90f, 0.85f);
        textY -= 15.0f;
        renderer.drawTextSmall(cardX + 22.0f, textY, "then hurls his main war spear with furious penetrating momentum!", 0.85f, 0.85f, 0.90f);
    }
    textY -= 22.0f;

    // 4. STAT ATTRIBUTES (Health, Damage, Agility, Special Cost)
    renderer.drawTextSmall(cardX + 18.0f, textY, "FIGHTER ATTRIBUTES:", 1.0f, 0.85f, 0.30f);
    textY -= 18.0f;
    char statLine[128];
    std::snprintf(statLine, sizeof(statLine), "Health: %d HP   |   Speed: %.0f px/s   |   Special Cost: %.0f Energy",
                  prof.maxHealth, prof.moveSpeed, prof.specialAttack.energyCost);
    renderer.drawTextSmall(cardX + 22.0f, textY, statLine, 0.45f, 0.92f, 0.65f);
    textY -= 18.0f;
    renderer.drawTextSmall(cardX + 22.0f, textY, prof.statsSummary, 0.75f, 0.78f, 0.88f);


    // =========================================================================
    // RIGHT CARD: BIG CHARACTER AVATAR SHOWCASE
    // =========================================================================
    float rightX = 478.0f;
    float rightY = 70.0f;
    float rightW = 284.0f;
    float rightH = 412.0f;

    renderer.drawRectAlpha(rightX + 4.0f, rightY - 4.0f, rightW, rightH, 0.0f, 0.0f, 0.0f, 0.55f);
    renderer.drawRectAlpha(rightX, rightY, rightW, rightH, 0.08f, 0.08f, 0.12f, 0.95f);
    renderer.drawRectOutline(rightX, rightY, rightW, rightH, prof.accentR, prof.accentG, prof.accentB);

    // Top Card Label
    renderer.drawText(rightX + 18.0f, rightY + rightH - 26.0f, "HERO AVATAR SHOWCASE",
                      prof.accentR, prof.accentG, prof.accentB);
    renderer.drawTextSmall(rightX + rightW - 95.0f, rightY + rightH - 26.0f, "SCALE 1.75X", 0.65f, 0.70f, 0.78f);
    renderer.drawLine(rightX + 16.0f, rightY + rightH - 35.0f, rightX + rightW - 16.0f, rightY + rightH - 35.0f,
                      prof.accentR * 0.7f, prof.accentG * 0.7f, prof.accentB * 0.7f, 0.65f, 1.2f);

    float avatarCenterX = rightX + rightW * 0.5f;
    float avatarGroundY = rightY + 54.0f;

    // Atmospheric Halo behind upper body
    renderer.drawCircle(avatarCenterX, avatarGroundY + 165.0f, 105.0f,
                        prof.accentR * 0.16f, prof.accentG * 0.16f, prof.accentB * 0.16f, 0.35f, 32);
    renderer.drawCircleOutline(avatarCenterX, avatarGroundY + 165.0f, 100.0f,
                               prof.accentR, prof.accentG, prof.accentB, 0.28f, 1.5f, 36);
    renderer.drawCircleOutline(avatarCenterX, avatarGroundY + 165.0f, 70.0f,
                               prof.accentR, prof.accentG, prof.accentB, 0.18f, 1.2f, 32);

    // Subtle rising flame/ember sparks
    for (int p = 0; p < 5; ++p) {
        float sparkT = std::fmod(animTimer * 0.8f + p * 0.42f, 1.8f);
        float sparkY = avatarGroundY + sparkT * 120.0f;
        float sparkX = avatarCenterX + std::sin(animTimer * 3.0f + p * 1.5f) * (35.0f + p * 12.0f);
        float sparkA = (1.0f - sparkT / 1.8f) * 0.65f;
        renderer.drawCircle(sparkX, sparkY, 2.2f, prof.accentR, prof.accentG, prof.accentB, sparkA, 8);
    }

    // Pedestal Floor Dais
    renderer.drawRectAlpha(avatarCenterX - 110.0f, avatarGroundY - 8.0f, 220.0f, 10.0f,
                           0.05f, 0.05f, 0.08f, 0.90f);
    renderer.drawLine(avatarCenterX - 105.0f, avatarGroundY + 2.0f, avatarCenterX + 105.0f, avatarGroundY + 2.0f,
                      prof.accentR, prof.accentG, prof.accentB, 0.85f, 2.0f);
    renderer.drawCircleOutline(avatarCenterX, avatarGroundY - 3.0f, 85.0f,
                               prof.accentR, prof.accentG, prof.accentB, 0.35f, 1.5f, 28);

    // =========================================================================
    // RENDER BIG HERO AVATAR SILHOUETTE
    // =========================================================================
    Fighter avatar(0.0f, 0.0f, prof.bodyWidth, prof.bodyHeight);
    avatar.setCharacter(selectedChar);
    avatar.setFacingRight(false); // Face toward the left details card!
    if (combatPose) {
        avatar.setShowcasePose(FighterState::PUNCH, 0.10f); // Combat ready strike pose
    } else {
        avatar.setShowcasePose(FighterState::IDLE, animTimer); // Breathing idle stance
    }

    float avatarScale = 1.75f;
    renderer.pushMatrix();
    renderer.translate(avatarCenterX - (prof.bodyWidth * 0.5f) * avatarScale, avatarGroundY);
    renderer.scale(avatarScale, avatarScale);
    avatar.draw(renderer);
    renderer.popMatrix();

    // Pedestal badge under avatar feet
    float badgeY = rightY + 14.0f;
    char badgeText[64];
    std::snprintf(badgeText, sizeof(badgeText), "%s  *  %s", prof.name, prof.weaponName);
    renderer.drawTextSmallCentered(avatarCenterX, badgeY, badgeText, 1.0f, 0.88f, 0.35f);

    // =========================================================================
    // BOTTOM CONTROLS & NAVIGATION BUTTONS
    // =========================================================================
    bool hPrev = UIButtons::BTN_ROSTER_PREV.contains(mouseX, mouseY);
    bool hBack = UIButtons::BTN_ROSTER_BACK.contains(mouseX, mouseY);
    bool hNext = UIButtons::BTN_ROSTER_NEXT.contains(mouseX, mouseY);
    bool hPose = UIButtons::BTN_ROSTER_POSE.contains(mouseX, mouseY);

    drawButton(UIButtons::BTN_ROSTER_PREV, hPrev);
    drawButton(UIButtons::BTN_ROSTER_BACK, hBack);
    drawButton(UIButtons::BTN_ROSTER_NEXT, hNext);

    const char* poseLabel = combatPose ? "STANCE: COMBAT" : "STANCE: IDLE";
    drawButton(UIButtons::BTN_ROSTER_POSE, hPose, combatPose, poseLabel);
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

void UI::drawControlsScreen(float mouseX, float mouseY, int selectedIndex,
                            const InputManager* input, bool showGamepadTab) const {
    const float centerX = Config::WORLD_WIDTH / 2.0f;

    renderer.drawOverlay(0.0f, 0.0f, 0.0f, 0.78f);

    bool gpConn = (input != nullptr) && input->isGamepadConnected();
    const char* gpName = (input != nullptr) ? input->getGamepadBackendName() : nullptr;

    renderer.drawTextCentered(centerX, 556.0f,
                              showGamepadTab ? "GAMEPAD CONTROLS" : "KEYBOARD CONTROLS",
                              1.0f, 1.0f, 1.0f, true);

    if (gpConn) {
        char gpStatus[96];
        std::snprintf(gpStatus, sizeof(gpStatus), "Controller Active: %s  (Auto-selected)",
                      gpName ? gpName : "Gamepad");
        renderer.drawTextCentered(centerX, 536.0f, gpStatus, 0.45f, 0.92f, 1.0f, false);
    } else {
        renderer.drawTextCentered(centerX, 536.0f,
                                  "No Gamepad Detected  -  Showing Keyboard Controls",
                                  0.78f, 0.80f, 0.85f, false);
    }

    drawButton(UIButtons::BTN_CTRL_TAB_KB,
               UIButtons::BTN_CTRL_TAB_KB.contains(mouseX, mouseY), !showGamepadTab);
    drawButton(UIButtons::BTN_CTRL_TAB_PAD,
               UIButtons::BTN_CTRL_TAB_PAD.contains(mouseX, mouseY), showGamepadTab);

    float cardX = 50.0f;
    float cardY = 122.0f;
    float cardW = 700.0f;
    float cardH = 370.0f;

    renderer.drawRectAlpha(cardX + 4.0f, cardY - 4.0f, cardW, cardH, 0.0f, 0.0f, 0.0f, 0.50f);
    renderer.drawRectAlpha(cardX, cardY, cardW, cardH, 0.08f, 0.08f, 0.12f, 0.94f);
    renderer.drawRectOutline(cardX, cardY, cardW, cardH, 0.55f, 0.50f, 0.35f);

    renderer.drawTextSmall(cardX + 20.0f,  cardY + cardH - 22.0f, "ACTION", 1.0f, 0.85f, 0.35f);
    renderer.drawTextSmall(cardX + 225.0f, cardY + cardH - 22.0f,
                           showGamepadTab ? "CONTROLLER BINDING" : "KEYBOARD BINDING",
                           0.45f, 0.90f, 1.0f);
    renderer.drawTextSmall(cardX + 435.0f, cardY + cardH - 22.0f, "EFFECT", 0.95f, 0.95f, 0.98f);
    renderer.drawLine(cardX + 16.0f, cardY + cardH - 31.0f, cardX + cardW - 16.0f, cardY + cardH - 31.0f,
                      0.55f, 0.45f, 0.25f, 0.65f, 1.0f);

    bool isRebind = (input != nullptr) && input->isRebinding();
    int  rebindIdx = isRebind ? (int)input->getRebindAction() : -1;

    for (int i = 0; i < (int)GameAction::COUNT; ++i) {
        GameAction act = (GameAction)i;
        RectButton rowBtn = UIButtons::controlRowButton(i);
        bool hovered  = !isRebind && rowBtn.contains(mouseX, mouseY);
        bool selected = (selectedIndex == i);
        bool waiting  = (rebindIdx == i);

        if (waiting) {
            renderer.drawRectAlpha(rowBtn.x, rowBtn.y, rowBtn.width, rowBtn.height, 0.38f, 0.28f, 0.06f, 0.92f);
            renderer.drawRectOutline(rowBtn.x, rowBtn.y, rowBtn.width, rowBtn.height, 1.0f, 0.85f, 0.25f);
        } else if (hovered || selected) {
            renderer.drawRectAlpha(rowBtn.x, rowBtn.y, rowBtn.width, rowBtn.height, 0.20f, 0.24f, 0.34f, 0.78f);
            renderer.drawRectOutline(rowBtn.x, rowBtn.y, rowBtn.width, rowBtn.height, 0.55f, 0.78f, 1.0f);
        }

        float textY = rowBtn.y + 8.0f;
        renderer.drawTextSmall(cardX + 20.0f, textY, InputManager::getActionName(act), 0.96f, 0.96f, 0.98f);

        char bindBuf[64];
        if (waiting) {
            if (showGamepadTab) {
                std::snprintf(bindBuf, sizeof(bindBuf), "PRESS BUTTON (RT+Btn / START cancel)");
            } else {
                std::snprintf(bindBuf, sizeof(bindBuf), "PRESS ANY KEY (ESC to cancel)");
            }
            renderer.drawTextSmall(cardX + 225.0f, textY, bindBuf, 1.0f, 0.90f, 0.25f);
        } else if (input != nullptr) {
            if (showGamepadTab) {
                InputManager::formatPadBinding(input->getPadBinding(act), bindBuf, sizeof(bindBuf));
            } else {
                int kc = input->getKeyBinding(act);
                InputManager::formatKeyName(kc, bindBuf, sizeof(bindBuf));
                if (act == GameAction::JUMP && kc == 'w') {
                    std::snprintf(bindBuf, sizeof(bindBuf), "W  (or SPACE)");
                } else if (act == GameAction::RUN_HOLD && kc == KeyCodes::SHIFT) {
                    std::snprintf(bindBuf, sizeof(bindBuf), "SHIFT  (+ A / D)");
                }
            }
            renderer.drawTextSmall(cardX + 225.0f, textY, bindBuf, 0.45f, 0.92f, 1.0f);
        }

        if (!waiting) {
            renderer.drawTextSmall(cardX + 435.0f, textY, InputManager::getActionDesc(act), 0.78f, 0.80f, 0.86f);
        }
    }

    renderer.drawTextCentered(centerX, 102.0f,
                              "Click any row (or press A / Enter) to rebind   |   Pause: P / ESC / START",
                              0.85f, 0.85f, 0.45f, false);

    bool hBack  = !isRebind && UIButtons::BTN_CTRL_BACK.contains(mouseX, mouseY);
    bool hReset = !isRebind && UIButtons::BTN_CTRL_RESET.contains(mouseX, mouseY);
    drawButton(UIButtons::BTN_CTRL_BACK, hBack);
    drawButton(UIButtons::BTN_CTRL_RESET, hReset);
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
