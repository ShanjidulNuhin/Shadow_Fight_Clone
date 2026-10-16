#include "Fighter.h"
#include "Renderer.h"
#include "Config.h"

Fighter::Fighter(float startX, float startY, float bodyWidth, float bodyHeight)
    : x(startX), y(startY), width(bodyWidth), height(bodyHeight),
      moveSpeed(250.0f), velocityY(0.0f), facingRight(true), onGround(false) {
}

void Fighter::moveHorizontal(float direction, float deltaTime) {
    x += direction * moveSpeed * deltaTime;

    if (direction > 0.0f) facingRight = true;
    else if (direction < 0.0f) facingRight = false;

    // keep the body inside the world
    if (x < 0.0f) x = 0.0f;
    if (x + width > Config::WORLD_WIDTH) x = Config::WORLD_WIDTH - width;
}

void Fighter::jump() {
    if (onGround) {
        velocityY = Config::JUMP_SPEED;
        onGround = false;
    }
}

void Fighter::applyPhysics(float deltaTime) {
    // gravity changes the speed, and the speed changes the position
    velocityY += Config::GRAVITY * deltaTime;
    y += velocityY * deltaTime;

    // landed on (or sunk into) the ground?
    if (y <= Config::GROUND_HEIGHT) {
        y = Config::GROUND_HEIGHT;
        velocityY = 0.0f;
        onGround = true;
    }
}

void Fighter::draw(const Renderer& renderer) const {
    // body
    renderer.drawRect(x, y, width, height, 1.0f, 1.0f, 1.0f);

    // small dark "eye" on the front so we can see which way we're facing
    const float eyeSize = 10.0f;
    float eyeX = facingRight ? (x + width - eyeSize - 4.0f) : (x + 4.0f);
    float eyeY = y + height - eyeSize - 8.0f;
    renderer.drawRect(eyeX, eyeY, eyeSize, eyeSize, 0.1f, 0.1f, 0.1f);
}
