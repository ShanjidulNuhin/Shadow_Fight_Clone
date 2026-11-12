#ifndef COMBATSYSTEM_H
#define COMBATSYSTEM_H

class Fighter;
class Effects;

struct HitOutcome {
    bool connected;
    bool dodged;
    bool blocked;
    bool wasSpecial;
    int  damageDealt;
};

namespace CombatSystem {
    bool resolveAttack(Fighter& attacker, Fighter& defender, Effects* effects = nullptr, HitOutcome* outOutcome = nullptr);
    void separateFighters(Fighter& a, float aXBefore, Fighter& b, float bXBefore);
}

#endif
