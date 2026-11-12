#include "CharacterData.h"

namespace {
    const CharacterProfile PROFILES[Characters::COUNT] = {
        // 0: KAI — Bare-Hand Martial Artist
        {
            CharacterId::BRAWLER,
            "KAI",
            "SHADOW BRAWLER",
            "None (Bare Fists & Feet)",
            "Unarmed Kung Fu (3-Hit Combo)",
            "Meteor Iron Fist",
            "Devastating high-power charged punch.",
            "Skill: Jump-Kick (W+K), Roll (U) dodges Punch/Chain",
            "HP: 100  |  Muscular  |  Unarmed Blitz",
            WeaponType::NONE_UNARMED,
            false, true, false, false,
            0.025f, 0.025f, 0.032f,
            0.25f, 0.85f, 0.95f, // Cyan focus glow
            92.0f, 188.0f,
            100, 295.0f, 790.0f, 10.5f,
            // 3-hit combo: Lead Jab -> Cross Hook -> Tiger Uppercut
            {
                { 0.05f, 0.08f, 0.14f,  86.0f, 44.0f, 118.0f,  8, 220.0f, 0.26f, 0.0f, true, 0.20f },
                { 0.06f, 0.09f, 0.15f,  94.0f, 48.0f, 114.0f, 11, 280.0f, 0.30f, 0.0f, true, 0.20f },
                { 0.09f, 0.11f, 0.22f, 104.0f, 64.0f, 105.0f, 16, 580.0f, 0.42f, 0.0f, true, 0.25f }
            },
            // Ground Kick
            { 0.20f, 0.10f, 0.20f, 108.0f, 54.0f, 22.0f, 13, 500.0f, 0.36f, 0.0f, false, 0.20f },
            // Flying Jump Kick (higher damage than normal kick!)
            { 0.08f, 0.26f, 0.14f, 118.0f, 58.0f, 20.0f, 21, 620.0f, 0.46f, 0.0f, false, 0.20f },
            // Grab (unused)
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f, false, 0.0f },
            // Special Critical: Meteor Iron Fist (High-Power Punch)
            { 0.24f, 0.14f, 0.30f, 140.0f, 85.0f, 80.0f, 35, 820.0f, 0.65f, 50.0f, true, 0.35f }
        },

