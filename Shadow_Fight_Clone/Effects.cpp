#include "Effects.h"
#include "Renderer.h"
#include "Config.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace {
    float randomFloat(float minVal, float maxVal) {
        float f = (std::rand() % 10000) / 10000.0f;
        return minVal + f * (maxVal - minVal);
    }
}

Effects::Effects()
    : shakeIntensity(0.0f),
      shakeDuration(0.0f),
      shakeTimer(0.0f),
      shakeX(0.0f),
      shakeY(0.0f),
      shakeFreqTimer(0.0f) {
    particles.reserve(256);
}

void Effects::reset() {
    particles.clear();
    shakeIntensity = 0.0f;
    shakeDuration  = 0.0f;
    shakeTimer     = 0.0f;
    shakeX         = 0.0f;
    shakeY         = 0.0f;
}

void Effects::triggerShake(float intensity, float duration) {
    // Keep the strongest shake if one is already active
    if (shakeTimer > 0.0f && intensity < shakeIntensity) return;

    shakeIntensity = intensity;
    shakeDuration  = duration;
    shakeTimer     = duration;
}

void Effects::triggerDust(float x, float y, float direction) {
    for (int i = 0; i < 5; ++i) {
        Particle p;
        p.x = x + randomFloat(-12.0f, 12.0f);
        p.y = y + randomFloat(1.0f, 6.0f);
        p.prevX = p.x;
        p.prevY = p.y;
        p.vx = -direction * randomFloat(25.0f, 90.0f) + randomFloat(-20.0f, 20.0f);
        p.vy = randomFloat(15.0f, 55.0f);
        p.size = randomFloat(4.5f, 7.5f);
        p.r = 0.22f; p.g = 0.26f; p.b = 0.32f;
        p.alpha = randomFloat(0.30f, 0.45f);
        p.maxLife = randomFloat(0.35f, 0.55f);
        p.currentLife = p.maxLife;
        p.type = 1; // dust
        particles.push_back(p);
    }
}

