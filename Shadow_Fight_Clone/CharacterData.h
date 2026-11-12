#ifndef CHARACTERDATA_H
#define CHARACTERDATA_H

#include "AttackData.h"

enum class CharacterId {
    BRAWLER = 0,     // 1: Bare-hand fighter (combos, jump-kick, dodge roll, high-power punch)
    CHAIN_WARRIOR,   // 2: Spiked Chain warrior (long grab+2s stun, electric chain critical)
    ARCHER,          // 3: Female Archer (same skills as 1st, back-roll big arrow special)
    FAN_DANCER,      // 4: Female Dual Chinese Fans (same skills, 2 fans -> run/jump/head-grab knee)
    SPEAR_WARRIOR,   // 5: Pure Spear warrior (no hands used, 10 sky spears + final force throw)
    COUNT
};

enum class WeaponType {
    NONE_UNARMED,
    SPIKED_CHAIN,
    WAR_BOW,
    DUAL_FANS,
    WAR_SPEAR
};

struct CharacterProfile {
    CharacterId id;
    const char* name;
    const char* title;
    const char* weaponName;
    const char* styleName;
    const char* specialName;
    const char* specialDesc;
    const char* skillDesc;
    const char* statsSummary;

    WeaponType  weapon;
    bool        isFemale;
    bool        hasAcrobaticKit;  // Flying jump-kick + low dodge roll (Ch 1, 3, 4)
    bool        hasChainPull;     // Long-range spiked chain grab + 2s stun on O (Ch 2)
    bool        spearOnlyStyle;   // Never uses bare hands in primary strikes (Ch 5)

    float       bodyR, bodyG, bodyB;
    float       accentR, accentG, accentB;
    float       bodyWidth;
    float       bodyHeight;

    int         maxHealth;
    float       moveSpeed;
    float       jumpSpeed;
    float       energyRegen;

    AttackData  comboAttacks[3];  // 3-hit martial combo (J -> J -> J)
    AttackData  kickAttack;       // Ground kick (K)
    AttackData  jumpKickAttack;   // Mid-air flying kick (W + K, higher damage for Ch 1, 3, 4)
    AttackData  grabAttack;       // Spiked chain pull attack (O, Ch 2)
    AttackData  specialAttack;    // Signature Critical / Special Attack (L)
};

namespace Characters {
    const int COUNT = (int)CharacterId::COUNT;
    const CharacterProfile& get(CharacterId id);
    const CharacterProfile& getByIndex(int index);
}

#endif // CHARACTERDATA_H
