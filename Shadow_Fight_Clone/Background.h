#ifndef BACKGROUND_H
#define BACKGROUND_H

class Renderer;

// Renders the cinematic moonlit martial-arts arena:
// - Midnight gradient sky with twinkling stars and glowing moon
// - Layered distant mountain ridges and temple silhouette
// - Animated drifting atmospheric mist / fog
// - Detailed ancient stone arena floor with slabs, cracks, and stone lanterns
class Background {
public:
    Background();

    void update(float deltaTime);
    void draw(const Renderer& renderer) const;

private:
    float time; // accumulated time for subtle ambient animations (mist drift, lantern flicker, star twinkle)
};

#endif // BACKGROUND_H