void Effects::triggerHit(float x, float y, float pushDirection, SparkStyle style) {
    float dirAngle = (pushDirection > 0.0f) ? 0.0f : 3.14159265f;

    switch (style) {
        case SparkStyle::BLOCK: {
            // Electrical blue/cyan shield sparks scattering vertically along guard plane
            for (int i = 0; i < 18; ++i) {
                Particle p;
                p.x = x + randomFloat(-4.0f, 4.0f);
                p.y = y + randomFloat(-22.0f, 22.0f);
                p.prevX = p.x;
                p.prevY = p.y;
                float angle = dirAngle + randomFloat(-1.2f, 1.2f);
                float speed = randomFloat(180.0f, 420.0f);
                p.vx = std::cos(angle) * speed;
                p.vy = std::sin(angle) * speed;
                p.size = randomFloat(2.0f, 3.5f);
                p.r = 0.35f; p.g = 0.85f; p.b = 1.0f;
                p.alpha = 1.0f;
                p.maxLife = randomFloat(0.14f, 0.24f);
                p.currentLife = p.maxLife;
                p.type = 0; // spark
                particles.push_back(p);
            }
            // Small electric shockwave ring
            {
                Particle sw;
                sw.x = x; sw.y = y; sw.prevX = x; sw.prevY = y;
                sw.vx = 220.0f; sw.vy = 0.0f; // expansion speed
                sw.size = 8.0f;
                sw.r = 0.4f; sw.g = 0.9f; sw.b = 1.0f; sw.alpha = 0.85f;
                sw.maxLife = 0.16f; sw.currentLife = sw.maxLife;
                sw.type = 2; // shockwave
                particles.push_back(sw);
            }
            triggerShake(3.5f, 0.10f);
            break;
        }

        case SparkStyle::PUNCH: {
            // Bright golden-yellow / white sparks bursting in directional cone
            for (int i = 0; i < 22; ++i) {
                Particle p;
                p.x = x + randomFloat(-3.0f, 3.0f);
                p.y = y + randomFloat(-4.0f, 4.0f);
                p.prevX = p.x;
                p.prevY = p.y;
                float angle = dirAngle + randomFloat(-0.55f, 0.55f);
                float speed = randomFloat(250.0f, 620.0f);
                p.vx = std::cos(angle) * speed;
                p.vy = std::sin(angle) * speed + randomFloat(-60.0f, 80.0f);
                p.size = randomFloat(2.0f, 3.5f);
                p.r = 1.0f; p.g = 0.92f; p.b = 0.35f;
                p.alpha = 1.0f;
                p.maxLife = randomFloat(0.18f, 0.30f);
                p.currentLife = p.maxLife;
                p.type = 0; // spark
                particles.push_back(p);
            }
            // Impact flash burst
            {
                Particle fl;
                fl.x = x; fl.y = y; fl.prevX = x; fl.prevY = y;
                fl.vx = 0.0f; fl.vy = 0.0f;
                fl.size = 14.0f;
                fl.r = 1.0f; fl.g = 1.0f; fl.b = 0.85f; fl.alpha = 0.95f;
                fl.maxLife = 0.06f; fl.currentLife = fl.maxLife;
                fl.type = 3; // flash
                particles.push_back(fl);
            }
            triggerShake(5.5f, 0.14f);
            break;
        }

        case SparkStyle::KICK: {
            // Heavier fiery orange/red spark burst in wide cone
            for (int i = 0; i < 32; ++i) {
                Particle p;
                p.x = x + randomFloat(-4.0f, 4.0f);
                p.y = y + randomFloat(-5.0f, 5.0f);
                p.prevX = p.x;
                p.prevY = p.y;
                float angle = dirAngle + randomFloat(-0.85f, 0.85f);
                float speed = randomFloat(300.0f, 750.0f);
                p.vx = std::cos(angle) * speed;
                p.vy = std::sin(angle) * speed + randomFloat(-80.0f, 100.0f);
                p.size = randomFloat(2.5f, 4.2f);
                p.r = 1.0f; p.g = randomFloat(0.35f, 0.65f); p.b = 0.10f;
                p.alpha = 1.0f;
                p.maxLife = randomFloat(0.22f, 0.36f);
                p.currentLife = p.maxLife;
                p.type = 0;
                particles.push_back(p);
            }
            // Expanding shockwave ring
            {
                Particle sw;
                sw.x = x; sw.y = y; sw.prevX = x; sw.prevY = y;
                sw.vx = 280.0f; sw.vy = 0.0f;
                sw.size = 10.0f;
                sw.r = 1.0f; sw.g = 0.70f; sw.b = 0.20f; sw.alpha = 0.90f;
                sw.maxLife = 0.22f; sw.currentLife = sw.maxLife;
                sw.type = 2;
                particles.push_back(sw);
            }
            triggerDust(x, Config::GROUND_HEIGHT, pushDirection);
            triggerShake(8.5f, 0.20f);
            break;
        }

        case SparkStyle::SPECIAL: {
            // Explosive 360-degree nova burst with fiery amber/golden sparks
            for (int i = 0; i < 48; ++i) {
                Particle p;
                p.x = x + randomFloat(-5.0f, 5.0f);
                p.y = y + randomFloat(-5.0f, 5.0f);
                p.prevX = p.x;
                p.prevY = p.y;
                float angle = randomFloat(0.0f, 6.283185f);
                float speed = randomFloat(280.0f, 850.0f);
                p.vx = std::cos(angle) * speed;
                p.vy = std::sin(angle) * speed;
                p.size = randomFloat(2.8f, 5.0f);
                p.r = 1.0f; p.g = randomFloat(0.55f, 0.90f); p.b = 0.15f;
                p.alpha = 1.0f;
                p.maxLife = randomFloat(0.26f, 0.44f);
                p.currentLife = p.maxLife;
                p.type = 0;
                particles.push_back(p);
            }
            // Large golden shockwave ring
            {
                Particle sw;
                sw.x = x; sw.y = y; sw.prevX = x; sw.prevY = y;
                sw.vx = 340.0f; sw.vy = 0.0f;
                sw.size = 12.0f;
                sw.r = 1.0f; sw.g = 0.85f; sw.b = 0.30f; sw.alpha = 0.95f;
                sw.maxLife = 0.28f; sw.currentLife = sw.maxLife;
                sw.type = 2;
                particles.push_back(sw);
            }
            triggerDust(x, Config::GROUND_HEIGHT, pushDirection);
            triggerShake(13.5f, 0.28f);
            break;
        }

        case SparkStyle::KO: {
            // Massive fireworks-style K.O. burst
            for (int i = 0; i < 65; ++i) {
                Particle p;
                p.x = x + randomFloat(-6.0f, 6.0f);
                p.y = y + randomFloat(-6.0f, 6.0f);
                p.prevX = p.x;
                p.prevY = p.y;
                float angle = randomFloat(0.0f, 6.283185f);
                float speed = randomFloat(300.0f, 950.0f);
                p.vx = std::cos(angle) * speed;
                p.vy = std::sin(angle) * speed;
                p.size = randomFloat(3.0f, 5.5f);
                p.r = 1.0f; p.g = randomFloat(0.25f, 0.95f); p.b = randomFloat(0.10f, 0.40f);
                p.alpha = 1.0f;
                p.maxLife = randomFloat(0.35f, 0.60f);
                p.currentLife = p.maxLife;
                p.type = 0;
                particles.push_back(p);
            }
            // Dual concentric shockwave rings
            {
                Particle sw1;
                sw1.x = x; sw1.y = y; sw1.prevX = x; sw1.prevY = y;
                sw1.vx = 420.0f; sw1.vy = 0.0f;
                sw1.size = 10.0f;
                sw1.r = 1.0f; sw1.g = 0.90f; sw1.b = 0.40f; sw1.alpha = 1.0f;
                sw1.maxLife = 0.35f; sw1.currentLife = sw1.maxLife;
                sw1.type = 2;
                particles.push_back(sw1);

                Particle sw2;
                sw2.x = x; sw2.y = y; sw2.prevX = x; sw2.prevY = y;
                sw2.vx = 260.0f; sw2.vy = 0.0f;
                sw2.size = 6.0f;
                sw2.r = 1.0f; sw2.g = 0.30f; sw2.b = 0.20f; sw2.alpha = 0.85f;
                sw2.maxLife = 0.40f; sw2.currentLife = sw2.maxLife;
                sw2.type = 2;
                particles.push_back(sw2);
            }
            triggerDust(x, Config::GROUND_HEIGHT, pushDirection);
            triggerShake(18.0f, 0.38f);
            break;
        }
    }
}

