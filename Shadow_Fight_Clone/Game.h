#ifndef GAME_H
#define GAME_H

#include "InputManager.h"
#include "Renderer.h"
#include "UI.h"
#include "Player.h"
#include "Enemy.h"
#include "Background.h"
#include "Effects.h"
#include "Config.h"

enum class GameState {
    MENU,         // the title screen
    FIGHT,        // a round is running (or about to start: the intro banner)
    PAUSED,       // the fight is frozen
    ROUND_OVER,   // someone just got knocked out: K.O. screen
    GAME_OVER     // the match is decided: win / lose screen
};

enum class MenuScreen {
    MAIN,         // [Start Game] [Options] [Exit]
    OPTIONS,      // [Game Controls] [Game Mode] [Back]
    CONTROLS,     // All controls display + [Back]
    DIFFICULTY    // [Easy] [Normal] [Hard] + [Back]
};

enum class PauseScreen {
    PAUSE_MAIN,
    PAUSE_OPTIONS,
    PAUSE_CONTROLS,
    PAUSE_DIFFICULTY
};

enum class RoundResult {
    NONE,
    PLAYER_WON,
    ENEMY_WON,
    DRAW
};

class Game {
public:
    Game();

    void init();
    void update(float deltaTime);
    void draw();

    void onKeyDown(unsigned char key);
    void onKeyUp(unsigned char key);
    void onSpecialKey(int key);
    void onMouseMove(float worldX, float worldY);
    void onMouseClick(int button, int state, float worldX, float worldY);
    void onResize(int windowW, int windowH);

    bool      shouldQuit() const;
    GameState getState() const;

private:
    void startMatch();
    void startRound();
    void endRound();
    void updateFight(float deltaTime);
    void updateRoundOver(float deltaTime);
    void drawScene();

    void handleMenuClick(float mx, float my);
    void handlePauseClick(float mx, float my);

    InputManager input;
    InputManager noInput;
    Renderer     renderer;
    UI           ui;
    Background   background;
    Effects      effects;
    Player       player;
    Enemy        enemy;

    GameState    state;
    MenuScreen   menuScreen;
    PauseScreen  pauseScreen;
    GameMode     gameMode;
    RoundResult  roundResult;

    int          roundNumber;
    int          playerWins;
    int          enemyWins;
    float        roundIntroTimer;
    float        roundOverTimer;

    float        mouseX;
    float        mouseY;
    int          selectedButtonIndex;
    int          windowWidth;
    int          windowHeight;

    bool         showDebugBoxes;
    bool         quitRequested;
};

#endif
