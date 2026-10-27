#include "Background.h"
#include "Renderer.h"
#include "Config.h"
#include <cmath>

namespace {
    struct Star {
        float x;
        float y;
        float radius;
        float baseAlpha;
        float speed;
        float phase;
    };

    // Pre-calculated star distribution for organic celestial feel
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

    // Draws a traditional stone lantern (Toro) with glowing fire chamber
    void drawStoneLantern(const Renderer& renderer, float x, float y, float time) {
        // Base pedestal (Kiso)
        renderer.drawRect(x - 14.0f, y, 28.0f, 5.0f, 0.12f, 0.13f, 0.16f);
        renderer.drawRect(x - 11.0f, y + 5.0f, 22.0f, 6.0f, 0.15f, 0.16f, 0.19f);

        // Column shaft (Sao)
        renderer.drawRect(x - 6.0f, y + 11.0f, 12.0f, 28.0f, 0.14f, 0.15f, 0.18f);

        // Middle shelf (Chudai)
        renderer.drawRect(x - 13.0f, y + 39.0f, 26.0f, 5.0f, 0.17f, 0.18f, 0.22f);
        renderer.drawRect(x - 15.0f, y + 44.0f, 30.0f, 3.0f, 0.19f, 0.20f, 0.24f);

        // Fire chamber (Hibukuro) background
        renderer.drawRect(x - 10.0f, y + 47.0f, 20.0f, 20.0f, 0.08f, 0.08f, 0.10f);

        // Warm flickering firelight inside the lantern
        float flicker = 0.85f + 0.15f * std::sin(time * 9.0f) * std::cos(time * 14.3f);
        float flameRadius = 7.0f * flicker;

        // Radial light aura around the lantern
        renderer.drawCircle(x, y + 57.0f, 48.0f, 1.0f, 0.65f, 0.20f, 0.06f * flicker, 20);
        renderer.drawCircle(x, y + 57.0f, 26.0f, 1.0f, 0.72f, 0.25f, 0.14f * flicker, 18);
        renderer.drawCircle(x, y + 57.0f, flameRadius, 1.0f, 0.88f, 0.45f, 0.85f, 16);

        // Wooden / stone window lattice bars
        renderer.drawRect(x - 10.0f, y + 47.0f, 3.0f, 20.0f, 0.13f, 0.13f, 0.15f);
        renderer.drawRect(x + 7.0f,  y + 47.0f, 3.0f, 20.0f, 0.13f, 0.13f, 0.15f);
        renderer.drawRect(x - 1.5f,  y + 47.0f, 3.0f, 20.0f, 0.13f, 0.13f, 0.15f);

        // Sweeping roof cap (Kasa) with flared eaves
        renderer.drawQuad(x - 18.0f, y + 67.0f,
                          x + 18.0f, y + 67.0f,
                          x + 14.0f, y + 74.0f,
                          x - 14.0f, y + 74.0f,
                          0.17f, 0.18f, 0.22f);
        renderer.drawTriangle(x - 14.0f, y + 74.0f,
                              x + 14.0f, y + 74.0f,
                              x,         y + 82.0f,
                              0.15f, 0.16f, 0.19f);

        // Roof turned-up tips
        renderer.drawTriangle(x - 21.0f, y + 71.0f,
                              x - 18.0f, y + 67.0f,
                              x - 14.0f, y + 72.0f,
                              0.17f, 0.18f, 0.22f);
        renderer.drawTriangle(x + 21.0f, y + 71.0f,
                              x + 18.0f, y + 67.0f,
                              x + 14.0f, y + 72.0f,
                              0.17f, 0.18f, 0.22f);

        // Top ornamental jewel (Hoju)
        renderer.drawCircle(x, y + 84.0f, 3.5f, 0.20f, 0.22f, 0.26f, 1.0f, 12);
        renderer.drawTriangle(x - 2.5f, y + 86.0f,
                              x + 2.5f, y + 86.0f,
                              x,        y + 90.0f,
                              0.22f, 0.24f, 0.28f);
    }
}

Background::Background()
    : time(0.0f) {
}

void Background::update(float deltaTime) {
    time += deltaTime;
}

