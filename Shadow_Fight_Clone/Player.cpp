#include "Player.h"
#include "InputManager.h"

Player::Player(float startX, float startY)
    : Fighter(startX, startY, 50.0f, 100.0f) {
    setColor(0.08f, 0.09f, 0.12f);       // Deep obsidian shadow silhouette
    setAccentColor(0.25f, 0.85f, 0.95f); // Luminous cyan headband / eye
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
