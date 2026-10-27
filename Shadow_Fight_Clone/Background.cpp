#include "Background.h"
#include "Renderer.h"
#include "Config.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace {
    struct Star {
        float x;
        float y;
        float radius;
        float baseAlpha;
        float speed;
        float phase;
    };

    const Star STARS[] = {
        {  25.0f, 540.0f, 1.4f, 0.75f, 2.1f, 0.4f },
        {  60.0f, 480.0f, 1.0f, 0.55f, 1.8f, 1.2f },
        {  95.0f, 565.0f, 1.6f, 0.85f, 2.6f, 2.1f },
        { 130.0f, 510.0f, 1.1f, 0.45f, 1.4f, 0.9f },
        { 165.0f, 580.0f, 1.3f, 0.70f, 2.3f, 3.4f },
        { 190.0f, 450.0f, 1.0f, 0.40f, 1.7f, 4.1f },
        { 220.0f, 525.0f, 1.5f, 0.80f, 2.8f, 1.7f },
        { 255.0f, 475.0f, 1.0f, 0.50f, 1.9f, 2.8f },
        { 285.0f, 555.0f, 1.4f, 0.65f, 2.2f, 0.5f },
        { 320.0f, 430.0f, 0.9f, 0.35f, 1.3f, 3.9f },
        { 350.0f, 505.0f, 1.3f, 0.60f, 2.0f, 1.5f },
        { 380.0f, 570.0f, 1.5f, 0.75f, 2.5f, 2.6f },
        { 415.0f, 460.0f, 1.0f, 0.45f, 1.6f, 4.8f },
        { 440.0f, 535.0f, 1.4f, 0.70f, 2.4f, 0.8f },
        { 470.0f, 490.0f, 1.1f, 0.50f, 1.8f, 3.1f },
        { 500.0f, 560.0f, 1.5f, 0.80f, 2.7f, 1.9f },
        { 660.0f, 575.0f, 1.3f, 0.65f, 2.2f, 2.3f },
        { 690.0f, 510.0f, 1.0f, 0.45f, 1.5f, 0.7f },
        { 715.0f, 460.0f, 1.2f, 0.60f, 2.1f, 3.8f },
        { 740.0f, 545.0f, 1.6f, 0.85f, 2.9f, 1.1f },
        { 775.0f, 495.0f, 1.1f, 0.50f, 1.7f, 4.5f },
        {  45.0f, 390.0f, 1.0f, 0.40f, 1.5f, 2.0f },
        { 140.0f, 370.0f, 1.2f, 0.50f, 2.0f, 1.8f },
        { 240.0f, 360.0f, 0.9f, 0.35f, 1.2f, 3.2f },
        { 330.0f, 350.0f, 1.1f, 0.45f, 1.9f, 0.6f },
        { 420.0f, 380.0f, 1.3f, 0.55f, 2.3f, 2.4f },
        { 680.0f, 390.0f, 1.0f, 0.40f, 1.6f, 4.2f },
        { 760.0f, 370.0f, 1.2f, 0.50f, 2.1f, 1.4f }
    };
    const int NUM_STARS = sizeof(STARS) / sizeof(STARS[0]);

    struct ThemeColors {
        float skyZenithR,  skyZenithG,  skyZenithB;
        float skyHorizonR, skyHorizonG, skyHorizonB;

        float celX, celY, celR;
        float celColR,  celColG,  celColB;
        float haloColR, haloColG, haloColB;
        bool  isSun;

        float mtn1R, mtn1G, mtn1B;
        float mtn2R, mtn2G, mtn2B;
        float mtnRimR, mtnRimG, mtnRimB;

        float mistR, mistG, mistB;
        float floorRimR, floorRimG, floorRimB;
        float floorBaseR, floorBaseG, floorBaseB;
    };

    ThemeColors getThemeColors(ArenaTheme theme) {
        ThemeColors c;
        switch (theme) {
            case ArenaTheme::SUNSET:
                // Burning fiery golden/crimson evening
                c.skyZenithR  = 0.14f; c.skyZenithG  = 0.08f; c.skyZenithB  = 0.24f; // Violet-indigo
                c.skyHorizonR = 0.88f; c.skyHorizonG = 0.44f; c.skyHorizonB = 0.12f; // Glowing amber

                c.celX = 570.0f; c.celY = 260.0f; c.celR = 62.0f;
                c.celColR  = 1.0f;  c.celColG  = 0.94f; c.celColB  = 0.60f; // Radiant golden sun
                c.haloColR = 1.0f;  c.haloColG = 0.48f; c.haloColB = 0.12f; // Warm halo
                c.isSun = true;

                c.mtn1R = 0.18f; c.mtn1G = 0.09f; c.mtn1B = 0.15f; // Warm plum
                c.mtn2R = 0.11f; c.mtn2G = 0.05f; c.mtn2B = 0.08f; // Dark dusty charcoal
                c.mtnRimR = 0.95f; c.mtnRimG = 0.55f; c.mtnRimB = 0.20f; // Burning rim

                c.mistR = 0.55f; c.mistG = 0.24f; c.mistB = 0.15f; // Amber mist
                c.floorRimR  = 1.0f;  c.floorRimG  = 0.72f; c.floorRimB  = 0.25f; // Golden rim
                c.floorBaseR = 0.12f; c.floorBaseG = 0.08f; c.floorBaseB = 0.09f;
                break;

            case ArenaTheme::NIGHT:
                // Vibrant sapphire moonlit midnight (much brighter and clearer)
                c.skyZenithR  = 0.04f; c.skyZenithG  = 0.06f; c.skyZenithB  = 0.14f;
                c.skyHorizonR = 0.14f; c.skyHorizonG = 0.24f; c.skyHorizonB = 0.42f; // Cobalt-cyan

                c.celX = 580.0f; c.celY = 460.0f; c.celR = 52.0f;
                c.celColR  = 0.96f; c.celColG  = 0.98f; c.celColB  = 1.0f;  // Pearl full moon
                c.haloColR = 0.45f; c.haloColG = 0.68f; c.haloColB = 0.95f; // Cyan halo
                c.isSun = false;

                c.mtn1R = 0.09f; c.mtn1G = 0.13f; c.mtn1B = 0.22f; // Slate blue
                c.mtn2R = 0.05f; c.mtn2G = 0.07f; c.mtn2B = 0.12f; // Charcoal
                c.mtnRimR = 0.45f; c.mtnRimG = 0.70f; c.mtnRimB = 0.95f; // Moonlit rim

                c.mistR = 0.15f; c.mistG = 0.26f; c.mistB = 0.42f;
                c.floorRimR  = 0.65f; c.floorRimG  = 0.82f; c.floorRimB  = 1.0f; // Silver rim
                c.floorBaseR = 0.08f; c.floorBaseG = 0.09f; c.floorBaseB = 0.12f;
                break;

            case ArenaTheme::DAWN:
                // Soft pastel morning sunrise
                c.skyZenithR  = 0.20f; c.skyZenithG  = 0.28f; c.skyZenithB  = 0.40f; // Teal slate
                c.skyHorizonR = 0.92f; c.skyHorizonG = 0.68f; c.skyHorizonB = 0.42f; // Peach champagne

                c.celX = 220.0f; c.celY = 230.0f; c.celR = 56.0f;
                c.celColR  = 1.0f;  c.celColG  = 0.96f; c.celColB  = 0.75f; // Morning sun
                c.haloColR = 1.0f;  c.haloColG = 0.75f; c.haloColB = 0.40f; // Soft gold halo
                c.isSun = true;

                c.mtn1R = 0.22f; c.mtn1G = 0.18f; c.mtn1B = 0.28f; // Lavender
                c.mtn2R = 0.13f; c.mtn2G = 0.12f; c.mtn2B = 0.17f; // Dark slate
                c.mtnRimR = 0.95f; c.mtnRimG = 0.80f; c.mtnRimB = 0.50f; // Morning dew gold

                c.mistR = 0.48f; c.mistG = 0.42f; c.mistB = 0.42f; // Morning fog
                c.floorRimR  = 0.90f; c.floorRimG  = 0.82f; c.floorRimB  = 0.60f; // Dew highlight
                c.floorBaseR = 0.12f; c.floorBaseG = 0.12f; c.floorBaseB = 0.15f;
                break;

            case ArenaTheme::TWILIGHT:
                // Mystical purple dusk / cosmic twilight
                c.skyZenithR  = 0.11f; c.skyZenithG  = 0.05f; c.skyZenithB  = 0.20f; // Cosmic indigo
                c.skyHorizonR = 0.45f; c.skyHorizonG = 0.16f; c.skyHorizonB = 0.48f; // Electric violet

                c.celX = 570.0f; c.celY = 450.0f; c.celR = 48.0f;
                c.celColR  = 0.96f; c.celColG  = 0.88f; c.celColB  = 1.0f;  // Violet crescent moon
                c.haloColR = 0.65f; c.haloColG = 0.35f; c.haloColB = 0.85f; // Purple corona
                c.isSun = false;

                c.mtn1R = 0.14f; c.mtn1G = 0.07f; c.mtn1B = 0.20f; // Violet peaks
                c.mtn2R = 0.08f; c.mtn2G = 0.04f; c.mtn2B = 0.12f; // Obsidian crags
                c.mtnRimR = 0.78f; c.mtnRimG = 0.48f; c.mtnRimB = 0.95f; // Lilac rim

                c.mistR = 0.30f; c.mistG = 0.16f; c.mistB = 0.38f;
                c.floorRimR  = 0.80f; c.floorRimG  = 0.55f; c.floorRimB  = 0.95f; // Amethyst rim
                c.floorBaseR = 0.09f; c.floorBaseG = 0.07f; c.floorBaseB = 0.12f;
                break;
        }
        return c;
    }
}

