#include "Player.h"
#include "InputManager.h"

Player::Player(float startX, float startY, CharacterId charId)
    : Fighter(startX, startY, 86.0f, 185.0f) {
    setCharacter(charId);
}

void Player::update(const InputManager& input, float deltaTime) {
    float direction = 0.0f;
    if (input.isKeyDown('a')) direction -= 1.0f;
    if (input.isKeyDown('d')) direction += 1.0f;

    // W or Space bar both jump
    if (input.isKeyDown('w') || input.isKeyDown(' ')) jump();

    if (input.wasKeyPressed('u')) startRoll(direction); // Dodge roll (Ch 1, 3, 4)
    if (input.wasKeyPressed('o')) startChainGrab();     // Spiked chain pull + 2s stun (Ch 2)

    if (input.wasKeyPressed('j')) startPunch();         // Primary attack / 3-hit combo
    if (input.wasKeyPressed('k')) startKick();          // Ground kick OR Flying Jump Kick
    if (input.wasKeyPressed('l')) startSpecial();       // Character Critical / Special

    setCrouching(input.isKeyDown('s'));
    setBlocking(input.isKeyDown('i'));   // hold I to guard

    moveHorizontal(direction, deltaTime);
    applyPhysics(deltaTime);
    updateState(deltaTime);
}
