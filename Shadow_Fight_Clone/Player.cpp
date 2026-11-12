#include "Player.h"
#include "InputManager.h"

Player::Player(float startX, float startY, CharacterId charId)
    : Fighter(startX, startY, 92.0f, 188.0f) {
    setCharacter(charId);
}

void Player::update(const InputManager& input, float deltaTime) {
    float direction = 0.0f;
    if (input.isActionDown(GameAction::MOVE_LEFT))  direction -= 1.0f;
    if (input.isActionDown(GameAction::MOVE_RIGHT)) direction += 1.0f;

    if (input.isActionDown(GameAction::JUMP)) jump();

    if (input.wasActionPressed(GameAction::DODGE))      startRoll(direction);
    if (input.wasActionPressed(GameAction::CHAIN_GRAB)) startChainGrab();

    // Critical checked first; InputManager suppresses single-X punch when RT+X chord is pressed
    if (input.wasActionPressed(GameAction::CRITICAL)) startSpecial();
    if (input.wasActionPressed(GameAction::PUNCH))    startPunch();
    if (input.wasActionPressed(GameAction::KICK))     startKick();

    setCrouching(input.isActionDown(GameAction::CROUCH));
    setBlocking(input.isActionDown(GameAction::BLOCK));
    setRunning(input.isActionDown(GameAction::RUN_HOLD) && direction != 0.0f);

    moveHorizontal(direction, deltaTime);
    applyPhysics(deltaTime);
    updateState(deltaTime);
}