void Background::draw(const Renderer& renderer) const {
    const float worldW = (float)Config::WORLD_WIDTH;
    const float groundY = Config::GROUND_HEIGHT;

    // =========================================================================
    // 1. NIGHT SKY GRADIENT (Midnight blue to deep horizon twilight)
    // =========================================================================
    renderer.drawGradientRect(0.0f, groundY, worldW, Config::WORLD_HEIGHT - groundY,
                              0.065f, 0.095f, 0.165f, 1.0f,   // horizon: dark moonlit teal-indigo
                              0.015f, 0.020f, 0.045f, 1.0f);  // sky zenith: midnight abyss

    // =========================================================================
    // 2. DISTANT TWINKLING STARS
    // =========================================================================
    for (int i = 0; i < NUM_STARS; ++i) {
        const Star& s = STARS[i];
        float twinkle = 0.75f + 0.25f * std::sin(time * s.speed + s.phase);
        float alpha = s.baseAlpha * twinkle;
        renderer.drawCircle(s.x, s.y, s.radius, 0.85f, 0.92f, 1.0f, alpha, 8);
    }

    // =========================================================================
    // 3. GLOWING MOON & CELESTIAL ATMOSPHERE
    // =========================================================================
    const float moonX = 580.0f;
    const float moonY = 460.0f;
    const float moonR = 52.0f;

    // Atmospheric halo glow rings (expanding soft radial light bloom)
    renderer.drawCircle(moonX, moonY, 170.0f, 0.40f, 0.60f, 0.85f, 0.025f, 32);
    renderer.drawCircle(moonX, moonY, 120.0f, 0.50f, 0.70f, 0.92f, 0.050f, 30);
    renderer.drawCircle(moonX, moonY,  82.0f, 0.65f, 0.82f, 1.00f, 0.090f, 28);
    renderer.drawCircle(moonX, moonY,  60.0f, 0.85f, 0.92f, 1.00f, 0.180f, 28);

    // Luminous lunar disc (pale pearl ivory with subtle cyan tint)
    renderer.drawCircle(moonX, moonY, moonR, 0.93f, 0.95f, 0.98f, 1.0f, 36);

    // Subtle lunar maria / shadow details (ancient craters seen from earth)
    renderer.drawCircle(moonX - 16.0f, moonY + 14.0f, 13.0f, 0.76f, 0.80f, 0.86f, 0.45f, 16);
    renderer.drawCircle(moonX + 10.0f, moonY + 18.0f, 10.0f, 0.74f, 0.78f, 0.85f, 0.40f, 14);
    renderer.drawCircle(moonX -  8.0f, moonY - 15.0f, 17.0f, 0.75f, 0.79f, 0.86f, 0.45f, 16);
    renderer.drawCircle(moonX + 18.0f, moonY -  9.0f, 12.0f, 0.77f, 0.81f, 0.87f, 0.38f, 14);
    renderer.drawCircle(moonX - 25.0f, moonY -  2.0f,  8.0f, 0.78f, 0.82f, 0.88f, 0.35f, 12);

    // =========================================================================
    // 4. DISTANT MOUNTAIN SILHOUETTE (Layer 1 - deep misty atmospheric peaks)
    // =========================================================================
    const float mtnColorR = 0.050f;
    const float mtnColorG = 0.065f;
    const float mtnColorB = 0.115f;

    // Peak 1 (Far Left)
    renderer.drawTriangle( -20.0f, groundY,
                            90.0f, 240.0f,
                           200.0f, groundY,
                           mtnColorR, mtnColorG, mtnColorB);

    // Peak 2 (Left Mid)
    renderer.drawTriangle( 130.0f, groundY,
                           240.0f, 285.0f,
                           360.0f, groundY,
                           mtnColorR, mtnColorG, mtnColorB);

    // Peak 3 (Center)
    renderer.drawTriangle( 290.0f, groundY,
                           390.0f, 220.0f,
                           480.0f, groundY,
                           mtnColorR, mtnColorG, mtnColorB);

    // Peak 4 (Right Giant Summit, partially behind moon)
    renderer.drawTriangle( 430.0f, groundY,
                           560.0f, 310.0f,
                           690.0f, groundY,
                           mtnColorR, mtnColorG, mtnColorB);

    // Peak 5 (Far Right)
    renderer.drawTriangle( 630.0f, groundY,
                           740.0f, 250.0f,
                           830.0f, groundY,
                           mtnColorR, mtnColorG, mtnColorB);

    // Subtle moonlit rim light on top mountain edges
    renderer.drawLine( 130.0f, groundY + 40.0f, 240.0f, 285.0f, 0.20f, 0.30f, 0.45f, 0.30f, 1.5f);
    renderer.drawLine( 430.0f, groundY + 40.0f, 560.0f, 310.0f, 0.28f, 0.40f, 0.60f, 0.40f, 1.8f);

    // =========================================================================
    // 5. MID-GROUND MOUNTAINS & CRAGS (Layer 2 - closer, darker charcoal peaks)
    // =========================================================================
    const float cragColorR = 0.030f;
    const float cragColorG = 0.038f;
    const float cragColorB = 0.065f;

    // Ridge 1 (Left with Pagoda perch)
    renderer.drawTriangle( -30.0f, groundY,
                            95.0f, 195.0f,
                           210.0f, groundY,
                           cragColorR, cragColorG, cragColorB);

    // Ridge 2
    renderer.drawTriangle( 160.0f, groundY,
                           265.0f, 175.0f,
                           360.0f, groundY,
                           cragColorR, cragColorG, cragColorB);

    // Ridge 3 (Center valley pass)
    renderer.drawTriangle( 320.0f, groundY,
                           440.0f, 160.0f,
                           540.0f, groundY,
                           cragColorR, cragColorG, cragColorB);

    // Ridge 4
    renderer.drawTriangle( 490.0f, groundY,
                           620.0f, 195.0f,
                           720.0f, groundY,
                           cragColorR, cragColorG, cragColorB);

    // Ridge 5 (Right cliff with pine tree)
    renderer.drawTriangle( 670.0f, groundY,
                           765.0f, 185.0f,
                           830.0f, groundY,
                           cragColorR, cragColorG, cragColorB);

    // =========================================================================
    // 6. MARTIAL-ARTS SILHOUETTES: ANCIENT PAGODA & CLIFF PINE
    // =========================================================================
    // Distant 3-tier Pagoda silhouette on the left mountain ridge
    {
        const float px = 95.0f;
        const float py = 195.0f;
        const float pr = 0.020f, pg = 0.025f, pb = 0.045f;

        // Base foundation
        renderer.drawRect(px - 14.0f, py, 28.0f, 4.0f, pr, pg, pb);

        // Tier 1
        renderer.drawRect(px - 9.0f, py + 4.0f, 18.0f, 12.0f, pr, pg, pb);
        renderer.drawQuad(px - 17.0f, py + 16.0f, px + 17.0f, py + 16.0f,
                          px + 12.0f, py + 21.0f, px - 12.0f, py + 21.0f,
                          pr, pg, pb);
        // Eave tips
        renderer.drawTriangle(px - 20.0f, py + 19.0f, px - 17.0f, py + 16.0f, px - 13.0f, py + 20.0f, pr, pg, pb);
        renderer.drawTriangle(px + 20.0f, py + 19.0f, px + 17.0f, py + 16.0f, px + 13.0f, py + 20.0f, pr, pg, pb);

        // Tier 2
        renderer.drawRect(px - 7.0f, py + 21.0f, 14.0f, 10.0f, pr, pg, pb);
        renderer.drawQuad(px - 14.0f, py + 31.0f, px + 14.0f, py + 31.0f,
                          px + 10.0f, py + 35.0f, px - 10.0f, py + 35.0f,
                          pr, pg, pb);
        // Eave tips
        renderer.drawTriangle(px - 17.0f, py + 34.0f, px - 14.0f, py + 31.0f, px - 11.0f, py + 34.0f, pr, pg, pb);
        renderer.drawTriangle(px + 17.0f, py + 34.0f, px + 14.0f, py + 31.0f, px + 11.0f, py + 34.0f, pr, pg, pb);

        // Tier 3
        renderer.drawRect(px - 5.0f, py + 35.0f, 10.0f, 8.0f, pr, pg, pb);
        renderer.drawQuad(px - 11.0f, py + 43.0f, px + 11.0f, py + 43.0f,
                          px +  7.0f, py + 47.0f, px -  7.0f, py + 47.0f,
                          pr, pg, pb);

        // Spire (Sorin)
        renderer.drawRect(px - 1.0f, py + 47.0f, 2.0f, 22.0f, pr, pg, pb);
        renderer.drawCircle(px, py + 69.0f, 2.2f, pr, pg, pb, 1.0f, 8);
    }

    // Ancient Bonsai / Pine Tree silhouette on the right ridge
    {
        const float tx = 765.0f;
        const float ty = 185.0f;
        const float tr = 0.020f, tg = 0.025f, tb = 0.045f;

        // Gnarled trunk leaning inward
        renderer.drawQuad(tx - 3.0f, ty, tx + 3.0f, ty,
                          tx - 8.0f, ty + 24.0f, tx - 14.0f, ty + 22.0f,
                          tr, tg, tb);
        renderer.drawQuad(tx - 11.0f, ty + 23.0f, tx - 6.0f, ty + 25.0f,
                          tx - 16.0f, ty + 42.0f, tx - 21.0f, ty + 40.0f,
                          tr, tg, tb);

        // Tiered pine foliage needle clusters
        renderer.drawCircle(tx - 18.0f, ty + 43.0f, 11.0f, tr, tg, tb, 1.0f, 14);
        renderer.drawCircle(tx - 28.0f, ty + 38.0f,  9.0f, tr, tg, tb, 1.0f, 14);
        renderer.drawCircle(tx -  4.0f, ty + 32.0f,  8.0f, tr, tg, tb, 1.0f, 12);
        renderer.drawCircle(tx - 32.0f, ty + 26.0f,  7.0f, tr, tg, tb, 1.0f, 12);
    }

    // =========================================================================
    // 7. ATMOSPHERIC MIST & HORIZON FOG
    // =========================================================================
    // Soft horizontal mist bank at the mountain base
    renderer.drawGradientRect(0.0f, groundY, worldW, 75.0f,
                              0.085f, 0.125f, 0.205f, 0.40f,   // bottom: dense mist
                              0.040f, 0.060f, 0.110f, 0.00f);  // top: fades into sky

    // Low animated drifting fog ribbons hovering right above the arena floor
    {
        float drift1 = std::fmod(time * 16.0f, worldW + 200.0f) - 100.0f;
        float drift2 = std::fmod(-time * 11.0f, worldW + 200.0f) + 100.0f;

        // Fog Ribbon 1
        float yWobble1 = std::sin(time * 1.5f) * 4.0f;
        renderer.drawGradientRect(drift1 - 180.0f, groundY - 5.0f + yWobble1, 360.0f, 26.0f,
                                  0.14f, 0.19f, 0.28f, 0.18f,
                                  0.06f, 0.09f, 0.15f, 0.00f);

        // Fog Ribbon 1 wrap-around for seamless loop
        renderer.drawGradientRect(drift1 - 180.0f - worldW - 200.0f, groundY - 5.0f + yWobble1, 360.0f, 26.0f,
                                  0.14f, 0.19f, 0.28f, 0.18f,
                                  0.06f, 0.09f, 0.15f, 0.00f);

        // Fog Ribbon 2
        float yWobble2 = std::cos(time * 1.8f) * 3.5f;
        renderer.drawGradientRect(drift2 - 160.0f, groundY + 6.0f + yWobble2, 320.0f, 22.0f,
                                  0.12f, 0.17f, 0.25f, 0.14f,
                                  0.05f, 0.08f, 0.13f, 0.00f);

        // Fog Ribbon 2 wrap-around
        renderer.drawGradientRect(drift2 - 160.0f + worldW + 200.0f, groundY + 6.0f + yWobble2, 320.0f, 22.0f,
                                  0.12f, 0.17f, 0.25f, 0.14f,
                                  0.05f, 0.08f, 0.13f, 0.00f);
    }

    // =========================================================================
    // 8. ANCIENT STONE ARENA FLOOR (Flagstone masonry from y = 0 to 100)
    // =========================================================================
    // Foundation dark base
    renderer.drawRect(0.0f, 0.0f, worldW, groundY, 0.075f, 0.082f, 0.105f);

    // Stone masonry courses (Running bond flagstone pattern)
    // Tier 1: Foundation Stones (y = 0 to 32)
    {
        const float y1 = 0.0f;
        const float h1 = 32.0f;
        const float widths1[] = { 105.0f, 95.0f, 110.0f, 85.0f, 105.0f, 115.0f, 95.0f, 90.0f };
        const float tones1[]  = { 0.11f,  0.13f, 0.10f,  0.12f, 0.14f,  0.11f,  0.12f, 0.10f };

        float curX = 0.0f;
        for (int i = 0; i < 8; ++i) {
            float t = tones1[i];
            renderer.drawRect(curX + 1.0f, y1, widths1[i] - 2.0f, h1, t, t * 1.05f, t * 1.25f);
            // Subtle top edge highlight on stone
            renderer.drawLine(curX + 2.0f, y1 + h1 - 1.0f, curX + widths1[i] - 3.0f, y1 + h1 - 1.0f,
                              t * 1.5f, t * 1.6f, t * 1.9f, 0.5f, 1.0f);
            curX += widths1[i];
        }
    }

    // Tier 2: Middle Course (y = 34 to 66)
    {
        const float y2 = 34.0f;
        const float h2 = 32.0f;
        const float widths2[] = { 65.0f, 85.0f, 95.0f, 80.0f, 90.0f, 85.0f, 90.0f, 80.0f, 75.0f, 55.0f };
        const float tones2[]  = { 0.13f, 0.15f, 0.12f, 0.16f, 0.14f, 0.15f, 0.12f, 0.16f, 0.13f, 0.14f };

        float curX = 0.0f;
        for (int i = 0; i < 10; ++i) {
            float t = tones2[i];
            renderer.drawRect(curX + 1.0f, y2, widths2[i] - 2.0f, h2, t, t * 1.05f, t * 1.22f);
            // Top bevel highlight
            renderer.drawLine(curX + 2.0f, y2 + h2 - 1.0f, curX + widths2[i] - 3.0f, y2 + h2 - 1.0f,
                              t * 1.6f, t * 1.7f, t * 2.0f, 0.6f, 1.0f);
            curX += widths2[i];
        }

        // Ancient stone cracks on selected pavers for realism
        renderer.drawLine(190.0f, 62.0f, 204.0f, 48.0f, 0.05f, 0.05f, 0.07f, 0.7f, 1.0f);
        renderer.drawLine(204.0f, 48.0f, 212.0f, 44.0f, 0.05f, 0.05f, 0.07f, 0.7f, 1.0f);
        renderer.drawLine(525.0f, 58.0f, 538.0f, 40.0f, 0.05f, 0.05f, 0.07f, 0.7f, 1.0f);
    }

    // Tier 3: Upper Pavers (y = 68 to 92)
    {
        const float y3 = 68.0f;
        const float h3 = 24.0f;
        const float widths3[] = { 90.0f, 85.0f, 95.0f, 100.0f, 85.0f, 95.0f, 90.0f, 80.0f, 80.0f };
        const float tones3[]  = { 0.15f, 0.17f, 0.14f, 0.18f,  0.16f, 0.17f, 0.14f, 0.18f, 0.15f };

        float curX = 0.0f;
        for (int i = 0; i < 9; ++i) {
            float t = tones3[i];
            renderer.drawRect(curX + 1.0f, y3, widths3[i] - 2.0f, h3, t, t * 1.05f, t * 1.20f);
            // Edge highlight
            renderer.drawLine(curX + 2.0f, y3 + h3 - 1.0f, curX + widths3[i] - 3.0f, y3 + h3 - 1.0f,
                              t * 1.7f, t * 1.8f, t * 2.1f, 0.7f, 1.0f);
            curX += widths3[i];
        }

        // Cracks on upper pavers
        renderer.drawLine(340.0f, 88.0f, 355.0f, 76.0f, 0.05f, 0.05f, 0.07f, 0.7f, 1.0f);
        renderer.drawLine(645.0f, 86.0f, 658.0f, 72.0f, 0.05f, 0.05f, 0.07f, 0.7f, 1.0f);
    }

    // Stone mortar horizontal seams (dark shadow grooves)
    renderer.drawRect(0.0f, 32.0f, worldW, 2.0f, 0.040f, 0.045f, 0.060f);
    renderer.drawRect(0.0f, 66.0f, worldW, 2.0f, 0.040f, 0.045f, 0.060f);
    renderer.drawRect(0.0f, 92.0f, worldW, 2.0f, 0.035f, 0.038f, 0.050f);

    // Top Coping Slab / Arena Trim (y = 94 to 100)
    renderer.drawRect(0.0f, 94.0f, worldW, 6.0f, 0.185f, 0.205f, 0.245f);

    // Brilliant moonlit surface rim highlight (sharp edge catching the celestial moon from above)
    renderer.drawLine(0.0f, 99.5f, worldW, 99.5f, 0.52f, 0.65f, 0.82f, 0.90f, 1.5f);

    // =========================================================================
    // 9. STONE LANTERNS (TORO) AT ARENA BOUNDARIES
    // =========================================================================
    drawStoneLantern(renderer, 32.0f, groundY, time);
    drawStoneLantern(renderer, worldW - 32.0f, groundY, time);
}
