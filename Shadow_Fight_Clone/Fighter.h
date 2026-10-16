#ifndef FIGHTER_H
#define FIGHTER_H

class Renderer;   // forward declaration: we only need the name here

// Anything that fights: has a position, a size, and can move and draw itself.
// (Position is the BOTTOM-LEFT corner of the body rectangle.)
class Fighter {
public:
    Fighter(float startX, float startY, float bodyWidth, float bodyHeight);

    // direction: -1 = left, 0 = stand still, +1 = right
    void moveHorizontal(float direction, float deltaTime);

    void jump();                          // only works while standing on the ground
    void applyPhysics(float deltaTime);   // gravity + landing; call once per frame

    void draw(const Renderer& renderer) const;

private:
    float x;
    float y;
    float width;
    float height;
    float moveSpeed;     // pixels per second
    float velocityY;     // vertical speed in pixels per second (+ = up)
    bool  facingRight;
    bool  onGround;
};

#endif
