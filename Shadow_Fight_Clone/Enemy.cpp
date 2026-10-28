#include "Enemy.h"
#include <cmath>
#include <cstdlib>

namespace {
    const float WALK_SPEED_RATIO    = 0.6f;
    const float RETREAT_SPEED_RATIO = 0.4f;
    const float MIN_SPACING   = 30.0f;
    const float MAX_SPACING   = 62.0f;
    const float SPACING_BAND  = 4.0f;

    // Timing
    const float FIRST_ATTACK_DELAY = 1.5f;
    const float ARRIVAL_PAUSE      = 0.3f;

    // Defending defaults
    const float GUARD_URGE_PER_SECOND = 0.15f;
    const float GUARD_URGE_RANGE      = 70.0f;
    const float MIN_GUARD_TIME   = 0.4f;
    const float MAX_GUARD_TIME   = 1.0f;
    const float NOTICE_MARGIN    = 15.0f;

    // Jumping defaults
    const float APPROACH_JUMP_PER_SECOND = 0.30f;
    const float APPROACH_JUMP_MIN_GAP    = 70.0f;
    const float AIR_DRIFT = 0.8f;
    const float REACH_MARGIN = 5.0f;

    float randomFraction() {
        return (std::rand() % 1000) / 1000.0f;
    }

    bool isInTheAir(const Fighter& f) {
        return f.getState() == FighterState::JUMP || f.getState() == FighterState::FALL;
    }
}

Enemy::Enemy(float startX, float startY)
    : Fighter(startX, startY, 50.0f, 100.0f),
      difficulty(GameMode::NORMAL),
      aiState(AIState::IDLE), attackCooldown(FIRST_ATTACK_DELAY),
      preferredGap(45.0f), lastAttack(AttackChoice::NONE),
      dodgePending(false), dodgeTimer(0.0f), airDrift(0.0f),
      targetWasAttacking(false), guardPending(false), reactionTimer(0.0f), guardTimer(0.0f) {
    setRole(FighterRole::ARMORED_SPEARMAN);
    setColor(0.13f, 0.08f, 0.08f);
    setAccentColor(0.95f, 0.45f, 0.15f);
    setFacingRight(false);
    setDifficulty(GameMode::NORMAL);
}

void Enemy::setDifficulty(GameMode mode) {
    difficulty = mode;
    switch (mode) {
        case GameMode::EASY:
            setDamageMultiplier(0.65f);
            break;
        case GameMode::NORMAL:
            setDamageMultiplier(1.00f);
            break;
        case GameMode::HARD:
            setDamageMultiplier(1.50f);
            break;
    }
}

GameMode Enemy::getDifficulty() const {
    return difficulty;
}

void Enemy::pickNewSpacing() {
    preferredGap = MIN_SPACING + randomFraction() * (MAX_SPACING - MIN_SPACING);
}

float Enemy::randomCooldown() const {
    if (difficulty == GameMode::EASY) {
        // Slow to attack, big recovery windows for player
        return 1.8f + randomFraction() * 1.5f;
    } else if (difficulty == GameMode::HARD) {
        // Aggressive, fast attack combos
        return 0.25f + randomFraction() * 0.50f;
    }
    // Normal: balanced
    int roll = std::rand() % 100;
    if (roll < 25) return 0.3f + randomFraction() * 0.3f;
    if (roll < 75) return 0.9f + randomFraction() * 0.9f;
    return 1.8f + randomFraction() * 0.8f;
}

void Enemy::watchForAttacks(const Fighter& target, float gap, float deltaTime) {
    const AttackData* theirAttack = target.getCurrentAttack();
    bool targetIsAttacking = (theirAttack != nullptr);

    int defendChance = 45;
    float minReact = 0.08f, maxReact = 0.16f;
    int dodgeChance = 40;
    float minDodge = 0.10f, maxDodge = 0.20f;

    if (difficulty == GameMode::EASY) {
        // Slow reaction and rarely blocks
        defendChance = 20;
        minReact = 0.30f;
        maxReact = 0.48f;
        dodgeChance = 15;
        minDodge = 0.30f;
        maxDodge = 0.45f;
    } else if (difficulty == GameMode::HARD) {
        // Lightning fast reflexes and frequent defense
        defendChance = 75;
        minReact = 0.02f;
        maxReact = 0.06f;
        dodgeChance = 70;
        minDodge = 0.03f;
        maxDodge = 0.08f;
    }

    if (targetIsAttacking && !targetWasAttacking &&
        gap < theirAttack->reach + NOTICE_MARGIN && canAct()) {

        if (!theirAttack->blockable) {
            if (!dodgePending && std::rand() % 100 < dodgeChance) {
                dodgePending = true;
                dodgeTimer = minDodge + randomFraction() * (maxDodge - minDodge);
            }
        } else if (!guardPending && guardTimer <= 0.0f) {
            if (std::rand() % 100 < defendChance) {
                guardPending  = true;
                reactionTimer = minReact + randomFraction() * (maxReact - minReact);
            }
        }
    }
    targetWasAttacking = targetIsAttacking;

    if (dodgePending) {
        dodgeTimer -= deltaTime;
        if (dodgeTimer <= 0.0f) {
            dodgePending = false;
            guardPending = false;
            guardTimer   = 0.0f;
            airDrift     = 0.0f;
            jump();
        }
    }

    if (!guardPending && guardTimer <= 0.0f && canAct() && gap < GUARD_URGE_RANGE) {
        float roll = std::rand() / (float)RAND_MAX;
        if (roll < GUARD_URGE_PER_SECOND * deltaTime) {
            guardTimer = MIN_GUARD_TIME + randomFraction() * (MAX_GUARD_TIME - MIN_GUARD_TIME);
        }
    }

    if (guardPending) {
        reactionTimer -= deltaTime;
        if (reactionTimer <= 0.0f) {
            guardPending = false;
            guardTimer = MIN_GUARD_TIME + randomFraction() * (MAX_GUARD_TIME - MIN_GUARD_TIME);
        }
    }

    if (guardTimer > 0.0f) guardTimer -= deltaTime;
}

