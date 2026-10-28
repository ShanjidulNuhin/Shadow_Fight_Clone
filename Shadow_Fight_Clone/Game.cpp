#include "Game.h"
#include "CombatSystem.h"
#include "Config.h"
#include <cstdio>
#include <cstdlib>

namespace {
    const unsigned char KEY_ENTER  = 13;
    const unsigned char KEY_ESCAPE = 27;

    const float PLAYER_START_X = 100.0f;
    const float ENEMY_START_X  = 550.0f;

    const float ROUND_INTRO_TIME  = 1.6f;
    const float FIGHT_BANNER_TIME = 0.6f;
    const float ROUND_OVER_TIME   = 3.5f;
    const float SLOW_MOTION_TIME  = 1.2f;
    const float SLOW_MOTION_SPEED = 0.35f;
}

Game::Game()
    : input(),
      noInput(),
      renderer(Config::WORLD_WIDTH, Config::WORLD_HEIGHT),
      ui(renderer),
      background(),
      effects(),
      player(PLAYER_START_X, Config::GROUND_HEIGHT),
      enemy(ENEMY_START_X, Config::GROUND_HEIGHT),
      state(GameState::MENU),
      menuScreen(MenuScreen::MAIN),
      pauseScreen(PauseScreen::PAUSE_MAIN),
      gameMode(GameMode::NORMAL),
      roundResult(RoundResult::NONE),
      roundNumber(1),
      playerWins(0),
      enemyWins(0),
      roundIntroTimer(0.0f),
      roundOverTimer(0.0f),
      mouseX(-1.0f),
      mouseY(-1.0f),
      selectedButtonIndex(-1),
      windowWidth(Config::WORLD_WIDTH),
      windowHeight(Config::WORLD_HEIGHT),
      showDebugBoxes(false), // Disable green box by default as requested!
      quitRequested(false) {
}

void Game::init() {
    renderer.init();
    enemy.setDifficulty(gameMode);
}

void Game::startMatch() {
    playerWins  = 0;
    enemyWins   = 0;
    roundNumber = 1;
    background.randomizeThemeAndLocation();
    startRound();
}

void Game::startRound() {
    player = Player(PLAYER_START_X, Config::GROUND_HEIGHT);
    enemy  = Enemy(ENEMY_START_X, Config::GROUND_HEIGHT);
    enemy.setDifficulty(gameMode);

    effects.reset();

    roundResult     = RoundResult::NONE;
    roundIntroTimer = ROUND_INTRO_TIME;
    roundOverTimer  = 0.0f;
    state           = GameState::FIGHT;
}

void Game::endRound() {
    bool playerDown = player.isDead();
    bool enemyDown  = enemy.isDead();

    if (playerDown && enemyDown) {
        roundResult = RoundResult::DRAW;
    } else if (enemyDown) {
        roundResult = RoundResult::PLAYER_WON;
        playerWins++;
    } else {
        roundResult = RoundResult::ENEMY_WON;
        enemyWins++;
    }

    roundOverTimer = 0.0f;
    state = GameState::ROUND_OVER;
}

void Game::update(float deltaTime) {
    background.update(deltaTime);
    effects.update(deltaTime);

    switch (state) {
        case GameState::FIGHT:      updateFight(deltaTime);     break;
        case GameState::ROUND_OVER: updateRoundOver(deltaTime); break;
        default: break;
    }
}

void Game::updateFight(float deltaTime) {
    if (roundIntroTimer > 0.0f) {
        roundIntroTimer -= deltaTime;
        return;
    }

    float playerXBefore = player.getHurtbox().x;
    float enemyXBefore  = enemy.getHurtbox().x;

    player.update(input, deltaTime);
    enemy.update(player, deltaTime);

    CombatSystem::separateFighters(player, playerXBefore, enemy, enemyXBefore);

    CombatSystem::resolveAttack(player, enemy, &effects);
    CombatSystem::resolveAttack(enemy, player, &effects);

    if (player.isDead() || enemy.isDead()) {
        endRound();
    }
}

