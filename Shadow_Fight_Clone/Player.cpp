#include "Player.h"
#include "InputManager.h"

Player::Player(float startX, float startY)
    : Fighter(startX, startY, 50.0f, 100.0f) {
}

void Player::update(const InputManager& input, float deltaTime) {
    float direction = 0.0f;
    if (input.isKeyDown('a')) direction -= 1.0f;
    if (input.isKeyDown('d')) direction += 1.0f;

    if (input.isKeyDown('w')|| input.isKeyDown(' ')) jump();

    moveHorizontal(direction, deltaTime);
    applyPhysics(deltaTime);
}
