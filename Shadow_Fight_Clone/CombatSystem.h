#ifndef COMBATSYSTEM_H
#define COMBATSYSTEM_H

class Fighter;   // forward declaration

namespace CombatSystem {
    // Checks whether the attacker's current attack hits the defender and,
    // if it does, applies the damage. Call once per frame.
    // Returns true if a hit landed this frame.
    bool resolveAttack(Fighter& attacker, Fighter& defender);
}

#endif
