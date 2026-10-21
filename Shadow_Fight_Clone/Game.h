#ifndef GAME_H
#define GAME_H

#include "InputManager.h"
#include "Renderer.h"
#include "UI.h"
#include "Player.h"
#include "Enemy.h"

// The screen the game is showing right now.
enum class GameState {
    MENU,     // the title screen
    FIGHT,    // the fight is running
    PAUSED    // the fight is frozen
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
    void startFight();                    // put both fighters back at the start
    void updateFight(float deltaTime);
    void drawScene();                     // the arena and the two fighters

    InputManager input;
    Renderer     renderer;
    UI           ui;
    Player       player;
    Enemy        enemy;

    GameState    state;
    bool         showDebugBoxes;
    bool         quitRequested;
};

#endif
