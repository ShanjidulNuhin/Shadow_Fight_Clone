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

    // can a guard stop it?
    bool  blockable;

    float totalTime() const { return startup + active + recovery; }
};

namespace Attacks {
    //                                 startup active recovery  reach   height offsetY  dmg  knock  stun  energy blockable
    const AttackData PUNCH   = {       0.08f,  0.10f, 0.17f,    85.0f,  44.0f, 120.0f,  10, 440.0f, 0.30f,   0.0f, true  };
    const AttackData KICK    = {       0.30f,  0.10f, 0.26f,   115.0f,  55.0f,  20.0f,  15, 560.0f, 0.40f,   0.0f, false };
    const AttackData SPECIAL = {       0.25f,  0.12f, 0.35f,   145.0f, 110.0f,  40.0f,  30, 750.0f, 0.60f,  50.0f, true  };
}

#endif
