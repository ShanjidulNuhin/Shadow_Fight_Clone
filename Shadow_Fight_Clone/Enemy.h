#ifndef ENEMY_H
#define ENEMY_H

#include "Fighter.h"

// The computer-controlled opponent. It is a Fighter, so it moves, jumps, attacks,
// blocks and takes hits exactly like the Player does. Enemy only adds the "brain".
class Enemy : public Fighter {
public:
    Enemy(float startX, float startY);

    // Decide what to do this frame (looking at the target), then move and update.
    void update(const Fighter& target, float deltaTime);
};

#endif