Enemy::AttackChoice Enemy::chooseAttack(const Fighter& target, float gap) const {
    if (isInTheAir(target)) return AttackChoice::NONE;
    int punchWeight   = 0;
    int kickWeight    = 0;
    int specialWeight = 0;
    if (gap < Attacks::PUNCH.reach - REACH_MARGIN) punchWeight = 50;
    if (gap < Attacks::KICK.reach  - REACH_MARGIN) kickWeight  = 35;
    if (gap < Attacks::SPECIAL.reach - REACH_MARGIN &&
        getEnergy() >= Attacks::SPECIAL.energyCost) specialWeight = 15;
    if (target.getState() == FighterState::CROUCH) {
        punchWeight = 0;
        kickWeight *= 2;
    }
    if (lastAttack == AttackChoice::PUNCH)   punchWeight   = punchWeight   * 4 / 10;
    if (lastAttack == AttackChoice::KICK)    kickWeight    = kickWeight    * 4 / 10;
    if (lastAttack == AttackChoice::SPECIAL) specialWeight = specialWeight * 4 / 10;

    int total = punchWeight + kickWeight + specialWeight;
    if (total <= 0) return AttackChoice::NONE;
    int roll = std::rand() % total;
    if (roll < punchWeight) return AttackChoice::PUNCH;
    roll -= punchWeight;
    if (roll < kickWeight)  return AttackChoice::KICK;
    return AttackChoice::SPECIAL;
}

void Enemy::update(const Fighter& target, float deltaTime) {
    Rect me   = getHurtbox();
    Rect them = target.getHurtbox();

    float myCenter    = me.x   + me.width   / 2.0f;
    float theirCenter = them.x + them.width / 2.0f;
    float directionToTarget = (theirCenter > myCenter) ? 1.0f : -1.0f;
    float gap = std::fabs(theirCenter - myCenter) - (me.width + them.width) / 2.0f;
    if (canAct()) setFacingRight(directionToTarget > 0.0f);

    if (attackCooldown > 0.0f) attackCooldown -= deltaTime;

    watchForAttacks(target, gap, deltaTime);

    float walkDirection = 0.0f;

    if (isDead()) {
        aiState = AIState::DEAD;
    } else if (target.isDead()) {
        aiState = AIState::IDLE;
    } else if (getState() == FighterState::HIT) {
        aiState = AIState::HIT;
    } else if (isAttacking()) {
        aiState = AIState::ATTACK;
    } else if (isInTheAir(*this)) {
        aiState = AIState::JUMP;
        walkDirection = airDrift;
    } else if (guardTimer > 0.0f) {
        aiState = AIState::DEFEND;
    } else {
        bool attackedThisFrame = false;

        if (attackCooldown <= 0.0f) {
            AttackChoice choice = chooseAttack(target, gap);

            int hesitationChance = 25;
            if (difficulty == GameMode::EASY) hesitationChance = 40;
            else if (difficulty == GameMode::HARD) hesitationChance = 5;

            if (choice == AttackChoice::NONE) {
                if (!isInTheAir(target)) preferredGap = MIN_SPACING + randomFraction() * 15.0f;
            } else if (std::rand() % 100 < hesitationChance) {
                attackCooldown = 0.2f + randomFraction() * 0.4f;
            } else {
                if (choice == AttackChoice::PUNCH)   startPunch();
                if (choice == AttackChoice::KICK)    startKick();
                if (choice == AttackChoice::SPECIAL) startSpecial();

                if (isAttacking()) {
                    aiState = AIState::ATTACK;
                    lastAttack = choice;
                    attackCooldown = randomCooldown();
                    pickNewSpacing();
                    attackedThisFrame = true;
                }
            }
        }
        if (!attackedThisFrame) {
            float speedRatio = WALK_SPEED_RATIO;
            if (difficulty == GameMode::EASY) speedRatio *= 0.75f;
            else if (difficulty == GameMode::HARD) speedRatio *= 1.30f;

            if (gap > preferredGap) {
                aiState = AIState::APPROACH;
                walkDirection = directionToTarget * speedRatio;
                if (attackCooldown < ARRIVAL_PAUSE) attackCooldown = ARRIVAL_PAUSE;

                if (gap > APPROACH_JUMP_MIN_GAP &&
                    std::rand() / (float)RAND_MAX < APPROACH_JUMP_PER_SECOND * deltaTime) {
                    airDrift = directionToTarget * AIR_DRIFT;
                    jump();
                }
            } else if (gap < preferredGap - SPACING_BAND) {
                aiState = AIState::RETREAT;
                walkDirection = -directionToTarget * RETREAT_SPEED_RATIO;
            } else {
                aiState = AIState::IDLE;
            }
        }
    }

    setBlocking(aiState == AIState::DEFEND);
    moveHorizontal(walkDirection, deltaTime);

    if (canAct()) setFacingRight(directionToTarget > 0.0f);

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
        case AIState::RETREAT:  return "RETREAT";
        case AIState::DEFEND:   return "DEFEND";
        case AIState::JUMP:     return "JUMP";
        case AIState::ATTACK:   return "ATTACK";
        case AIState::HIT:      return "HIT";
        case AIState::DEAD:     return "DEAD";
    }
    return "UNKNOWN";
}
