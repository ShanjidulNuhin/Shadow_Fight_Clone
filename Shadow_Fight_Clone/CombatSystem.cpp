#include "CombatSystem.h"
#include "Fighter.h"
#include "Collision.h"

namespace CombatSystem {

bool resolveAttack(Fighter& attacker, Fighter& defender) {
    if (defender.isDead()) return false;

    // Does the attacker have a live hitbox right now?
    Rect hitbox;
    if (!attacker.getHitbox(hitbox)) return false;

    // Does that hitbox touch the defender's hurtbox?
    Rect defenderBody = defender.getHurtbox();
    if (!Collision::overlaps(hitbox, defenderBody)) return false;

    // A hit! Knock the defender away from the attacker.
    Rect attackerBody = attacker.getHurtbox();
    float attackerCenter = attackerBody.x + attackerBody.width / 2.0f;
    float defenderCenter = defenderBody.x + defenderBody.width / 2.0f;
    float pushDirection = (defenderCenter > attackerCenter) ? 1.0f : -1.0f;

    defender.receiveHit(attacker.getAttackDamage(), pushDirection);

    // Use up this swing so it can't hit again.
    attacker.onAttackConnected();
    return true;
}

}   // namespace CombatSystem
