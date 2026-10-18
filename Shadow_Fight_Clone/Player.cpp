#include "Player.h"
#include "InputManager.h"

Player::Player(float startX, float startY)
    : Fighter(startX, startY, 50.0f, 100.0f) {
}

void Player::update(const InputManager& input, float deltaTime) {
    float direction = 0.0f;
    if (input.isKeyDown('a')) direction -= 1.0f;
    if (input.isKeyDown('d')) direction += 1.0f;

    // W or Space bar both jump
    if (input.isKeyDown('w') || input.isKeyDown(' ')) jump();

    if (input.isKeyDown('j')) startPunch();
    if (input.isKeyDown('k')) startKick();
    if (input.isKeyDown('l')) startSpecial();   // needs enough energy

    setCrouching(input.isKeyDown('s'));
    setBlocking(input.isKeyDown('i'));   // hold I to guard

    moveHorizontal(direction, deltaTime);
    applyPhysics(deltaTime);
    updateState(deltaTime);   // always last: it looks at the result of everything above
}
