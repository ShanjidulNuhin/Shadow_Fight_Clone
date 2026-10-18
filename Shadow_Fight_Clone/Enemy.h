#ifndef ENEMY_H
#define ENEMY_H

#include "Fighter.h"

// What the enemy's brain is currently trying to do.
// (This is separate from FighterState, which is what the BODY is doing.)
enum class AIState {
    IDLE,       // close enough, waiting for its next attack
    APPROACH,   // walking toward the player
    ATTACK,     // swinging an attack
    HIT,        // stunned: the brain waits until the body recovers
    DEAD        // out of health: does nothing
};

// The computer-controlled opponent. It is a Fighter, so it moves, jumps, attacks,
// blocks and takes hits exactly like the Player does. Enemy only adds the "brain".
class Enemy : public Fighter {
public:
    Enemy(float startX, float startY);

    // Decide what to do this frame (looking at the target), then move and update.
    void update(const Fighter& target, float deltaTime);

    AIState     getAIState() const;
    const char* getAIStateName() const;   // e.g. "APPROACH", handy for debugging

private:
    void startRandomAttack();

    AIState aiState;
    float   attackCooldown;   // seconds until the enemy is allowed to attack again
};

#endif
