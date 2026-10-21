#include "Game.h"
#include "CombatSystem.h"
#include "Config.h"

namespace {
    const unsigned char KEY_ENTER  = 13;
    const unsigned char KEY_ESCAPE = 27;

    const float PLAYER_START_X = 100.0f;
    const float ENEMY_START_X  = 550.0f;
}

Game::Game()
    : input(),
      renderer(Config::WORLD_WIDTH, Config::WORLD_HEIGHT),
      ui(renderer),
      player(PLAYER_START_X, Config::GROUND_HEIGHT),
      enemy(ENEMY_START_X, Config::GROUND_HEIGHT),
      state(GameState::MENU),
      showDebugBoxes(true),
      quitRequested(false) {
}

void Game::init() {
    renderer.init();
}

void Game::startFight() {
    // brand-new fighters, standing at their starting spots with full health and energy
    player = Player(PLAYER_START_X, Config::GROUND_HEIGHT);
    enemy  = Enemy(ENEMY_START_X, Config::GROUND_HEIGHT);
    state  = GameState::FIGHT;
}

void Game::update(float deltaTime) {
    // Only the fight moves. In the menu and while paused, everything is frozen.
    if (state == GameState::FIGHT) {
        updateFight(deltaTime);
    }
}

void Game::updateFight(float deltaTime) {
    float playerXBefore = player.getHurtbox().x;
    float enemyXBefore  = enemy.getHurtbox().x;

    player.update(input, deltaTime);

    // The enemy's brain decides what to do (approach, attack, wait...).
    enemy.update(player, deltaTime);

    // Solid bodies: fighters can't walk through each other (jump over instead!).
    CombatSystem::separateFighters(player, playerXBefore, enemy, enemyXBefore);

    // Did either fighter's attack land on the other?
    CombatSystem::resolveAttack(player, enemy);
    CombatSystem::resolveAttack(enemy, player);
}

void Game::drawScene() {
    // ground
    renderer.drawRect(0, 0, Config::WORLD_WIDTH, Config::GROUND_HEIGHT,
                      0.30f, 0.20f, 0.12f);

    player.draw(renderer);
    enemy.draw(renderer);

    if (showDebugBoxes && state != GameState::MENU) {
        ui.drawDebugBoxes(player);
        ui.drawDebugBoxes(enemy);
    }
}

void Game::draw() {
    renderer.beginFrame();

    drawScene();

    switch (state) {
        case GameState::MENU:
            ui.drawMenu();
            break;
        case GameState::FIGHT:
            ui.drawHUD(player, enemy);
            break;
        case GameState::PAUSED:
            ui.drawHUD(player, enemy);
            ui.drawPauseScreen();
            break;
    }

    renderer.endFrame();
}

void Game::onKeyDown(unsigned char key) {
    // Held keys (A, D, J, K...) are always tracked, whatever screen we're on.
    input.keyDown(key);

    // One-time key presses: what they do depends on the current state.
    switch (state) {
        case GameState::MENU:
            if (key == KEY_ENTER)  startFight();
            if (key == KEY_ESCAPE) quitRequested = true;
            break;

        case GameState::FIGHT:
            if (key == 'p' || key == 'P' || key == KEY_ESCAPE) state = GameState::PAUSED;
            if (key == 'h' || key == 'H') showDebugBoxes = !showDebugBoxes;
            if (key == 'r' || key == 'R') {            // testing aid: refill everyone's health
                player.restoreHealth();
                enemy.restoreHealth();
                player.restoreEnergy();
                enemy.restoreEnergy();
            }
            break;

        case GameState::PAUSED:
            if (key == 'p' || key == 'P' || key == KEY_ESCAPE) state = GameState::FIGHT;
            if (key == 'm' || key == 'M') state = GameState::MENU;
            break;
    }
}

void Game::onKeyUp(unsigned char key) {
    input.keyUp(key);
}

void Game::onResize(int windowW, int windowH) {
    renderer.resize(windowW, windowH);
}

bool Game::shouldQuit() const {
    return quitRequested;
}

GameState Game::getState() const {
    return state;
}