Background::Background()
    : theme(ArenaTheme::SUNSET),
      location(ArenaLocation::TEMPLE_PEAKS),
      time(0.0f) {
    initLeaves();
    randomizeThemeAndLocation();
}

void Background::initLeaves() {
    for (int i = 0; i < 24; ++i) {
        leaves[i].x = (std::rand() % 850) - 25.0f;
        leaves[i].y = Config::GROUND_HEIGHT + 10.0f + (std::rand() % 480);
        leaves[i].speedX = 35.0f + (std::rand() % 50);
        leaves[i].speedY = 25.0f + (std::rand() % 45);
        leaves[i].size = 3.5f + (std::rand() % 40) / 10.0f;
        leaves[i].rotPhase = (std::rand() % 628) / 100.0f;
    }
}

void Background::randomizeThemeAndLocation() {
    int oldTheme = (int)theme;
    int oldLoc   = (int)location;

    // Pick a new random theme and location (avoiding repeating the exact same pair)
    int newTheme = (oldTheme + 1 + (std::rand() % 3)) % 4;
    int newLoc   = (oldLoc   + 1 + (std::rand() % 3)) % 4;

    theme    = (ArenaTheme)newTheme;
    location = (ArenaLocation)newLoc;
}

void Background::setThemeAndLocation(ArenaTheme newTheme, ArenaLocation newLocation) {
    theme    = newTheme;
    location = newLocation;
}

