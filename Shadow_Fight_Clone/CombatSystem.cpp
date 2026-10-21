#include "CombatSystem.h"
#include "Fighter.h"
#include "Collision.h"
#include <cmath>

namespace CombatSystem {

bool resolveAttack(Fighter& attacker, Fighter& defender) {
    if (defender.isDead()) return false;
    if (defender.isInvulnerable()) return false;
    Rect hitbox;
    if (!attacker.getHitbox(hitbox)) return false;

    const AttackData* attack = attacker.getCurrentAttack();
    if (attack == nullptr) return false;

    Rect defenderBody = defender.getHurtbox();
    if (!Collision::overlaps(hitbox, defenderBody)) return false;

    Rect attackerBody = attacker.getHurtbox();
    float attackerCenter = attackerBody.x + attackerBody.width / 2.0f;
    float defenderCenter = defenderBody.x + defenderBody.width / 2.0f;
    float pushDirection = (defenderCenter > attackerCenter) ? 1.0f : -1.0f;

    defender.receiveHit(*attack, pushDirection);

    attacker.onAttackConnected();
    return true;
}

void separateFighters(Fighter& a, float aXBefore, Fighter& b, float bXBefore) {
    if (a.isDead() || b.isDead()) return;

    Rect bodyA = a.getHurtbox();
    Rect bodyB = b.getHurtbox();
    if (!Collision::overlaps(bodyA, bodyB)) return;

    float centerA = bodyA.x + bodyA.width / 2.0f;
    float centerB = bodyB.x + bodyB.width / 2.0f;
    float towardB = (centerA <= centerB) ? 1.0f : -1.0f;

    float overlap = (bodyA.width + bodyB.width) / 2.0f - std::fabs(centerA - centerB);
    if (overlap <= 0.0f) return;

    float approachA = (bodyA.x - aXBefore) * towardB;
    float approachB = (bodyB.x - bXBefore) * (-towardB);
    if (approachA < 0.0f) approachA = 0.0f;
    if (approachB < 0.0f) approachB = 0.0f;
    float totalApproach = approachA + approachB;

    float remaining = overlap;

    if (totalApproach > 0.0f) {
        float share = overlap / totalApproach;
        if (share > 1.0f) share = 1.0f;
        remaining -= std::fabs(a.pushHorizontally(-towardB * approachA * share));
        remaining -= std::fabs(b.pushHorizontally( towardB * approachB * share));
    }

    if (remaining > 0.001f) {
        float half = remaining / 2.0f;
        float movedA = std::fabs(a.pushHorizontally(-towardB * half));
        float movedB = std::fabs(b.pushHorizontally( towardB * half));
        float left = remaining - movedA - movedB;
        if (left > 0.001f) {
            left -= std::fabs(a.pushHorizontally(-towardB * left));
            if (left > 0.001f) b.pushHorizontally(towardB * left);
        }
    }
}

}
