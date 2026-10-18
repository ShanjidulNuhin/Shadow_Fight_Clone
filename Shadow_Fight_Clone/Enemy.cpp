#include "Enemy.h"
#include <cmath>

namespace {
    const float WALK_SPEED_RATIO = 0.6f;   // the enemy walks at 60% of full speed
    const float PREFERRED_GAP    = 45.0f;  // how close it likes to stand (empty space between bodies)
}

Enemy::Enemy(float startX, float startY)
    : Fighter(startX, startY, 50.0f, 100.0f) {
    setColor(0.95f, 0.55f, 0.20f);   // orange, so it's easy to tell apart from the player
    setFacingRight(false);           // starts out facing left, toward the player
}

void Enemy::update(const Fighter& target, float deltaTime) {
    Rect me   = getHurtbox();
    Rect them = target.getHurtbox();

    float myCenter    = me.x   + me.width   / 2.0f;
    float theirCenter = them.x + them.width / 2.0f;

    // +1 if the target is to our right, -1 if it is to our left
    float directionToTarget = (theirCenter > myCenter) ? 1.0f : -1.0f;

    // the empty space between the two bodies
    float gap = std::fabs(theirCenter - myCenter) - (me.width + them.width) / 2.0f;

    // Always look at the target (unless we're busy attacking or stunned).
    if (canAct()) setFacingRight(directionToTarget > 0.0f);

    // Too far away? Walk toward the target, a bit slower than a full-speed walk.
    float direction = 0.0f;
    if (gap > PREFERRED_GAP) direction = directionToTarget * WALK_SPEED_RATIO;

    moveHorizontal(direction, deltaTime);
    applyPhysics(deltaTime);
    updateState(deltaTime);
}