const char* Background::getThemeName() const {
    switch (theme) {
        case ArenaTheme::SUNSET:   return "SUNSET";
        case ArenaTheme::NIGHT:    return "NIGHT";
        case ArenaTheme::DAWN:     return "DAWN";
        case ArenaTheme::TWILIGHT: return "TWILIGHT";
    }
    return "ARENA";
}

const char* Background::getLocationName() const {
    switch (location) {
        case ArenaLocation::TEMPLE_PEAKS:  return "TEMPLE PEAKS";
        case ArenaLocation::BAMBOO_SHRINE: return "BAMBOO SHRINE";
        case ArenaLocation::CASTLE_ROOF:   return "CASTLE ROOFTOP";
        case ArenaLocation::BLOSSOM_VALLEY:return "BLOSSOM VALLEY";
    }
    return "ARENA";
}

void Background::getClearColor(float& r, float& g, float& b) const {
    ThemeColors c = getThemeColors(theme);
    r = c.skyZenithR;
    g = c.skyZenithG;
    b = c.skyZenithB;
}

void Background::update(float deltaTime) {
    time += deltaTime;

    // Update drifting petals/leaves for Blossom Valley
    if (location == ArenaLocation::BLOSSOM_VALLEY) {
        for (int i = 0; i < 24; ++i) {
            Leaf& l = leaves[i];
            float windWobble = std::sin(time * 2.5f + l.rotPhase) * 18.0f;
            l.x += (l.speedX + windWobble) * deltaTime;
            l.y -= l.speedY * deltaTime;

            if (l.y < Config::GROUND_HEIGHT - 6.0f || l.x > Config::WORLD_WIDTH + 30.0f) {
                l.x = (std::rand() % 400) - 50.0f;
                l.y = (float)Config::WORLD_HEIGHT + 10.0f + (std::rand() % 50);
            }
        }
    }
}

void Background::drawSky(const Renderer& renderer) const {
    const float worldW  = (float)Config::WORLD_WIDTH;
    const float worldH  = (float)Config::WORLD_HEIGHT;
    const float groundY = Config::GROUND_HEIGHT;
    ThemeColors c = getThemeColors(theme);

    // Vertical sky gradient (horizon to zenith)
    renderer.drawGradientRect(0.0f, groundY, worldW, worldH - groundY,
                              c.skyHorizonR, c.skyHorizonG, c.skyHorizonB, 1.0f,
                              c.skyZenithR,  c.skyZenithG,  c.skyZenithB,  1.0f);

    // Stars (visible in Night and Twilight)
    if (!c.isSun) {
        for (int i = 0; i < NUM_STARS; ++i) {
            const Star& s = STARS[i];
            float twinkle = 0.75f + 0.25f * std::sin(time * s.speed + s.phase);
            float alpha = s.baseAlpha * twinkle;
            renderer.drawCircle(s.x, s.y, s.radius, 0.90f, 0.95f, 1.0f, alpha, 8);
        }
    }
}

void Background::drawCelestial(const Renderer& renderer) const {
    ThemeColors c = getThemeColors(theme);

    // Soft atmospheric halo bloom
    renderer.drawCircle(c.celX, c.celY, c.celR * 3.4f, c.haloColR, c.haloColG, c.haloColB, 0.035f, 32);
    renderer.drawCircle(c.celX, c.celY, c.celR * 2.3f, c.haloColR, c.haloColG, c.haloColB, 0.070f, 30);
    renderer.drawCircle(c.celX, c.celY, c.celR * 1.5f, c.haloColR, c.haloColG, c.haloColB, 0.140f, 28);
    renderer.drawCircle(c.celX, c.celY, c.celR * 1.15f, c.haloColR, c.haloColG, c.haloColB, 0.240f, 28);

    // Luminous core disc
    renderer.drawCircle(c.celX, c.celY, c.celR, c.celColR, c.celColG, c.celColB, 1.0f, 36);

    // Sun god-rays (for Dawn)
    if (theme == ArenaTheme::DAWN) {
        for (int i = 0; i < 7; ++i) {
            float angle = 0.2f + i * 0.35f;
            float rayLen = 500.0f;
            float x1 = c.celX + std::cos(angle - 0.08f) * rayLen;
            float y1 = c.celY + std::sin(angle - 0.08f) * rayLen;
            float x2 = c.celX + std::cos(angle + 0.08f) * rayLen;
            float y2 = c.celY + std::sin(angle + 0.08f) * rayLen;
            renderer.drawTriangle(c.celX, c.celY, x1, y1, x2, y2, 1.0f, 0.85f, 0.45f, 0.06f);
        }
    }

    // Lunar maria / craters (for Night)
    if (theme == ArenaTheme::NIGHT) {
        renderer.drawCircle(c.celX - 16.0f, c.celY + 14.0f, 13.0f, 0.78f, 0.82f, 0.88f, 0.40f, 16);
        renderer.drawCircle(c.celX + 10.0f, c.celY + 18.0f, 10.0f, 0.76f, 0.80f, 0.87f, 0.35f, 14);
        renderer.drawCircle(c.celX -  8.0f, c.celY - 15.0f, 17.0f, 0.77f, 0.81f, 0.88f, 0.40f, 16);
        renderer.drawCircle(c.celX + 18.0f, c.celY -  9.0f, 12.0f, 0.79f, 0.83f, 0.89f, 0.32f, 14);
    }
}

