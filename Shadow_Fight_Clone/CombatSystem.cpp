#include "CombatSystem.h"
#include "Fighter.h"
#include "Collision.h"
#include "Effects.h"
#include "SoundManager.h"
#include <cmath>
#include <algorithm>

namespace CombatSystem {

bool resolveAttack(Fighter& attacker, Fighter& defender, Effects* effects, HitOutcome* outOutcome) {
    if (outOutcome != nullptr) {
        *outOutcome = { false, false, false, false, 0 };
    }
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

    float hitX = (hitbox.x + hitbox.width * 0.5f + defenderBody.x + defenderBody.width * 0.5f) * 0.5f;
    float overlapYMin = std::max(hitbox.y, defenderBody.y);
    float overlapYMax = std::min(hitbox.y + hitbox.height, defenderBody.y + defenderBody.height);
    float hitY = (overlapYMin + overlapYMax) * 0.5f;

    FighterState atkState = attacker.getState();

    // Rolling fighters completely dodge punches, combos, and spiked chain grabs!
    bool dodgeableStrike = (atkState == FighterState::PUNCH ||
                            atkState == FighterState::COMBO2 ||
                            atkState == FighterState::COMBO3 ||
                            atkState == FighterState::CHAIN_GRAB);
    if (defender.isRolling() && dodgeableStrike) {
        attacker.onAttackConnected();
        if (effects != nullptr) {
            effects->triggerHit(hitX, hitY, pushDirection, SparkStyle::DODGE);
        }
        SoundManager::play(SoundEffect::PUNCH_WHOOSH);
        if (outOutcome != nullptr) {
            outOutcome->dodged = true;
        }
        return false;
    }

    bool defenderRolling = defender.isRolling();
    bool wasBlocking = (!defenderRolling) && (defender.getState() == FighterState::BLOCK) && attack->blockable;

    float mult = attacker.getDamageMultiplier();
    int damageToApply = std::max(1, (int)std::round(attack->damage * mult));

    // Rolling against an opponent's Critical/Special attack still damages, but significantly less!
    float customStun = -1.0f;
    float knockScale = 1.0f;
    if (defenderRolling && atkState == FighterState::SPECIAL) {
        damageToApply = std::max(1, (int)std::round(damageToApply * 0.35f));
        customStun = 0.20f;
        knockScale = 0.40f;
    }

    bool isChainGrabHit = (atkState == FighterState::CHAIN_GRAB) && !wasBlocking && !defenderRolling;
    bool isElectroShock = (atkState == FighterState::SPECIAL) &&
                          (attacker.getCharacterId() == CharacterId::CHAIN_WARRIOR) &&
                          !wasBlocking && !defenderRolling;

    if (isChainGrabHit) {
        // Pull the victim close in front of Kuro and stun for 2.0 seconds
        float pullTargetCenter = attackerCenter + attacker.getDirection() * (attackerBody.width * 0.5f + defenderBody.width * 0.5f + 14.0f);
        defender.pushHorizontally(pullTargetCenter - defenderCenter);
        customStun = 2.00f;
        knockScale = 0.05f;
    } else if (isElectroShock) {
        // Snare into mid-close range and jolt with electric shock
        float snareTargetCenter = attackerCenter + attacker.getDirection() * (attackerBody.width * 0.5f + defenderBody.width * 0.5f + 42.0f);
        if (std::fabs(defenderCenter - attackerCenter) > std::fabs(snareTargetCenter - attackerCenter)) {
            defender.pushHorizontally(snareTargetCenter - defenderCenter);
        }
        defender.applyElectrocution(0.75f);
        customStun = 1.15f;
        knockScale = 0.35f;
    }

    int hpBefore = defender.getHealth();
    defender.receiveHit(*attack, pushDirection, damageToApply, customStun, knockScale);
    int actualDamage = std::max(0, hpBefore - defender.getHealth());

    if (outOutcome != nullptr) {
        outOutcome->connected   = true;
        outOutcome->blocked     = wasBlocking;
        outOutcome->wasSpecial  = (atkState == FighterState::SPECIAL || atkState == FighterState::CHAIN_GRAB);
        outOutcome->damageDealt = actualDamage;
    }

    if (effects != nullptr) {
        SparkStyle style = SparkStyle::PUNCH;
        if (defender.isDead()) {
            style = SparkStyle::KO;
        } else if (wasBlocking) {
            style = SparkStyle::BLOCK;
        } else if (isElectroShock) {
            style = SparkStyle::ELECTRIC;
        } else if (atkState == FighterState::KICK || atkState == FighterState::JUMP_KICK) {
            style = SparkStyle::KICK;
        } else if (atkState == FighterState::SPECIAL || atkState == FighterState::COMBO3) {
            style = SparkStyle::SPECIAL;
        }

        effects->triggerHit(hitX, hitY, pushDirection, style);
    }

    if (defender.isDead()) {
        SoundManager::play(SoundEffect::KO);
    } else if (defender.getState() == FighterState::BLOCK) {
        SoundManager::play(SoundEffect::HIT_BLOCK);
    } else if (atkState == FighterState::KICK || atkState == FighterState::JUMP_KICK) {
        SoundManager::play(SoundEffect::HIT_KICK);
    } else if (atkState == FighterState::SPECIAL || atkState == FighterState::COMBO3 || isChainGrabHit) {
        SoundManager::play(SoundEffect::HIT_SPECIAL);
    } else {
        SoundManager::play(SoundEffect::HIT_PUNCH);
    }

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

} // namespace CombatSystem
