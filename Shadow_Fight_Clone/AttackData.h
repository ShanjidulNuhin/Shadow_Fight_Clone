#ifndef ATTACKDATA_H
#define ATTACKDATA_H

// Everything that makes one attack different from another.
struct AttackData {
    // timing, in seconds
    float startup;        // winding up: can't hurt anyone yet
    float active;         // the dangerous moment
    float recovery;       // pulling back: can't act yet

    // shape of the hitbox
    float reach;          // how far it sticks out in front of the body
    float hitboxHeight;
    float hitboxOffsetY;  // how far above the attacker's FEET the hitbox starts

    // effect on the victim
    int   damage;
    float knockbackSpeed; // sideways push (pixels/second)
    float hitStun;        // seconds the victim can't act

    // price of using it
    float energyCost;

    float totalTime() const { return startup + active + recovery; }
};

namespace Attacks {
    //                                 startup active recovery  reach height offsetY  dmg  knock  stun  energy
    const AttackData PUNCH   = {       0.08f,  0.10f, 0.17f,    50.0f, 24.0f, 70.0f,  10, 400.0f, 0.30f,   0.0f };
    const AttackData KICK    = {       0.14f,  0.10f, 0.26f,    70.0f, 30.0f,  5.0f,  15, 520.0f, 0.40f,   0.0f };
    const AttackData SPECIAL = {       0.25f,  0.12f, 0.35f,    90.0f, 70.0f, 20.0f,  30, 700.0f, 0.60f,  50.0f };
}

#endif