void Background::drawMountains(const Renderer& renderer) const {
    const float groundY = Config::GROUND_HEIGHT;
    ThemeColors c = getThemeColors(theme);

    // Layer 1: Far Mountain Silhouette
    renderer.drawTriangle(-20.0f, groundY, 90.0f, 240.0f, 200.0f, groundY, c.mtn1R, c.mtn1G, c.mtn1B);
    renderer.drawTriangle(130.0f, groundY, 240.0f, 285.0f, 360.0f, groundY, c.mtn1R, c.mtn1G, c.mtn1B);
    renderer.drawTriangle(290.0f, groundY, 390.0f, 220.0f, 480.0f, groundY, c.mtn1R, c.mtn1G, c.mtn1B);
    renderer.drawTriangle(430.0f, groundY, 560.0f, 310.0f, 690.0f, groundY, c.mtn1R, c.mtn1G, c.mtn1B);
    renderer.drawTriangle(630.0f, groundY, 740.0f, 250.0f, 830.0f, groundY, c.mtn1R, c.mtn1G, c.mtn1B);

    // Mountain peak rim light catching celestial light
    renderer.drawLine(130.0f, groundY + 40.0f, 240.0f, 285.0f, c.mtnRimR, c.mtnRimG, c.mtnRimB, 0.40f, 1.5f);
    renderer.drawLine(430.0f, groundY + 40.0f, 560.0f, 310.0f, c.mtnRimR, c.mtnRimG, c.mtnRimB, 0.50f, 1.8f);

    // Layer 2: Mid-Ground Crags
    renderer.drawTriangle(-30.0f, groundY, 95.0f, 195.0f, 210.0f, groundY, c.mtn2R, c.mtn2G, c.mtn2B);
    renderer.drawTriangle(160.0f, groundY, 265.0f, 175.0f, 360.0f, groundY, c.mtn2R, c.mtn2G, c.mtn2B);
    renderer.drawTriangle(320.0f, groundY, 440.0f, 160.0f, 540.0f, groundY, c.mtn2R, c.mtn2G, c.mtn2B);
    renderer.drawTriangle(490.0f, groundY, 620.0f, 195.0f, 720.0f, groundY, c.mtn2R, c.mtn2G, c.mtn2B);
    renderer.drawTriangle(670.0f, groundY, 765.0f, 185.0f, 830.0f, groundY, c.mtn2R, c.mtn2G, c.mtn2B);
}

