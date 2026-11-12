#include "StatsManager.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace {
    const char* STATS_FILE = "shadow_fight_stats.dat";

    void clearCareer(CareerStats& c) {
        c.highScore           = 0;
        c.totalScore          = 0;
        c.matchesPlayed       = 0;
        c.matchesWon          = 0;
        c.matchesLost         = 0;
        c.roundsWon           = 0;
        c.roundsLost          = 0;
        c.currentWinStreak    = 0;
        c.bestWinStreak       = 0;
        c.totalHitsLanded     = 0;
        c.totalSpecialsLanded = 0;
        c.totalDodges         = 0;
        c.totalBlocks         = 0;
        c.totalKnockouts      = 0;
        c.perfectRounds       = 0;
        c.totalDamageDealt    = 0;
        c.totalDamageTaken    = 0;
        c.fastestRoundWinSec  = 0.0f;
        for (int i = 0; i < Characters::COUNT; ++i) {
            c.charMatches[i] = 0;
            c.charWins[i]    = 0;
        }
    }

    void clearMatch(MatchStats& m) {
        m.score           = 0;
        m.hitsLanded      = 0;
        m.specialsLanded  = 0;
        m.dodges          = 0;
        m.blocks          = 0;
        m.damageDealt     = 0;
        m.damageTaken     = 0;
        m.perfectRounds   = 0;
        m.fastestRoundSec = 0.0f;
        m.isNewHighScore  = false;
    }
}

StatsManager::StatsManager() : previousHighScore(0) {
    clearCareer(career);
    clearMatch(match);
}

float StatsManager::difficultyMultiplier(GameMode mode) const {
    switch (mode) {
        case GameMode::EASY:   return 1.00f;
        case GameMode::NORMAL: return 1.25f;
        case GameMode::HARD:   return 1.60f;
    }
    return 1.00f;
}

void StatsManager::load() {
    clearCareer(career);
    std::ifstream in(STATS_FILE);
    if (!in.is_open()) {
        previousHighScore = 0;
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        std::istringstream ss(val);

        if      (key == "highScore")           ss >> career.highScore;
        else if (key == "totalScore")          ss >> career.totalScore;
        else if (key == "matchesPlayed")       ss >> career.matchesPlayed;
        else if (key == "matchesWon")          ss >> career.matchesWon;
        else if (key == "matchesLost")         ss >> career.matchesLost;
        else if (key == "roundsWon")           ss >> career.roundsWon;
        else if (key == "roundsLost")          ss >> career.roundsLost;
        else if (key == "currentWinStreak")    ss >> career.currentWinStreak;
        else if (key == "bestWinStreak")       ss >> career.bestWinStreak;
        else if (key == "totalHitsLanded")     ss >> career.totalHitsLanded;
        else if (key == "totalSpecialsLanded") ss >> career.totalSpecialsLanded;
        else if (key == "totalDodges")         ss >> career.totalDodges;
        else if (key == "totalBlocks")         ss >> career.totalBlocks;
        else if (key == "totalKnockouts")      ss >> career.totalKnockouts;
        else if (key == "perfectRounds")       ss >> career.perfectRounds;
        else if (key == "totalDamageDealt")    ss >> career.totalDamageDealt;
        else if (key == "totalDamageTaken")    ss >> career.totalDamageTaken;
        else if (key == "fastestRoundWinSec")  ss >> career.fastestRoundWinSec;
        else {
            for (int i = 0; i < Characters::COUNT; ++i) {
                char mKey[32], wKey[32];
                std::snprintf(mKey, sizeof(mKey), "charMatches_%d", i);
                std::snprintf(wKey, sizeof(wKey), "charWins_%d", i);
                if (key == mKey) ss >> career.charMatches[i];
                else if (key == wKey) ss >> career.charWins[i];
            }
        }
    }
    previousHighScore = career.highScore;
}

void StatsManager::save() const {
    std::ofstream out(STATS_FILE, std::ios::trunc);
    if (!out.is_open()) return;

    out << "highScore=" << career.highScore << "\n";
    out << "totalScore=" << career.totalScore << "\n";
    out << "matchesPlayed=" << career.matchesPlayed << "\n";
    out << "matchesWon=" << career.matchesWon << "\n";
    out << "matchesLost=" << career.matchesLost << "\n";
    out << "roundsWon=" << career.roundsWon << "\n";
    out << "roundsLost=" << career.roundsLost << "\n";
    out << "currentWinStreak=" << career.currentWinStreak << "\n";
    out << "bestWinStreak=" << career.bestWinStreak << "\n";
    out << "totalHitsLanded=" << career.totalHitsLanded << "\n";
    out << "totalSpecialsLanded=" << career.totalSpecialsLanded << "\n";
    out << "totalDodges=" << career.totalDodges << "\n";
    out << "totalBlocks=" << career.totalBlocks << "\n";
    out << "totalKnockouts=" << career.totalKnockouts << "\n";
    out << "perfectRounds=" << career.perfectRounds << "\n";
    out << "totalDamageDealt=" << career.totalDamageDealt << "\n";
    out << "totalDamageTaken=" << career.totalDamageTaken << "\n";
    out << "fastestRoundWinSec=" << career.fastestRoundWinSec << "\n";
    for (int i = 0; i < Characters::COUNT; ++i) {
        out << "charMatches_" << i << "=" << career.charMatches[i] << "\n";
        out << "charWins_" << i << "=" << career.charWins[i] << "\n";
    }
}

