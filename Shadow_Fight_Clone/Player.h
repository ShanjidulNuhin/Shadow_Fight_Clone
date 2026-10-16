#ifndef PLAYER_H
#define PLAYER_H

#include "Fighter.h"

class InputManager;   // forward declaration

// The fighter controlled by the human, using the keyboard.
class Player : public Fighter {
public:
    Player(float startX, float startY);

    // Reads the keyboard and moves the player accordingly.
    void update(const InputManager& input, float deltaTime);
};

#endif