void Background::drawLocationElements(const Renderer& renderer) const {
    const float groundY = Config::GROUND_HEIGHT;
    ThemeColors c = getThemeColors(theme);
    float silR = c.mtn2R * 0.75f;
    float silG = c.mtn2G * 0.75f;
    float silB = c.mtn2B * 0.75f;

    switch (location) {
        case ArenaLocation::TEMPLE_PEAKS: {
            // 3-tier Pagoda silhouette on left ridge
            float px = 95.0f, py = 195.0f;
            renderer.drawRect(px - 14.0f, py, 28.0f, 4.0f, silR, silG, silB);
            renderer.drawRect(px - 9.0f, py + 4.0f, 18.0f, 12.0f, silR, silG, silB);
            renderer.drawQuad(px - 17.0f, py + 16.0f, px + 17.0f, py + 16.0f,
                              px + 12.0f, py + 21.0f, px - 12.0f, py + 21.0f, silR, silG, silB);
            renderer.drawRect(px - 7.0f, py + 21.0f, 14.0f, 10.0f, silR, silG, silB);
            renderer.drawQuad(px - 14.0f, py + 31.0f, px + 14.0f, py + 31.0f,
                              px + 10.0f, py + 35.0f, px - 10.0f, py + 35.0f, silR, silG, silB);
            renderer.drawRect(px - 5.0f, py + 35.0f, 10.0f, 8.0f, silR, silG, silB);
            renderer.drawQuad(px - 11.0f, py + 43.0f, px + 11.0f, py + 43.0f,
                              px + 7.0f, py + 47.0f, px - 7.0f, py + 47.0f, silR, silG, silB);
            renderer.drawRect(px - 1.0f, py + 47.0f, 2.0f, 22.0f, silR, silG, silB);
            renderer.drawCircle(px, py + 69.0f, 2.2f, silR, silG, silB, 1.0f, 8);

            // Gnarled pine on right crag
            float tx = 765.0f, ty = 185.0f;
            renderer.drawQuad(tx - 3.0f, ty, tx + 3.0f, ty, tx - 14.0f, ty + 22.0f, tx - 8.0f, ty + 24.0f, silR, silG, silB);
            renderer.drawCircle(tx - 18.0f, ty + 43.0f, 11.0f, silR, silG, silB, 1.0f, 14);
            renderer.drawCircle(tx - 28.0f, ty + 38.0f,  9.0f, silR, silG, silB, 1.0f, 14);
            renderer.drawCircle(tx -  4.0f, ty + 32.0f,  8.0f, silR, silG, silB, 1.0f, 12);
            break;
        }

        case ArenaLocation::BAMBOO_SHRINE: {
            // Grand Torii Gate silhouette
            float gx = 410.0f, gy = groundY;
            float gw = 130.0f, gh = 125.0f;
            // Pillars (Hashira)
            renderer.drawRect(gx - gw * 0.45f, gy, 12.0f, gh, silR, silG, silB);
            renderer.drawRect(gx + gw * 0.35f, gy, 12.0f, gh, silR, silG, silB);
            // Lower tie-beam (Nuki)
            renderer.drawRect(gx - gw * 0.52f, gy + gh * 0.72f, gw * 1.04f, 8.0f, silR, silG, silB);
            // Upper curved lintel (Kasagi) with sweeping flared ends
            renderer.drawQuad(gx - gw * 0.62f, gy + gh, gx + gw * 0.62f, gy + gh,
                              gx + gw * 0.68f, gy + gh + 14.0f, gx - gw * 0.68f, gy + gh + 14.0f, silR, silG, silB);
            // Central tablet strut (Gakuzuka)
            renderer.drawRect(gx - 4.0f, gy + gh * 0.72f, 8.0f, gh * 0.28f, silR, silG, silB);

            // Dense swaying Bamboo Stalks
            const float stalkX[] = { 35.0f, 70.0f, 115.0f, 160.0f, 210.0f, 255.0f, 310.0f, 520.0f, 570.0f, 615.0f, 660.0f, 705.0f, 745.0f, 780.0f };
            const float stalkW[] = { 9.0f,  7.0f,  11.0f,  8.0f,  10.0f,  7.0f,   9.0f,  11.0f,  8.0f,  10.0f,  7.0f,   9.0f,  12.0f,  8.0f };
            for (int i = 0; i < 14; ++i) {
                float sx = stalkX[i];
                float sw = stalkW[i];
                float sway = std::sin(time * 1.8f + sx * 0.05f) * 4.0f;
                // Bamboo trunk
                renderer.drawQuad(sx, groundY, sx + sw, groundY,
                                  sx + sw + sway, groundY + 220.0f, sx + sway, groundY + 220.0f,
                                  silR, silG, silB);
                // Node rings
                for (float ny = groundY + 30.0f; ny < groundY + 210.0f; ny += 32.0f) {
                    float nSway = sway * ((ny - groundY) / 220.0f);
                    renderer.drawRect(sx - 1.5f + nSway, ny, sw + 3.0f, 3.5f, c.mtnRimR, c.mtnRimG, c.mtnRimB);
                }
                // Leaf clusters
                renderer.drawCircle(sx + sway - 8.0f, groundY + 210.0f, 7.0f, silR, silG, silB, 1.0f, 10);
                renderer.drawCircle(sx + sway + sw + 8.0f, groundY + 195.0f, 6.0f, silR, silG, silB, 1.0f, 10);
            }
            break;
        }

        case ArenaLocation::CASTLE_ROOF: {
            // Feudal Fortress Tower (Tenshukaku)
            float cx = 400.0f, cy = groundY + 30.0f;
            // Stone battlement base
            renderer.drawQuad(cx - 160.0f, cy, cx + 160.0f, cy,
                              cx + 120.0f, cy + 50.0f, cx - 120.0f, cy + 50.0f, silR, silG, silB);
            // Tier 1 Castle Body
            renderer.drawRect(cx - 105.0f, cy + 50.0f, 210.0f, 40.0f, silR, silG, silB);
            // Tier 1 Curved Roof Eaves
            renderer.drawQuad(cx - 145.0f, cy + 90.0f, cx + 145.0f, cy + 90.0f,
                              cx + 120.0f, cy + 104.0f, cx - 120.0f, cy + 104.0f, silR, silG, silB);
            // Tier 2 Tower
            renderer.drawRect(cx - 75.0f, cy + 104.0f, 150.0f, 35.0f, silR, silG, silB);
            // Tier 2 Curved Roof Eaves
            renderer.drawQuad(cx - 105.0f, cy + 139.0f, cx + 105.0f, cy + 139.0f,
                              cx + 80.0f, cy + 152.0f, cx - 80.0f, cy + 152.0f, silR, silG, silB);
            // Golden Dolphin/Fish Roof Ornaments (Shachihoko)
            renderer.drawCircle(cx - 80.0f, cy + 156.0f, 5.0f, c.mtnRimR, c.mtnRimG, c.mtnRimB, 1.0f, 10);
            renderer.drawCircle(cx + 80.0f, cy + 156.0f, 5.0f, c.mtnRimR, c.mtnRimG, c.mtnRimB, 1.0f, 10);

            // Fluttering War Banners (Sashimono)
            const float bannerX[] = { 120.0f, 250.0f, 550.0f, 680.0f };
            for (int i = 0; i < 4; ++i) {
                float bx = bannerX[i];
                float flagWave = std::sin(time * 7.5f + i * 1.5f) * 6.0f;
                // Pole
                renderer.drawRect(bx, groundY, 3.0f, 95.0f, silR, silG, silB);
                // Flag cloth
                renderer.drawQuad(bx + 3.0f, groundY + 50.0f,
                                  bx + 32.0f + flagWave, groundY + 54.0f,
                                  bx + 28.0f + flagWave, groundY + 90.0f,
                                  bx + 3.0f, groundY + 92.0f,
                                  c.mtnRimR * 0.8f, c.mtnRimG * 0.5f, c.mtnRimB * 0.5f, 0.9f);
            }
            break;
        }

        case ArenaLocation::BLOSSOM_VALLEY: {
            // Distant arched moon bridge (Taiko-bashi)
            float bx = 400.0f, by = groundY + 15.0f;
            renderer.drawCircleOutline(bx, by - 40.0f, 75.0f, silR, silG, silB, 1.0f, 3.5f, 32);
            renderer.drawRect(bx - 40.0f, by + 32.0f, 80.0f, 5.0f, silR, silG, silB);

            // Twin gnarled sakura trees overarching left and right
            // Left Tree
            renderer.drawQuad(0.0f, groundY, 20.0f, groundY, 55.0f, groundY + 80.0f, 40.0f, groundY + 85.0f, silR, silG, silB);
            renderer.drawQuad(40.0f, groundY + 85.0f, 55.0f, groundY + 80.0f, 110.0f, groundY + 130.0f, 100.0f, groundY + 140.0f, silR, silG, silB);
            renderer.drawCircle(85.0f,  groundY + 130.0f, 24.0f, silR, silG, silB, 0.95f, 16);
            renderer.drawCircle(125.0f, groundY + 145.0f, 20.0f, silR, silG, silB, 0.95f, 16);

            // Right Tree
            renderer.drawQuad(800.0f, groundY, 780.0f, groundY, 745.0f, groundY + 80.0f, 760.0f, groundY + 85.0f, silR, silG, silB);
            renderer.drawQuad(760.0f, groundY + 85.0f, 745.0f, groundY + 80.0f, 690.0f, groundY + 130.0f, 700.0f, groundY + 140.0f, silR, silG, silB);
            renderer.drawCircle(715.0f, groundY + 130.0f, 24.0f, silR, silG, silB, 0.95f, 16);
            renderer.drawCircle(675.0f, groundY + 145.0f, 20.0f, silR, silG, silB, 0.95f, 16);

            // Drifting falling petals / autumn leaves
            for (int i = 0; i < 24; ++i) {
                const Leaf& l = leaves[i];
                float rot = std::sin(time * 3.0f + l.rotPhase);
                float pr = (theme == ArenaTheme::SUNSET) ? 0.95f : 0.85f;
                float pg = (theme == ArenaTheme::SUNSET) ? 0.45f : 0.55f;
                float pb = (theme == ArenaTheme::SUNSET) ? 0.25f : 0.65f;
                renderer.drawQuad(l.x, l.y,
                                  l.x + l.size * rot, l.y + l.size,
                                  l.x + l.size * 1.5f, l.y,
                                  l.x + l.size * 0.5f, l.y - l.size * rot,
                                  pr, pg, pb, 0.75f);
            }
            break;
        }
    }
}

