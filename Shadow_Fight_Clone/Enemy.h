#ifndef ENEMY_H
#define ENEMY_H

#include "Fighter.h"

// What the enemy's brain is currently trying to do.
// (This is separate from FighterState, which is what the BODY is doing.)
enum class AIState {
    IDLE,       // waiting for its next move
    APPROACH,   // walking toward the player
    RETREAT,    // backing away to get some space
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
    enum class AttackChoice { NONE, PUNCH, KICK, SPECIAL };

    AttackChoice chooseAttack(const Fighter& target, float gap) const;
    float        randomCooldown() const;
    void         pickNewSpacing();

    AIState      aiState;
    float        attackCooldown;   // seconds until the enemy may attack again
    float        preferredGap;     // the distance it wants to stand at right now (changes often)
    AttackChoice lastAttack;       // what it did last time (so it can avoid repeating itself)
};

#endif
