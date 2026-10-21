#ifndef COMBATSYSTEM_H
#define COMBATSYSTEM_H

class Fighter;
namespace CombatSystem {
    bool resolveAttack(Fighter& attacker, Fighter& defender);
    void separateFighters(Fighter& a, float aXBefore, Fighter& b, float bXBefore);
}

#endif