void Background::drawMist(const Renderer& renderer) const {
    const float worldW  = (float)Config::WORLD_WIDTH;
    const float groundY = Config::GROUND_HEIGHT;
    ThemeColors c = getThemeColors(theme);

    // Horizon fog bank
    renderer.drawGradientRect(0.0f, groundY, worldW, 80.0f,
                              c.mistR, c.mistG, c.mistB, 0.35f,
                              c.mistR * 0.5f, c.mistG * 0.5f, c.mistB * 0.5f, 0.0f);

    // Low animated drifting ground fog ribbons
    float drift1 = std::fmod(time * 16.0f, worldW + 200.0f) - 100.0f;
    float drift2 = std::fmod(-time * 11.0f, worldW + 200.0f) + 100.0f;

    float yWobble1 = std::sin(time * 1.5f) * 4.0f;
    renderer.drawGradientRect(drift1 - 180.0f, groundY - 5.0f + yWobble1, 360.0f, 26.0f,
                              c.mistR * 1.2f, c.mistG * 1.2f, c.mistB * 1.2f, 0.16f,
                              c.mistR * 0.6f, c.mistG * 0.6f, c.mistB * 0.6f, 0.00f);

    renderer.drawGradientRect(drift1 - 180.0f - worldW - 200.0f, groundY - 5.0f + yWobble1, 360.0f, 26.0f,
                              c.mistR * 1.2f, c.mistG * 1.2f, c.mistB * 1.2f, 0.16f,
                              c.mistR * 0.6f, c.mistG * 0.6f, c.mistB * 0.6f, 0.00f);

    float yWobble2 = std::cos(time * 1.8f) * 3.5f;
    renderer.drawGradientRect(drift2 - 160.0f, groundY + 6.0f + yWobble2, 320.0f, 22.0f,
                              c.mistR, c.mistG, c.mistB, 0.12f,
                              c.mistR * 0.5f, c.mistG * 0.5f, c.mistB * 0.5f, 0.00f);

    renderer.drawGradientRect(drift2 - 160.0f + worldW + 200.0f, groundY + 6.0f + yWobble2, 320.0f, 22.0f,
                              c.mistR, c.mistG, c.mistB, 0.12f,
                              c.mistR * 0.5f, c.mistG * 0.5f, c.mistB * 0.5f, 0.00f);
}

