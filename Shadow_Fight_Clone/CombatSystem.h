#ifndef COMBATSYSTEM_H
#define COMBATSYSTEM_H

class Fighter;
class Effects;

namespace CombatSystem {
    bool resolveAttack(Fighter& attacker, Fighter& defender, Effects* effects = nullptr);
    void separateFighters(Fighter& a, float aXBefore, Fighter& b, float bXBefore);
}

#endif
