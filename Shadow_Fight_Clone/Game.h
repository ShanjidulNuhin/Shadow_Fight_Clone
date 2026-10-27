#ifndef GAME_H
#define GAME_H

#include "InputManager.h"
#include "Renderer.h"
#include "UI.h"
#include "Player.h"
#include "Enemy.h"
#include "Background.h"
#include "Effects.h"

// The screen the game is showing right now.
enum class GameState {
    MENU,         // the title screen
    FIGHT,        // a round is running (or about to start: the intro banner)
    PAUSED,       // the fight is frozen
    ROUND_OVER,   // someone just got knocked out: K.O. screen
    GAME_OVER     // the match is decided: win / lose screen
};

// How the last round ended.
enum class RoundResult {
    NONE,
    PLAYER_WON,
    ENEMY_WON,
    DRAW          // both fighters went down at the same moment
};

// Owns everything in the game and decides what happens in each GameState.
// main.cpp only passes GLUT's events along to it.
class Game {
public:
    Game();

    void init();                          // call once, after the window exists
    void update(float deltaTime);         // advance the game
    void draw();                          // draw one frame

    void onKeyDown(unsigned char key);
    void onKeyUp(unsigned char key);
    void onResize(int windowW, int windowH);

    bool      shouldQuit() const;
    GameState getState() const;

private:
    void startMatch();                    // new match: scores back to 0, round 1
    void startRound();                    // fresh fighters + the "ROUND n" intro
    void endRound();                      // someone dropped: work out who won the round
    void updateFight(float deltaTime);
    void updateRoundOver(float deltaTime);
    void drawScene();                     // the arena and the two fighters

    InputManager input;
    InputManager noInput;                 // never has a key pressed (used while the K.O. plays)
    Renderer     renderer;
    UI           ui;
    Background   background;
    Effects      effects;
    Player       player;
    Enemy        enemy;

    GameState    state;
    RoundResult  roundResult;
    int          roundNumber;             // 1, 2, 3...
    int          playerWins;              // rounds won so far in this match
    int          enemyWins;
    float        roundIntroTimer;         // > 0 while the "ROUND n / FIGHT!" banner shows
    float        roundOverTimer;          // seconds since the K.O.

    bool         showDebugBoxes;
    bool         quitRequested;
};

#endif