void Background::drawFloor(const Renderer& renderer) const {
    const float worldW  = (float)Config::WORLD_WIDTH;
    const float groundY = Config::GROUND_HEIGHT;
    ThemeColors c = getThemeColors(theme);

    // Foundation dark base
    renderer.drawRect(0.0f, 0.0f, worldW, groundY, c.floorBaseR, c.floorBaseG, c.floorBaseB);

    switch (location) {
        case ArenaLocation::TEMPLE_PEAKS: {
            // Flagstone masonry courses (3 running bond tiers)
            const float widths1[] = { 105.0f, 95.0f, 110.0f, 85.0f, 105.0f, 115.0f, 95.0f, 90.0f };
            float curX = 0.0f;
            for (int i = 0; i < 8; ++i) {
                float t = 0.12f + (i % 3) * 0.02f;
                renderer.drawRect(curX + 1.0f, 0.0f, widths1[i] - 2.0f, 32.0f, t * c.floorBaseR * 8.0f, t * c.floorBaseG * 8.0f, t * c.floorBaseB * 8.0f);
                curX += widths1[i];
            }

            const float widths2[] = { 65.0f, 85.0f, 95.0f, 80.0f, 90.0f, 85.0f, 90.0f, 80.0f, 75.0f, 55.0f };
            curX = 0.0f;
            for (int i = 0; i < 10; ++i) {
                float t = 0.14f + (i % 3) * 0.02f;
                renderer.drawRect(curX + 1.0f, 34.0f, widths2[i] - 2.0f, 32.0f, t * c.floorBaseR * 8.0f, t * c.floorBaseG * 8.0f, t * c.floorBaseB * 8.0f);
                curX += widths2[i];
            }

            const float widths3[] = { 90.0f, 85.0f, 95.0f, 100.0f, 85.0f, 95.0f, 90.0f, 80.0f, 80.0f };
            curX = 0.0f;
            for (int i = 0; i < 9; ++i) {
                float t = 0.16f + (i % 3) * 0.02f;
                renderer.drawRect(curX + 1.0f, 68.0f, widths3[i] - 2.0f, 24.0f, t * c.floorBaseR * 8.0f, t * c.floorBaseG * 8.0f, t * c.floorBaseB * 8.0f);
                curX += widths3[i];
            }

            // Mortar seams
            renderer.drawRect(0.0f, 32.0f, worldW, 2.0f, 0.03f, 0.03f, 0.04f);
            renderer.drawRect(0.0f, 66.0f, worldW, 2.0f, 0.03f, 0.03f, 0.04f);
            renderer.drawRect(0.0f, 92.0f, worldW, 2.0f, 0.03f, 0.03f, 0.04f);

            // Boundary Stone Lanterns (Toro)
            const float lanternX[] = { 32.0f, worldW - 32.0f };
            for (int i = 0; i < 2; ++i) {
                float lx = lanternX[i];
                renderer.drawRect(lx - 14.0f, groundY, 28.0f, 10.0f, 0.14f, 0.15f, 0.18f);
                renderer.drawRect(lx - 6.0f, groundY + 10.0f, 12.0f, 28.0f, 0.15f, 0.16f, 0.19f);
                renderer.drawRect(lx - 14.0f, groundY + 38.0f, 28.0f, 8.0f, 0.18f, 0.19f, 0.22f);
                renderer.drawRect(lx - 10.0f, groundY + 46.0f, 20.0f, 20.0f, 0.08f, 0.08f, 0.10f);

                // Fire flame inside lantern
                float flicker = 0.85f + 0.15f * std::sin(time * 9.0f) * std::cos(time * 14.3f);
                renderer.drawCircle(lx, groundY + 56.0f, 45.0f, 1.0f, 0.65f, 0.20f, 0.07f * flicker, 20);
                renderer.drawCircle(lx, groundY + 56.0f, 8.0f * flicker, 1.0f, 0.88f, 0.40f, 0.85f, 16);

                // Flared roof cap
                renderer.drawQuad(lx - 18.0f, groundY + 66.0f, lx + 18.0f, groundY + 66.0f,
                                  lx + 13.0f, groundY + 74.0f, lx - 13.0f, groundY + 74.0f, 0.18f, 0.19f, 0.22f);
                renderer.drawCircle(lx, groundY + 80.0f, 3.5f, 0.22f, 0.24f, 0.28f, 1.0f, 10);
            }
            break;
        }

        case ArenaLocation::BAMBOO_SHRINE: {
            // Timber Boardwalk Shrine Floor
            for (float bx = 0.0f; bx < worldW; bx += 22.0f) {
                float t = 0.15f + ((int)bx % 4) * 0.015f;
                renderer.drawRect(bx + 1.0f, 0.0f, 20.0f, groundY - 6.0f, t * 1.1f, t * 0.85f, t * 0.65f);
                // Nail studs
                renderer.drawCircle(bx + 6.0f, groundY - 14.0f, 1.5f, 0.08f, 0.08f, 0.09f, 1.0f, 8);
                renderer.drawCircle(bx + 14.0f, groundY - 14.0f, 1.5f, 0.08f, 0.08f, 0.09f, 1.0f, 8);
            }

            // Standing Fire Braziers (Kagaribi)
            const float brazierX[] = { 35.0f, worldW - 35.0f };
            for (int i = 0; i < 2; ++i) {
                float fx = brazierX[i];
                // Iron tripod legs
                renderer.drawLine(fx, groundY + 45.0f, fx - 16.0f, groundY, 0.18f, 0.18f, 0.20f, 1.0f, 3.0f);
                renderer.drawLine(fx, groundY + 45.0f, fx + 16.0f, groundY, 0.18f, 0.18f, 0.20f, 1.0f, 3.0f);
                // Iron basket
                renderer.drawQuad(fx - 15.0f, groundY + 45.0f, fx + 15.0f, groundY + 45.0f,
                                  fx + 22.0f, groundY + 60.0f, fx - 22.0f, groundY + 60.0f, 0.12f, 0.12f, 0.14f);
                // Roaring fire
                float flicker = 0.8f + 0.2f * std::sin(time * 11.0f);
                renderer.drawCircle(fx, groundY + 62.0f, 50.0f, 1.0f, 0.50f, 0.10f, 0.08f * flicker, 20);
                renderer.drawCircle(fx, groundY + 64.0f, 14.0f * flicker, 1.0f, 0.75f, 0.20f, 0.80f, 16);
                renderer.drawCircle(fx, groundY + 66.0f,  7.0f * flicker, 1.0f, 0.95f, 0.60f, 0.95f, 12);
            }
            break;
        }

        case ArenaLocation::CASTLE_ROOF: {
            // Ceramic Japanese Roof Tiles (Kawara)
            for (float rx = 0.0f; rx < worldW; rx += 28.0f) {
                // Curved tile channel
                renderer.drawRect(rx + 2.0f, 0.0f, 24.0f, groundY - 6.0f, 0.13f, 0.14f, 0.16f);
                // Tile highlight rib
                renderer.drawLine(rx + 14.0f, 0.0f, rx + 14.0f, groundY - 6.0f, 0.22f, 0.24f, 0.28f, 0.7f, 2.0f);
                // Circular end-cap crest tile (Tomoe-gawara)
                renderer.drawCircle(rx + 14.0f, groundY - 10.0f, 7.0f, 0.18f, 0.19f, 0.23f, 1.0f, 14);
                renderer.drawCircle(rx + 14.0f, groundY - 10.0f, 4.0f, 0.10f, 0.11f, 0.13f, 1.0f, 10);
            }

            // Dragon/Demon crest tile finials (Onigawara) at arena corners
            const float finialX[] = { 25.0f, worldW - 25.0f };
            for (int i = 0; i < 2; ++i) {
                float ox = finialX[i];
                renderer.drawQuad(ox - 16.0f, groundY, ox + 16.0f, groundY,
                                  ox + 12.0f, groundY + 35.0f, ox - 12.0f, groundY + 35.0f, 0.15f, 0.16f, 0.19f);
                renderer.drawCircle(ox, groundY + 40.0f, 8.0f, 0.20f, 0.22f, 0.26f, 1.0f, 12);
            }
            break;
        }

        case ArenaLocation::BLOSSOM_VALLEY: {
            // Weathered River Stone Pavers with mossy seams
            for (float sx = 0.0f; sx < worldW; sx += 40.0f) {
                renderer.drawRect(sx + 2.0f, 0.0f, 36.0f, groundY - 6.0f, 0.12f, 0.14f, 0.13f);
                // Subtle mossy patch
                renderer.drawRectAlpha(sx + 5.0f, groundY - 18.0f, 12.0f, 10.0f, 0.14f, 0.22f, 0.12f, 0.4f);
            }

            // Sacred Stone Boundary Posts wrapped with Shimenawa rope
            const float postX[] = { 30.0f, worldW - 30.0f };
            for (int i = 0; i < 2; ++i) {
                float px = postX[i];
                renderer.drawRect(px - 10.0f, groundY, 20.0f, 45.0f, 0.16f, 0.17f, 0.20f);
                // Braided Shimenawa straw rope
                renderer.drawRect(px - 12.0f, groundY + 28.0f, 24.0f, 5.0f, 0.85f, 0.75f, 0.45f);
                // Paper zigzag tassels (Shide)
                renderer.drawTriangle(px - 6.0f, groundY + 28.0f, px - 2.0f, groundY + 28.0f, px - 4.0f, groundY + 16.0f, 0.95f, 0.95f, 0.95f);
                renderer.drawTriangle(px + 2.0f, groundY + 28.0f, px + 6.0f, groundY + 28.0f, px + 4.0f, groundY + 16.0f, 0.95f, 0.95f, 0.95f);
            }
            break;
        }
    }

    // Top Coping Slab / Arena Trim (y = 94 to 100)
    renderer.drawRect(0.0f, 94.0f, worldW, 6.0f, c.floorBaseR * 1.6f, c.floorBaseG * 1.6f, c.floorBaseB * 1.6f);

    // Brilliant Surface Rim Highlight catching celestial light from above
    renderer.drawLine(0.0f, 99.5f, worldW, 99.5f, c.floorRimR, c.floorRimG, c.floorRimB, 0.95f, 1.8f);
}

void Background::draw(const Renderer& renderer) const {
    // 1. Vibrant Sky Gradient & Stars
    drawSky(renderer);

    // 2. Glowing Sun or Moon with Halos & Sunbeams
    drawCelestial(renderer);

    // 3. Multi-Layer Mountain Ridges
    drawMountains(renderer);

    // 4. Unique Fighting Location Elements (Pagoda, Torii Gate, Castle, Sakura)
    drawLocationElements(renderer);

    // 5. Atmospheric Mist & Animated Low Fog Ribbons
    drawMist(renderer);

    // 6. Detailed Arena Ground Floor with Boundary Props
    drawFloor(renderer);
}
