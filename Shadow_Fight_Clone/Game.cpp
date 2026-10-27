#include "Game.h"
#include "CombatSystem.h"
#include "Config.h"
#include <cstdio>

namespace {
    const unsigned char KEY_ENTER  = 13;
    const unsigned char KEY_ESCAPE = 27;

    const float PLAYER_START_X = 100.0f;
    const float ENEMY_START_X  = 550.0f;

    const float ROUND_INTRO_TIME  = 1.6f;   // how long the fighters wait at the start of a round
    const float FIGHT_BANNER_TIME = 0.6f;   // the LAST part of the intro shows "FIGHT!"
    const float ROUND_OVER_TIME   = 3.5f;   // how long the K.O. screen stays up
    const float SLOW_MOTION_TIME  = 1.2f;   // the K.O. plays in slow motion for this long...
    const float SLOW_MOTION_SPEED = 0.35f;  // ...at this fraction of normal speed
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
      roundResult(RoundResult::NONE),
      roundNumber(1),
      playerWins(0),
      enemyWins(0),
      roundIntroTimer(0.0f),
      roundOverTimer(0.0f),
      showDebugBoxes(true),
      quitRequested(false) {
}

void Game::init() {
    renderer.init();
}

void Game::startMatch() {
    playerWins  = 0;
    enemyWins   = 0;
    roundNumber = 1;
    background.randomizeThemeAndLocation();
    startRound();
}

void Game::startRound() {
    // brand-new fighters, standing at their starting spots with full health and energy
    player = Player(PLAYER_START_X, Config::GROUND_HEIGHT);
    enemy  = Enemy(ENEMY_START_X, Config::GROUND_HEIGHT);

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
        roundResult = RoundResult::DRAW;          // nobody scores
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
    // Always advance ambient background and active visual effects
    background.update(deltaTime);
    effects.update(deltaTime);

    // Only the fight and the K.O. moment advance combat. In the menu, while paused and on the
    // game-over screen, gameplay simulation is frozen.
    switch (state) {
        case GameState::FIGHT:      updateFight(deltaTime);     break;
        case GameState::ROUND_OVER: updateRoundOver(deltaTime); break;
        default: break;
    }
}

void Game::updateFight(float deltaTime) {
    // The "ROUND n / FIGHT!" banner: everyone waits until it's gone.
    if (roundIntroTimer > 0.0f) {
        roundIntroTimer -= deltaTime;
        return;
    }

    float playerXBefore = player.getHurtbox().x;
    float enemyXBefore  = enemy.getHurtbox().x;

    player.update(input, deltaTime);

    // The enemy's brain decides what to do (approach, attack, wait...).
    enemy.update(player, deltaTime);

    // Solid bodies: fighters can't walk through each other (jump over instead!).
    CombatSystem::separateFighters(player, playerXBefore, enemy, enemyXBefore);

    // Did either fighter's attack land on the other? Triggers hit sparks & screen shake
    CombatSystem::resolveAttack(player, enemy, &effects);
    CombatSystem::resolveAttack(enemy, player, &effects);

    // Did that hit knock someone out?
    if (player.isDead() || enemy.isDead()) {
        endRound();
    }
}

void Game::updateRoundOver(float deltaTime) {
    roundOverTimer += deltaTime;

    // The K.O. plays in slow motion for a moment. Nobody is controlled any more:
    // the player gets "no keys pressed", and the enemy's brain just stands there.
    float simTime = (roundOverTimer < SLOW_MOTION_TIME) ? deltaTime * SLOW_MOTION_SPEED : deltaTime;
    player.update(noInput, simTime);
    enemy.update(player, simTime);

    if (roundOverTimer >= ROUND_OVER_TIME) {
        bool matchDecided = (playerWins >= Config::ROUNDS_TO_WIN) || (enemyWins >= Config::ROUNDS_TO_WIN);

        if (matchDecided) {
            state = GameState::GAME_OVER;
        } else {
            if (roundResult != RoundResult::DRAW) roundNumber++;   // a drawn round is replayed
            startRound();
        }
    }
}

void Game::drawScene() {
    // 1. Cinematic arena environment: moonlit sky, mountains, silhouettes, mist & stone floor
    background.draw(renderer);

    // 2. Silhouette Fighters
    player.draw(renderer);
    enemy.draw(renderer);

    // 3. Optional debug bounding boxes
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

    // 1. Apply multi-axis screen shake to the world camera
    renderer.setCameraOffset(effects.getShakeX(), effects.getShakeY());

    // 2. Draw background and fighters in world coordinates
    drawScene();

    // 3. Draw particles, sparks, dust, and shockwaves in world coordinates
    effects.drawParticles(renderer);

    // 4. Reset camera offset for HUD and vignette
    renderer.resetCamera();

    // 5. Draw cinematic corner vignette
    effects.drawVignette(renderer);

    // 6. Draw UI / HUD on top (always crisp, stable and unshakeable)
    switch (state) {
        case GameState::MENU:
            ui.drawMenu();
            break;

        case GameState::FIGHT:
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins);
            if (roundIntroTimer > 0.0f) {
                char stageInfo[64];
                std::snprintf(stageInfo, sizeof(stageInfo), "%s  -  %s",
                              background.getLocationName(), background.getThemeName());
                ui.drawRoundIntro(roundNumber, roundIntroTimer <= FIGHT_BANNER_TIME, stageInfo);
            }
            break;

        case GameState::PAUSED:
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins);
            ui.drawPauseScreen();
            break;

        case GameState::ROUND_OVER: {
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins);

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
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins);
            ui.drawGameOver(playerWins >= Config::ROUNDS_TO_WIN, playerWins, enemyWins);
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
            if (key == KEY_ENTER)  startMatch();
            if (key == KEY_ESCAPE) quitRequested = true;
            break;

        case GameState::FIGHT:
            if (key == 'p' || key == 'P' || key == KEY_ESCAPE) state = GameState::PAUSED;
            if (key == 'h' || key == 'H') showDebugBoxes = !showDebugBoxes;
            if (key == 't' || key == 'T') {            // testing aid: refill everyone's health
                player.restoreHealth();
                enemy.restoreHealth();
                player.restoreEnergy();
                enemy.restoreEnergy();
            }
            break;

        case GameState::PAUSED:
            if (key == 'p' || key == 'P' || key == KEY_ESCAPE) state = GameState::FIGHT;
            if (key == 'r' || key == 'R') startMatch();
            if (key == 'm' || key == 'M') state = GameState::MENU;
            break;

        case GameState::ROUND_OVER:
            break;   // nothing to press during the K.O. moment

        case GameState::GAME_OVER:
            if (key == KEY_ENTER) startMatch();
            if (key == 'm' || key == 'M' || key == KEY_ESCAPE) state = GameState::MENU;
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
