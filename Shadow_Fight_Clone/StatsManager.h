#ifndef STATSMANAGER_H
#define STATSMANAGER_H

#include "CharacterData.h"
#include "Config.h"

struct CareerStats {
    int   highScore;
    long long totalScore;
    int   matchesPlayed;
    int   matchesWon;
    int   matchesLost;
    int   roundsWon;
    int   roundsLost;
    int   currentWinStreak;
    int   bestWinStreak;
    int   totalHitsLanded;
    int   totalSpecialsLanded;
    int   totalDodges;
    int   totalBlocks;
    int   totalKnockouts;
    int   perfectRounds;
    long long totalDamageDealt;
    long long totalDamageTaken;
    float fastestRoundWinSec; // 0.0f if no wins recorded yet
    int   charMatches[Characters::COUNT];
    int   charWins[Characters::COUNT];
};

struct MatchStats {
    int   score;
    int   hitsLanded;
    int   specialsLanded;
    int   dodges;
    int   blocks;
    int   damageDealt;
    int   damageTaken;
    int   perfectRounds;
    float fastestRoundSec;
    bool  isNewHighScore;
};

class StatsManager {
public:
    StatsManager();

    void load();
    void save() const;
    void resetAll();

    void beginMatch();
    void beginRound();

    void recordPlayerHit(int damage, bool wasSpecial, GameMode mode);
    void recordPlayerDodged(GameMode mode);
    void recordPlayerBlocked(int chipDamage, GameMode mode);
    void recordEnemyHitPlayer(int damage);

    void recordRoundEnd(bool playerWonRound, bool enemyDown, int playerHP, int playerMaxHP,
                        float roundElapsedSec, GameMode mode);
    void recordMatchEnd(bool playerWonMatch, CharacterId playerChar, GameMode mode);

    const CareerStats& getCareer() const;
    const MatchStats&  getMatch() const;

private:
    CareerStats career;
    MatchStats  match;
    int         previousHighScore;

    float difficultyMultiplier(GameMode mode) const;
};

#endif // STATSMANAGER_H
