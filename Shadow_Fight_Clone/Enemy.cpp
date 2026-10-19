#include "Enemy.h"
#include <cmath>
#include <cstdlib>

namespace {
    const float WALK_SPEED_RATIO    = 0.6f;   // approaching: 60% of full walking speed
    const float RETREAT_SPEED_RATIO = 0.4f;   // backing away: slower still

    // Spacing: the distance (empty space between bodies) it likes to fight at.
    // A new random value in this range is picked after every attack.
    const float MIN_SPACING   = 30.0f;
    const float MAX_SPACING   = 62.0f;
    const float SPACING_BAND  = 4.0f;         // closer than (spacing - band) => back away a little

    // Timing
    const float FIRST_ATTACK_DELAY = 1.5f;    // breathing room at the start of the fight
    const float ARRIVAL_PAUSE      = 0.3f;    // short pause after walking up to you
    const int   HESITATION_PERCENT = 25;      // chance of "second thoughts" before an attack

    // Attacks only count as "in range" if the gap is a little SHORTER than their reach.
    const float REACH_MARGIN = 5.0f;

    // A random number from 0.0 up to 1.0
    float randomFraction() {
        return (std::rand() % 1000) / 1000.0f;
    }

    bool isInTheAir(const Fighter& f) {
        return f.getState() == FighterState::JUMP || f.getState() == FighterState::FALL;
    }
}

Enemy::Enemy(float startX, float startY)
    : Fighter(startX, startY, 50.0f, 100.0f),
      aiState(AIState::IDLE), attackCooldown(FIRST_ATTACK_DELAY),
      preferredGap(45.0f), lastAttack(AttackChoice::NONE) {
    setColor(0.95f, 0.55f, 0.20f);   // orange, so it's easy to tell apart from the player
    setFacingRight(false);           // starts out facing left, toward the player
}

void Enemy::pickNewSpacing() {
    preferredGap = MIN_SPACING + randomFraction() * (MAX_SPACING - MIN_SPACING);
}

// Time to wait after an attack. Sometimes short (a quick follow-up), usually
// medium, sometimes long (a slow, hesitant enemy). The player can't rely on a rhythm.
float Enemy::randomCooldown() const {
    int roll = std::rand() % 100;
    if (roll < 25) return 0.3f + randomFraction() * 0.3f;   // 25%: quick follow-up
    if (roll < 75) return 1.0f + randomFraction() * 1.0f;   // 50%: normal
    return 2.2f + randomFraction() * 1.0f;                  // 25%: slow
}

// Pick an attack that makes sense RIGHT NOW (or NONE if nothing is worth trying).
Enemy::AttackChoice Enemy::chooseAttack(const Fighter& target, float gap) const {
    // Swinging at someone who is jumping is mostly wasted: wait for them to land.
    if (isInTheAir(target)) return AttackChoice::NONE;

    // Each attack is only an option if the target is within its reach.
    int punchWeight   = 0;
    int kickWeight    = 0;
    int specialWeight = 0;
    if (gap < Attacks::PUNCH.reach - REACH_MARGIN) punchWeight = 50;
    if (gap < Attacks::KICK.reach  - REACH_MARGIN) kickWeight  = 35;
    if (gap < Attacks::SPECIAL.reach - REACH_MARGIN &&
        getEnergy() >= Attacks::SPECIAL.energyCost) specialWeight = 15;

    // A punch flies over the head of a crouching player: use kicks instead.
    if (target.getState() == FighterState::CROUCH) {
        punchWeight = 0;
        kickWeight *= 2;
    }

    // Avoid doing the same thing twice in a row (it only gets 40% of its normal chance).
    if (lastAttack == AttackChoice::PUNCH)   punchWeight   = punchWeight   * 4 / 10;
    if (lastAttack == AttackChoice::KICK)    kickWeight    = kickWeight    * 4 / 10;
    if (lastAttack == AttackChoice::SPECIAL) specialWeight = specialWeight * 4 / 10;

    int total = punchWeight + kickWeight + specialWeight;
    if (total <= 0) return AttackChoice::NONE;

    // Weighted dice roll: bigger weight = bigger slice of the 0..total range.
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
    } else {
        bool attackedThisFrame = false;

        // 1) Is it time to attack?
        if (attackCooldown <= 0.0f) {
            AttackChoice choice = chooseAttack(target, gap);

            if (choice == AttackChoice::NONE) {
                // Nothing in reach. If the target is on the ground, decide to stand closer.
                if (!isInTheAir(target)) preferredGap = MIN_SPACING + randomFraction() * 15.0f;
            } else if (std::rand() % 100 < HESITATION_PERCENT) {
                attackCooldown = 0.2f + randomFraction() * 0.4f;   // second thoughts: wait a bit more
            } else {
                if (choice == AttackChoice::PUNCH)   startPunch();
                if (choice == AttackChoice::KICK)    startKick();
                if (choice == AttackChoice::SPECIAL) startSpecial();

                if (isAttacking()) {                 // (starting can fail, e.g. while still landing)
                    aiState = AIState::ATTACK;
                    lastAttack = choice;
                    attackCooldown = randomCooldown();
                    pickNewSpacing();                // next fight position will be different
                    attackedThisFrame = true;
                }
            }
        }

        // 2) Otherwise, adjust the distance to wherever it wants to stand.
        if (!attackedThisFrame) {
            if (gap > preferredGap) {
                aiState = AIState::APPROACH;
                walkDirection = directionToTarget * WALK_SPEED_RATIO;
                if (attackCooldown < ARRIVAL_PAUSE) attackCooldown = ARRIVAL_PAUSE;
            } else if (gap < preferredGap - SPACING_BAND) {
                aiState = AIState::RETREAT;
                walkDirection = -directionToTarget * RETREAT_SPEED_RATIO;
            } else {
                aiState = AIState::IDLE;
            }
        }
    }

    moveHorizontal(walkDirection, deltaTime);

    // Walking backwards turns the body around, so look at the target again.
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
        case AIState::ATTACK:   return "ATTACK";
        case AIState::HIT:      return "HIT";
        case AIState::DEAD:     return "DEAD";
    }
    return "UNKNOWN";
}