        // 1: KURO — Spiked Chain Warrior
        {
            CharacterId::CHAIN_WARRIOR,
            "KURO",
            "CHAIN REAVER",
            "Spiked Iron Chain",
            "Chain Whip Combos & Hook Pull",
            "Electro-Chain Shock",
            "Energizes chain, grabs & shocks opponent.",
            "Skill (O): Long Chain Grab & Pull + 2s Stun",
            "HP: 110  |  Strong Build |  Chain Stun",
            WeaponType::SPIKED_CHAIN,
            false, false, true, false,
            0.025f, 0.025f, 0.032f,
            0.35f, 0.78f, 1.0f, // Electric blue-cyan
            94.0f, 190.0f,
            110, 260.0f, 755.0f, 10.0f,
            // 3-hit combo: Forward Lash -> Reverse Whip -> Overhead Spike Smash
            {
                { 0.08f, 0.10f, 0.16f, 132.0f, 48.0f, 112.0f, 10, 260.0f, 0.28f, 0.0f, true, 0.20f },
                { 0.08f, 0.10f, 0.17f, 142.0f, 52.0f, 108.0f, 12, 300.0f, 0.32f, 0.0f, true, 0.20f },
                { 0.12f, 0.12f, 0.24f, 152.0f, 70.0f,  92.0f, 17, 600.0f, 0.44f, 0.0f, true, 0.25f }
            },
            // Ground Kick
            { 0.24f, 0.10f, 0.22f, 108.0f, 54.0f, 22.0f, 13, 520.0f, 0.38f, 0.0f, false, 0.20f },
            // Jump Kick (unused)
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f, false, 0.0f },
            // Long-Distance Chain Grab & Pull + 2.0s Stun (O)
            { 0.14f, 0.18f, 0.26f, 330.0f, 54.0f, 100.0f, 8, 60.0f, 2.00f, 0.0f, true, 0.0f },
            // Special Critical: Electro-Chain Shock
            { 0.26f, 0.22f, 0.34f, 245.0f, 80.0f, 85.0f, 34, 480.0f, 1.15f, 50.0f, true, 0.35f }
        },

        // 2: AKARI — Female Shadow Archer
        {
            CharacterId::ARCHER,
            "AKARI",
            "SHADOW ARCHER",
            "Shadow War Bow & Arrows",
            "Bow-Fu Combos (Roll + Air Kick)",
            "Evade War-Arrow",
            "Rolls back for range & fires a power arrow.",
            "Skill: Jump-Kick (W+K), Roll (U) dodges Punch/Chain",
            "HP: 95   |  Very Fast |  Bow & Arrow",
            WeaponType::WAR_BOW,
            true, true, false, false,
            0.025f, 0.025f, 0.032f,
            0.95f, 0.72f, 0.25f, // Amber-gold bow aura
            80.0f, 176.0f,
            95, 355.0f, 835.0f, 10.5f,
            // 3-hit combo: Bow Strike -> Arrow Slash -> Point-Blank Piercing Shot
            {
                { 0.06f, 0.09f, 0.14f,  96.0f, 46.0f, 114.0f,  8, 240.0f, 0.26f, 0.0f, true, 0.20f },
                { 0.07f, 0.09f, 0.16f, 106.0f, 50.0f, 110.0f, 10, 290.0f, 0.30f, 0.0f, true, 0.20f },
                { 0.10f, 0.11f, 0.22f, 138.0f, 52.0f, 106.0f, 15, 580.0f, 0.40f, 0.0f, true, 0.25f }
            },
            // Ground Kick
            { 0.20f, 0.10f, 0.20f, 106.0f, 54.0f, 22.0f, 12, 500.0f, 0.36f, 0.0f, false, 0.20f },
            // Flying Jump Kick
            { 0.08f, 0.26f, 0.14f, 116.0f, 58.0f, 20.0f, 20, 620.0f, 0.45f, 0.0f, false, 0.20f },
            // Grab (unused)
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f, false, 0.0f },
            // Special Critical: Back-Roll + Giant Power Arrow
            { 0.56f, 0.16f, 0.28f, 110.0f, 60.0f, 95.0f, 34, 760.0f, 0.62f, 50.0f, true, 0.30f }
        },

        // 3: MEI-LING — Female Twin Fan Dancer
        {
            CharacterId::FAN_DANCER,
            "MEI-LING",
            "FAN DANCER",
            "Twin Chinese War Fans",
            "Fan Dance Combos (Roll + Air Kick)",
            "Fan Rush Knee",
            "Throws 2 fans -> sprints, grabs head & knees!",
            "Skill: Jump-Kick (W+K), Roll (U) dodges Punch/Chain",
            "HP: 95   |  Max Agile |  Twin War Fans",
            WeaponType::DUAL_FANS,
            true, true, false, false,
            0.025f, 0.025f, 0.032f,
            0.96f, 0.28f, 0.48f, // Crimson lotus rose
            80.0f, 176.0f,
            95, 370.0f, 845.0f, 11.0f,
            // 3-hit combo: Fan Crescent -> Cross Scissor -> Spinning Cyclone Fans
            {
                { 0.05f, 0.09f, 0.14f, 102.0f, 48.0f, 112.0f,  9, 240.0f, 0.26f, 0.0f, true, 0.20f },
                { 0.06f, 0.09f, 0.15f, 112.0f, 54.0f, 108.0f, 11, 290.0f, 0.30f, 0.0f, true, 0.20f },
                { 0.09f, 0.11f, 0.21f, 124.0f, 68.0f,  98.0f, 15, 560.0f, 0.42f, 0.0f, true, 0.25f }
            },
            // Ground Kick
            { 0.20f, 0.10f, 0.20f, 106.0f, 54.0f, 22.0f, 12, 500.0f, 0.36f, 0.0f, false, 0.20f },
            // Flying Jump Kick
            { 0.08f, 0.26f, 0.14f, 116.0f, 58.0f, 20.0f, 20, 620.0f, 0.45f, 0.0f, false, 0.20f },
            // Grab (unused)
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f, false, 0.0f },
            // Special Critical: Twin Fan Throw -> Run, Jump, Head-Grab Knee!
            { 0.12f, 0.85f, 0.25f, 95.0f, 85.0f, 90.0f, 26, 760.0f, 0.65f, 50.0f, true, 0.30f }
        },

        // 4: TAKEDA — Pure Spear Warrior
        {
            CharacterId::SPEAR_WARRIOR,
            "TAKEDA",
            "SKY SPEARMAN",
            "Imperial War Spear",
            "Pure Spear Art (No Hands Used)",
            "10-Spear Sky Storm",
            "Raises spear -> 10 sky spears -> force throw!",
            "Pure Spear Combos (J), Kick (K), Block (I)",
            "HP: 125  |  Heavy Muscle |  Spear Master",
            WeaponType::WAR_SPEAR,
            false, false, false, true,
            0.025f, 0.025f, 0.032f,
            1.0f, 0.58f, 0.15f, // Burning imperial gold-amber
            104.0f, 196.0f,
            125, 232.0f, 715.0f, 10.0f,
            // 3-hit combo (100% spear strikes, no fists): Jab -> Overhead Cleave -> Impaler Thrust
            {
                { 0.08f, 0.10f, 0.17f, 138.0f, 44.0f, 114.0f, 10, 270.0f, 0.28f, 0.0f, true, 0.20f },
                { 0.09f, 0.10f, 0.18f, 148.0f, 64.0f,  98.0f, 12, 320.0f, 0.33f, 0.0f, true, 0.20f },
                { 0.12f, 0.12f, 0.24f, 165.0f, 52.0f, 106.0f, 17, 630.0f, 0.45f, 0.0f, true, 0.28f }
            },
            // Ground Kick
            { 0.24f, 0.10f, 0.22f, 112.0f, 54.0f, 22.0f, 13, 540.0f, 0.38f, 0.0f, false, 0.20f },
            // Jump Kick (unused)
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f, false, 0.0f },
            // Grab (unused)
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f, false, 0.0f },
            // Special Critical: Spear Raise + 10 Sky Spears + Main Spear Throw
            { 0.42f, 1.38f, 0.30f, 130.0f, 65.0f, 90.0f, 16, 860.0f, 0.65f, 50.0f, true, 0.35f }
        }
    };
}

namespace Characters {
    const int COUNT_CHECK = ((int)CharacterId::COUNT == 5) ? 5 : -1;

    const CharacterProfile& get(CharacterId id) {
        int idx = (int)id;
        if (idx < 0 || idx >= COUNT) idx = 0;
        return PROFILES[idx];
    }

    const CharacterProfile& getByIndex(int index) {
        if (index < 0 || index >= COUNT) index = 0;
        return PROFILES[index];
    }
}
