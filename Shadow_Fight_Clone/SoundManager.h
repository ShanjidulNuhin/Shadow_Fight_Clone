#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

enum class SoundEffect {
    PUNCH_WHOOSH,
    KICK_WHOOSH,
    SPECIAL_CHARGE,
    SPECIAL_WHOOSH,

    HIT_PUNCH,
    HIT_KICK,
    HIT_SPECIAL,
    HIT_BLOCK,
    KO,

    JUMP,
    LAND,
    BLOCK_GUARD,

    ROUND_START,
    FIGHT_START,
    VICTORY,
    DEFEAT,

    UI_CLICK,
    UI_BACK,
    UI_START
};

namespace SoundManager {
    void init();
    void shutdown();
    void play(SoundEffect sfx, float volume = 1.0f);
}

#endif // SOUNDMANAGER_H
