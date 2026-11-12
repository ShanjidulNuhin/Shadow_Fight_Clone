#ifndef ENEMY_H
#define ENEMY_H

#include "Fighter.h"
#include "Config.h"

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
    Enemy(float startX, float startY, CharacterId charId = CharacterId::CHAIN_WARRIOR);

    void update(const Fighter& target, float deltaTime);

    void     setDifficulty(GameMode mode);
    GameMode getDifficulty() const;

    AIState     getAIState() const;
    const char* getAIStateName() const;

private:
    enum class AttackChoice { NONE, PUNCH, KICK, CHAIN_GRAB, SPECIAL };

    AttackChoice chooseAttack(const Fighter& target, float gap) const;
    float        randomCooldown() const;
    void         pickNewSpacing();
    void         watchForAttacks(const Fighter& target, float gap, float directionToTarget, float deltaTime);

    GameMode     difficulty;

    AIState      aiState;
    float        attackCooldown;
    float        preferredGap;
    AttackChoice lastAttack;
    int          pendingComboHits;

    // jumping / dodge
    bool  dodgePending;
    float dodgeTimer;
    bool  rollPending;
    float airDrift;
    // defending
    bool  targetWasAttacking;
    bool  guardPending;
    float reactionTimer;
    float guardTimer;
};

#endif
