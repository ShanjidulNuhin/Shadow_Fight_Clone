#ifndef ENEMY_H
#define ENEMY_H

#include "Fighter.h"

enum class AIState {
    IDLE,
    APPROACH,
    RETREAT,
    DEFEND,
    JUMP,
    ATTACK,
    HIT,
    DEAD
};

class Enemy : public Fighter {
public:
    Enemy(float startX, float startY);

    void update(const Fighter& target, float deltaTime);

    AIState     getAIState() const;
    const char* getAIStateName() const;

private:
    enum class AttackChoice { NONE, PUNCH, KICK, SPECIAL };

    AttackChoice chooseAttack(const Fighter& target, float gap) const;
    float        randomCooldown() const;
    void         pickNewSpacing();
    void         watchForAttacks(const Fighter& target, float gap, float deltaTime);

    AIState      aiState;
    float        attackCooldown;
    float        preferredGap;
    AttackChoice lastAttack;

    // jumping
    bool  dodgePending;
    float dodgeTimer;
    float airDrift;
    // defending
    bool  targetWasAttacking;
    bool  guardPending;
    float reactionTimer;
    float guardTimer;
};

#endif