void Game::updateRoundOver(float deltaTime) {
    roundOverTimer += deltaTime;

    float simTime = (roundOverTimer < SLOW_MOTION_TIME) ? deltaTime * SLOW_MOTION_SPEED : deltaTime;
    player.update(noInput, simTime);
    enemy.update(player, simTime);

    if (roundOverTimer >= ROUND_OVER_TIME) {
        bool matchDecided = (playerWins >= Config::ROUNDS_TO_WIN) || (enemyWins >= Config::ROUNDS_TO_WIN);

        if (matchDecided) {
            state = GameState::GAME_OVER;
        } else {
            if (roundResult != RoundResult::DRAW) roundNumber++;
            startRound();
        }
    }
}

void Game::drawScene() {
    background.draw(renderer);
    player.draw(renderer);
    enemy.draw(renderer);

    if (showDebugBoxes && state != GameState::MENU) {
        ui.drawDebugBoxes(player);
        ui.drawDebugBoxes(enemy);
    }
}

void Game::draw() {
    float bgR, bgG, bgB;
    background.getClearColor(bgR, bgG, bgB);
    renderer.setClearColor(bgR, bgG, bgB);

    renderer.beginFrame();

    renderer.setCameraOffset(effects.getShakeX(), effects.getShakeY());
    drawScene();
    effects.drawParticles(renderer);
    renderer.resetCamera();
    effects.drawVignette(renderer);

    switch (state) {
        case GameState::MENU:
            if (menuScreen == MenuScreen::MAIN) {
                ui.drawMainMenu(mouseX, mouseY, selectedButtonIndex);
            } else if (menuScreen == MenuScreen::OPTIONS) {
                ui.drawOptionsMenu(mouseX, mouseY, selectedButtonIndex, gameMode);
            } else if (menuScreen == MenuScreen::CONTROLS) {
                ui.drawControlsScreen(mouseX, mouseY, selectedButtonIndex);
            } else if (menuScreen == MenuScreen::DIFFICULTY) {
                ui.drawGameModeScreen(mouseX, mouseY, selectedButtonIndex, gameMode);
            }
            break;

        case GameState::FIGHT:
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins, mouseX, mouseY);
            if (roundIntroTimer > 0.0f) {
                char stageInfo[64];
                std::snprintf(stageInfo, sizeof(stageInfo), "%s  -  %s",
                              background.getLocationName(), background.getThemeName());
                ui.drawRoundIntro(roundNumber, roundIntroTimer <= FIGHT_BANNER_TIME, stageInfo);
            }
            break;

        case GameState::PAUSED:
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins, mouseX, mouseY);
            if (pauseScreen == PauseScreen::PAUSE_MAIN) {
                ui.drawPauseMenu(mouseX, mouseY, selectedButtonIndex);
            } else if (pauseScreen == PauseScreen::PAUSE_OPTIONS) {
                ui.drawOptionsMenu(mouseX, mouseY, selectedButtonIndex, gameMode);
            } else if (pauseScreen == PauseScreen::PAUSE_CONTROLS) {
                ui.drawControlsScreen(mouseX, mouseY, selectedButtonIndex);
            } else if (pauseScreen == PauseScreen::PAUSE_DIFFICULTY) {
                ui.drawGameModeScreen(mouseX, mouseY, selectedButtonIndex, gameMode);
            }
            break;

        case GameState::ROUND_OVER: {
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins, mouseX, mouseY);
            char message[64];
            if (roundResult == RoundResult::PLAYER_WON) {
                std::snprintf(message, sizeof(message), "You won round %d", roundNumber);
            } else if (roundResult == RoundResult::ENEMY_WON) {
                std::snprintf(message, sizeof(message), "You lost round %d", roundNumber);
            } else {
                std::snprintf(message, sizeof(message), "Double K.O. - this round is replayed");
            }
            ui.drawRoundOver(message);
            break;
        }

        case GameState::GAME_OVER:
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins, mouseX, mouseY);
            ui.drawGameOver(playerWins >= Config::ROUNDS_TO_WIN, playerWins, enemyWins);
            break;
    }

    renderer.endFrame();
}

