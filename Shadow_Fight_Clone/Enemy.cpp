#include "Enemy.h"
#include <cmath>
#include <cstdlib>

namespace {
    const float WALK_SPEED_RATIO = 0.6f;   // the enemy walks at 60% of full speed
    const float PREFERRED_GAP    = 45.0f;  // how close it likes to stand (empty space between bodies)

    // Attacking
    const float FIRST_ATTACK_DELAY = 1.5f; // breathing room at the start of the fight
    const float REACTION_TIME      = 0.5f; // after arriving next to you, it waits this long
    const float MIN_COOLDOWN       = 1.2f; // wait between attacks: a random time in this range
    const float MAX_COOLDOWN       = 2.4f;

    // A random number from 0.0 up to 1.0
    float randomFraction() {
        return (std::rand() % 1000) / 1000.0f;
    }
}

Enemy::Enemy(float startX, float startY)
    : Fighter(startX, startY, 50.0f, 100.0f),
      aiState(AIState::IDLE), attackCooldown(FIRST_ATTACK_DELAY) {
    setColor(0.95f, 0.55f, 0.20f);   // orange, so it's easy to tell apart from the player
    setFacingRight(false);           // starts out facing left, toward the player
}

void Enemy::startRandomAttack() {
    int roll = std::rand() % 100;    // 0..99

    if (roll < 10 && getEnergy() >= Attacks::SPECIAL.energyCost) {
        startSpecial();              // 10% of the time, if it has the energy
    } else if (roll < 45) {
        startKick();                 // 35%
    } else {
        startPunch();                // the rest: about 55%
    }
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

    if (attackCooldown > 0.0f) attackCooldown -= deltaTime;

    // ---- the brain: pick ONE thing to do this frame ----
    float walkDirection = 0.0f;

    if (isDead()) {
        aiState = AIState::DEAD;                 // a defeated enemy does nothing
    } else if (target.isDead()) {
        aiState = AIState::IDLE;                 // it won: stop fighting
    } else if (getState() == FighterState::HIT) {
        aiState = AIState::HIT;                  // stunned: wait for the body to recover
    } else if (isAttacking()) {
        aiState = AIState::ATTACK;               // mid-swing: let it finish
    } else if (gap > PREFERRED_GAP) {
        aiState = AIState::APPROACH;             // too far: walk closer
        walkDirection = directionToTarget * WALK_SPEED_RATIO;
        if (attackCooldown < REACTION_TIME) attackCooldown = REACTION_TIME;   // short pause on arrival
    } else if (attackCooldown <= 0.0f) {
        startRandomAttack();                     // close enough and ready: attack!
        if (isAttacking()) {
            aiState = AIState::ATTACK;
            attackCooldown = MIN_COOLDOWN + randomFraction() * (MAX_COOLDOWN - MIN_COOLDOWN);
        } else {
            aiState = AIState::IDLE;             // couldn't start one (e.g. still landing): try next frame
        }
    } else {
        aiState = AIState::IDLE;                 // close enough, waiting for the cooldown
    }

    moveHorizontal(walkDirection, deltaTime);
    applyPhysics(deltaTime);
    updateState(deltaTime);
}

AIState Enemy::getAIState() const {
    return aiState;
}

const char* Enemy::getAIStateName() const {
    switch (aiState) {
        case AIState::IDLE:     return "IDLE";
        case AIState::APPROACH: return "APPROACH";
        case AIState::ATTACK:   return "ATTACK";
        case AIState::HIT:      return "HIT";
        case AIState::DEAD:     return "DEAD";
    }
    return "UNKNOWN";
}