void StatsManager::resetAll() {
    clearCareer(career);
    clearMatch(match);
    previousHighScore = 0;
    save();
}

void StatsManager::beginMatch() {
    clearMatch(match);
    previousHighScore = career.highScore;
}

void StatsManager::beginRound() {
    // Per-round timer handled cleanly by Game; match accumulators continue across rounds
}

void StatsManager::recordPlayerHit(int damage, bool wasSpecial, GameMode mode) {
    if (damage <= 0) return;
    match.hitsLanded++;
    match.damageDealt += damage;
    career.totalHitsLanded++;
    career.totalDamageDealt += damage;

    int basePts = 15 + damage * 5;
    if (wasSpecial) {
        match.specialsLanded++;
        career.totalSpecialsLanded++;
        basePts += 140;
    }

    int pts = (int)std::round(basePts * difficultyMultiplier(mode));
    match.score += pts;
    if (match.score > career.highScore) {
        career.highScore = match.score;
        match.isNewHighScore = (match.score > previousHighScore);
    }
}

void StatsManager::recordPlayerDodged(GameMode mode) {
    match.dodges++;
    career.totalDodges++;
    int pts = (int)std::round(75.0f * difficultyMultiplier(mode));
    match.score += pts;
    if (match.score > career.highScore) {
        career.highScore = match.score;
        match.isNewHighScore = (match.score > previousHighScore);
    }
}

void StatsManager::recordPlayerBlocked(int chipDamage, GameMode mode) {
    match.blocks++;
    career.totalBlocks++;
    if (chipDamage > 0) {
        match.damageTaken += chipDamage;
        career.totalDamageTaken += chipDamage;
    }
    int pts = (int)std::round(25.0f * difficultyMultiplier(mode));
    match.score += pts;
    if (match.score > career.highScore) {
        career.highScore = match.score;
        match.isNewHighScore = (match.score > previousHighScore);
    }
}

void StatsManager::recordEnemyHitPlayer(int damage) {
    if (damage <= 0) return;
    match.damageTaken += damage;
    career.totalDamageTaken += damage;
}

void StatsManager::recordRoundEnd(bool playerWonRound, bool enemyDown, int playerHP, int playerMaxHP,
                                  float roundElapsedSec, GameMode mode) {
    if (enemyDown && playerWonRound) {
        career.totalKnockouts++;
    }

    if (playerWonRound) {
        career.roundsWon++;
        if (roundElapsedSec > 0.1f) {
            if (match.fastestRoundSec <= 0.0f || roundElapsedSec < match.fastestRoundSec) {
                match.fastestRoundSec = roundElapsedSec;
            }
            if (career.fastestRoundWinSec <= 0.0f || roundElapsedSec < career.fastestRoundWinSec) {
                career.fastestRoundWinSec = roundElapsedSec;
            }
        }

        int hpBonus     = std::max(0, playerHP) * 6;
        int speedBonus  = std::max(0, (int)std::round((45.0f - roundElapsedSec) * 12.0f));
        int perfectAdd  = 0;
        if (playerHP >= playerMaxHP) {
            perfectAdd = 400;
            match.perfectRounds++;
            career.perfectRounds++;
        }

        int roundPts = (int)std::round((450 + hpBonus + speedBonus + perfectAdd) * difficultyMultiplier(mode));
        match.score += roundPts;
    } else if (!enemyDown) {
        career.roundsLost++;
    } else {
        // Double KO draw still counts enemy KO if enemyDown, or round lost if player lost
    }

    if (match.score > career.highScore) {
        career.highScore = match.score;
        match.isNewHighScore = (match.score > previousHighScore);
    }
    save();
}

void StatsManager::recordMatchEnd(bool playerWonMatch, CharacterId playerChar, GameMode mode) {
    int idx = (int)playerChar;
    career.matchesPlayed++;
    if (idx >= 0 && idx < Characters::COUNT) {
        career.charMatches[idx]++;
    }

    if (playerWonMatch) {
        career.matchesWon++;
        career.currentWinStreak++;
        if (career.currentWinStreak > career.bestWinStreak) {
            career.bestWinStreak = career.currentWinStreak;
        }
        if (idx >= 0 && idx < Characters::COUNT) {
            career.charWins[idx]++;
        }
        int victoryBonus = (int)std::round(900.0f * difficultyMultiplier(mode));
        match.score += victoryBonus;
    } else {
        career.matchesLost++;
        career.currentWinStreak = 0;
    }

    career.totalScore += match.score;
    if (match.score > career.highScore) {
        career.highScore = match.score;
        match.isNewHighScore = (match.score > previousHighScore);
    }
    save();
}

const CareerStats& StatsManager::getCareer() const {
    return career;
}

const MatchStats& StatsManager::getMatch() const {
    return match;
}