void Game::handleMenuClick(float mx, float my) {
    if (menuScreen == MenuScreen::MAIN) {
        if (UIButtons::BTN_MAIN_START.contains(mx, my)) {
            startMatch();
        } else if (UIButtons::BTN_MAIN_OPTIONS.contains(mx, my)) {
            menuScreen = MenuScreen::OPTIONS;
        } else if (UIButtons::BTN_MAIN_EXIT.contains(mx, my)) {
            quitRequested = true;
        }
    } else if (menuScreen == MenuScreen::OPTIONS) {
        if (UIButtons::BTN_OPT_CONTROLS.contains(mx, my)) {
            menuScreen = MenuScreen::CONTROLS;
        } else if (UIButtons::BTN_OPT_MODE.contains(mx, my)) {
            menuScreen = MenuScreen::DIFFICULTY;
        } else if (UIButtons::BTN_OPT_BACK.contains(mx, my)) {
            menuScreen = MenuScreen::MAIN;
        }
    } else if (menuScreen == MenuScreen::CONTROLS) {
        if (UIButtons::BTN_CTRL_BACK.contains(mx, my)) {
            menuScreen = MenuScreen::OPTIONS;
        }
    } else if (menuScreen == MenuScreen::DIFFICULTY) {
        if (UIButtons::BTN_DIFF_EASY.contains(mx, my)) {
            gameMode = GameMode::EASY;
            enemy.setDifficulty(gameMode);
        } else if (UIButtons::BTN_DIFF_NORMAL.contains(mx, my)) {
            gameMode = GameMode::NORMAL;
            enemy.setDifficulty(gameMode);
        } else if (UIButtons::BTN_DIFF_HARD.contains(mx, my)) {
            gameMode = GameMode::HARD;
            enemy.setDifficulty(gameMode);
        } else if (UIButtons::BTN_DIFF_BACK.contains(mx, my)) {
            menuScreen = MenuScreen::OPTIONS;
        }
    }
}

void Game::handlePauseClick(float mx, float my) {
    if (pauseScreen == PauseScreen::PAUSE_MAIN) {
        if (UIButtons::BTN_PAUSE_RESUME.contains(mx, my)) {
            state = GameState::FIGHT;
        } else if (UIButtons::BTN_PAUSE_RESTART.contains(mx, my)) {
            startMatch();
        } else if (UIButtons::BTN_PAUSE_OPTIONS.contains(mx, my)) {
            pauseScreen = PauseScreen::PAUSE_OPTIONS;
        } else if (UIButtons::BTN_PAUSE_EXIT.contains(mx, my)) {
            // Exit returns to Home screen (Main Menu), NOT closing full game!
            state = GameState::MENU;
            menuScreen = MenuScreen::MAIN;
        }
    } else if (pauseScreen == PauseScreen::PAUSE_OPTIONS) {
        if (UIButtons::BTN_OPT_CONTROLS.contains(mx, my)) {
            pauseScreen = PauseScreen::PAUSE_CONTROLS;
        } else if (UIButtons::BTN_OPT_MODE.contains(mx, my)) {
            pauseScreen = PauseScreen::PAUSE_DIFFICULTY;
        } else if (UIButtons::BTN_OPT_BACK.contains(mx, my)) {
            pauseScreen = PauseScreen::PAUSE_MAIN;
        }
    } else if (pauseScreen == PauseScreen::PAUSE_CONTROLS) {
        if (UIButtons::BTN_CTRL_BACK.contains(mx, my)) {
            pauseScreen = PauseScreen::PAUSE_OPTIONS;
        }
    } else if (pauseScreen == PauseScreen::PAUSE_DIFFICULTY) {
        if (UIButtons::BTN_DIFF_EASY.contains(mx, my)) {
            gameMode = GameMode::EASY;
            enemy.setDifficulty(gameMode);
        } else if (UIButtons::BTN_DIFF_NORMAL.contains(mx, my)) {
            gameMode = GameMode::NORMAL;
            enemy.setDifficulty(gameMode);
        } else if (UIButtons::BTN_DIFF_HARD.contains(mx, my)) {
            gameMode = GameMode::HARD;
            enemy.setDifficulty(gameMode);
        } else if (UIButtons::BTN_DIFF_BACK.contains(mx, my)) {
            pauseScreen = PauseScreen::PAUSE_OPTIONS;
        }
    }
}

