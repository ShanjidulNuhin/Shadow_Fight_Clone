#ifndef EFFECTS_H
#define EFFECTS_H

#include <vector>

class Renderer;

enum class SparkStyle {
    PUNCH,
    KICK,
    SPECIAL,
    BLOCK,
    KO
};

// Manages visual impact effects:
// - Directional hit sparks and impact bursts
// - Atmospheric dust puffs from movement & heavy strikes
// - Expanding shockwave rings
// - Damped multi-axis screen shake
// - Photographic cinematic vignette
class Effects {
public:
    Effects();

    void update(float deltaTime);
    void drawParticles(const Renderer& renderer) const;
    void drawVignette(const Renderer& renderer) const;

    // Trigger hit sparks, bursts, and tailored screen shake
    void triggerHit(float x, float y, float pushDirection, SparkStyle style);

    // Trigger foot landing / sliding dust puffs
    void triggerDust(float x, float y, float direction);

    // Screen shake controls
    void triggerShake(float intensity, float duration);
    float getShakeX() const { return shakeX; }
    float getShakeY() const { return shakeY; }

    void reset();

private:
    struct Particle {
        float x;
        float y;
        float prevX;
        float prevY;
        float vx;
        float vy;
        float size;
        float r, g, b, alpha;
        float maxLife;
        float currentLife;
        int   type; // 0 = spark streak, 1 = dust puff, 2 = shockwave ring, 3 = flash burst
    };

    std::vector<Particle> particles;

    // Screen shake state
    float shakeIntensity;
    float shakeDuration;
    float shakeTimer;
    float shakeX;
    float shakeY;
    float shakeFreqTimer;
};

#endif // EFFECTS_H