void Effects::update(float deltaTime) {
    // 1. Update Screen Shake
    if (shakeTimer > 0.0f) {
        shakeTimer -= deltaTime;
        if (shakeTimer <= 0.0f) {
            shakeTimer = 0.0f;
            shakeX = 0.0f;
            shakeY = 0.0f;
        } else {
            float t = (shakeDuration - shakeTimer) / shakeDuration;
            float decay = std::exp(-4.5f * t);
            shakeFreqTimer += deltaTime * 60.0f;
            shakeX = shakeIntensity * decay * std::sin(shakeFreqTimer);
            shakeY = shakeIntensity * decay * std::cos(shakeFreqTimer * 1.37f) * 0.75f;
        }
    } else {
        shakeX = 0.0f;
        shakeY = 0.0f;
    }

    // 2. Update Particles
    for (size_t i = 0; i < particles.size(); ) {
        Particle& p = particles[i];
        p.currentLife -= deltaTime;

        if (p.currentLife <= 0.0f) {
            particles[i] = particles.back();
            particles.pop_back();
            continue;
        }

        p.prevX = p.x;
        p.prevY = p.y;

        if (p.type == 0) { // Spark streak
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.vy -= 780.0f * deltaTime; // gravity on sparks
            p.vx *= 0.96f;
        } else if (p.type == 1) { // Dust puff
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.vx *= 0.88f;
            p.vy *= 0.88f;
            p.size += 14.0f * deltaTime;
        } else if (p.type == 2) { // Shockwave ring
            p.size += p.vx * deltaTime; // expands
        }

        ++i;
    }
}

void Effects::drawParticles(const Renderer& renderer) const {
    for (size_t i = 0; i < particles.size(); ++i) {
        const Particle& p = particles[i];
        float lifeRatio = p.currentLife / p.maxLife;

        if (p.type == 0) { // Spark streak line
            float a = p.alpha * lifeRatio;
            renderer.drawLine(p.prevX, p.prevY, p.x, p.y, p.r, p.g, p.b, a, p.size);
            // Glowing spark head
            renderer.drawCircle(p.x, p.y, p.size * 0.7f, 1.0f, 1.0f, 1.0f, a, 8);
        } else if (p.type == 1) { // Dust puff
            float a = p.alpha * lifeRatio;
            renderer.drawCircle(p.x, p.y, p.size, p.r, p.g, p.b, a, 14);
        } else if (p.type == 2) { // Shockwave ring
            float a = p.alpha * lifeRatio;
            renderer.drawCircleOutline(p.x, p.y, p.size, p.r, p.g, p.b, a, 2.2f, 24);
        } else if (p.type == 3) { // Impact flash burst
            float a = p.alpha * lifeRatio;
            renderer.drawCircle(p.x, p.y, p.size * lifeRatio, p.r, p.g, p.b, a, 14);
        }
    }
}

void Effects::drawVignette(const Renderer& renderer) const {
    const float w = (float)Config::WORLD_WIDTH;
    const float h = (float)Config::WORLD_HEIGHT;

    // Softened peripheral borders fading smoothly inward toward center (open & bright)
    const float marginX = 130.0f;
    const float marginY = 100.0f;

    // Left edge
    renderer.drawGradientRectH(0.0f, 0.0f, marginX, h,
                               0.0f, 0.0f, 0.0f, 0.35f,
                               0.0f, 0.0f, 0.0f, 0.0f);

    // Right edge
    renderer.drawGradientRectH(w - marginX, 0.0f, marginX, h,
                               0.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 0.35f);

    // Top edge
    renderer.drawGradientRect(0.0f, h - marginY, w, marginY,
                              0.0f, 0.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 0.0f, 0.32f);

    // Bottom edge
    renderer.drawGradientRect(0.0f, 0.0f, w, marginY,
                              0.0f, 0.0f, 0.0f, 0.30f,
                              0.0f, 0.0f, 0.0f, 0.0f);

    // 4 Corner soft shadow accents for subtle depth without darkening the center fight
    const float cornerSize = 100.0f;
    renderer.drawTriangle(0.0f, 0.0f, cornerSize, 0.0f, 0.0f, cornerSize, 0.0f, 0.0f, 0.0f, 0.22f);
    renderer.drawTriangle(w, 0.0f, w - cornerSize, 0.0f, w, cornerSize, 0.0f, 0.0f, 0.0f, 0.22f);
    renderer.drawTriangle(0.0f, h, cornerSize, h, 0.0f, h - cornerSize, 0.0f, 0.0f, 0.0f, 0.22f);
    renderer.drawTriangle(w, h, w - cornerSize, h, w, h - cornerSize, 0.0f, 0.0f, 0.0f, 0.22f);
}