void Game::onMouseMove(float worldX, float worldY) {
    mouseX = worldX;
    mouseY = worldY;
    selectedButtonIndex = -1;
}

void Game::onMouseClick(int button, int btnState, float worldX, float worldY) {
    mouseX = worldX;
    mouseY = worldY;

    if (button == 0 && btnState == 0) { // GLUT_LEFT_BUTTON and GLUT_DOWN
        if (state == GameState::MENU) {
            handleMenuClick(worldX, worldY);
        } else if (state == GameState::FIGHT) {
            // Check top-right in-game settings icon
            if (UIButtons::BTN_SETTINGS_ICON.contains(worldX, worldY)) {
                state = GameState::PAUSED;
                pauseScreen = PauseScreen::PAUSE_MAIN;
            }
        } else if (state == GameState::PAUSED) {
            handlePauseClick(worldX, worldY);
        } else if (state == GameState::GAME_OVER) {
            startMatch();
        }
    }
}

void Game::onKeyDown(unsigned char key) {
    input.keyDown(key);

    switch (state) {
        case GameState::MENU:
            if (key == KEY_ENTER) {
                if (menuScreen == MenuScreen::MAIN) startMatch();
                else if (menuScreen == MenuScreen::OPTIONS) menuScreen = MenuScreen::MAIN;
                else if (menuScreen == MenuScreen::CONTROLS) menuScreen = MenuScreen::OPTIONS;
                else if (menuScreen == MenuScreen::DIFFICULTY) menuScreen = MenuScreen::OPTIONS;
            } else if (key == KEY_ESCAPE) {
                if (menuScreen == MenuScreen::MAIN) quitRequested = true;
                else if (menuScreen == MenuScreen::OPTIONS) menuScreen = MenuScreen::MAIN;
                else if (menuScreen == MenuScreen::CONTROLS) menuScreen = MenuScreen::OPTIONS;
                else if (menuScreen == MenuScreen::DIFFICULTY) menuScreen = MenuScreen::OPTIONS;
            }
            break;

        case GameState::FIGHT:
            if (key == 'p' || key == 'P' || key == KEY_ESCAPE) {
                state = GameState::PAUSED;
                pauseScreen = PauseScreen::PAUSE_MAIN;
            }
            if (key == 'h' || key == 'H') showDebugBoxes = !showDebugBoxes;
            if (key == 't' || key == 'T') {
                player.restoreHealth();
                enemy.restoreHealth();
                player.restoreEnergy();
                enemy.restoreEnergy();
            }
            break;

        case GameState::PAUSED:
            if (key == 'p' || key == 'P' || key == KEY_ESCAPE) {
                if (pauseScreen == PauseScreen::PAUSE_MAIN) {
                    state = GameState::FIGHT;
                } else if (pauseScreen == PauseScreen::PAUSE_OPTIONS) {
                    pauseScreen = PauseScreen::PAUSE_MAIN;
                } else {
                    pauseScreen = PauseScreen::PAUSE_OPTIONS;
                }
            }
            if (key == 'r' || key == 'R') startMatch();
            if (key == 'm' || key == 'M') {
                state = GameState::MENU;
                menuScreen = MenuScreen::MAIN;
            }
            break;

        case GameState::ROUND_OVER:
            break;

        case GameState::GAME_OVER:
            if (key == KEY_ENTER) startMatch();
            if (key == 'm' || key == 'M' || key == KEY_ESCAPE) {
                state = GameState::MENU;
                menuScreen = MenuScreen::MAIN;
            }
            break;
    }
}

void Game::onKeyUp(unsigned char key) {
    input.keyUp(key);
}

void Game::onSpecialKey(int key) {
    // Optional special key support (e.g. arrow keys)
}

void Game::onResize(int windowW, int windowH) {
    windowWidth = windowW;
    windowHeight = windowH;
    renderer.resize(windowW, windowH);
}

bool Game::shouldQuit() const {
    return quitRequested;
}

GameState Game::getState() const {
    return state;
}
