#include "Game.h"
#include "CombatSystem.h"
#include "Config.h"
#include "SoundManager.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace {
    const unsigned char KEY_ENTER  = 13;
    const unsigned char KEY_ESCAPE = 27;

    const float PLAYER_START_X = 90.0f;
    const float ENEMY_START_X  = 610.0f;

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
      player(PLAYER_START_X, Config::GROUND_HEIGHT, CharacterId::BRAWLER),
      enemy(ENEMY_START_X, Config::GROUND_HEIGHT, CharacterId::CHAIN_WARRIOR),
      stats(),
      state(GameState::MENU),
      menuScreen(MenuScreen::MAIN),
      pauseScreen(PauseScreen::PAUSE_MAIN),
      gameMode(GameMode::NORMAL),
      selectedDifficulty(GameMode::NORMAL),
      playerCharacter(CharacterId::BRAWLER),
      enemyCharacter(CharacterId::CHAIN_WARRIOR),
      showcaseCharacter(CharacterId::BRAWLER),
      showcaseTimer(0.0f),
      showcasePoseCombat(false),
      roundResult(RoundResult::NONE),
      roundNumber(1),
      playerWins(0),
      enemyWins(0),
      roundIntroTimer(0.0f),
      roundOverTimer(0.0f),
      roundElapsedSec(0.0f),
      mouseX(-1.0f),
      mouseY(-1.0f),
      selectedButtonIndex(-1),
      charSelectColumn(0),
      controlsTabOverride(-1),
      windowWidth(Config::WORLD_WIDTH),
      windowHeight(Config::WORLD_HEIGHT),
      matchInProgress(false),
      isStartGameFlow(false),
      confirmStatsReset(false),
      popupType(PopupType::NONE),
      showDebugBoxes(false), // Disable green box by default as requested!
      quitRequested(false) {
}

void Game::init() {
    renderer.init();
    enemy.setDifficulty(gameMode);
    SoundManager::init();
    stats.load();
    input.loadBindings();
}

bool Game::shouldShowGamepadControls() const {
    if (controlsTabOverride == 0) return false;
    if (controlsTabOverride == 1) return true;
    return input.isGamepadConnected();
}

void Game::handleControlsClick(float mx, float my, bool fromPause) {
    if (UIButtons::BTN_CTRL_BACK.contains(mx, my)) {
        input.cancelRebind();
        SoundManager::play(SoundEffect::UI_BACK);
        if (fromPause) pauseScreen = PauseScreen::PAUSE_OPTIONS;
        else           menuScreen  = MenuScreen::OPTIONS;
        selectedButtonIndex = 0;
        return;
    }
    if (UIButtons::BTN_CTRL_TAB_KB.contains(mx, my)) {
        input.cancelRebind();
        SoundManager::play(SoundEffect::UI_CLICK);
        controlsTabOverride = 0;
        return;
    }
    if (UIButtons::BTN_CTRL_TAB_PAD.contains(mx, my)) {
        input.cancelRebind();
        SoundManager::play(SoundEffect::UI_CLICK);
        controlsTabOverride = 1;
        return;
    }
    if (UIButtons::BTN_CTRL_RESET.contains(mx, my)) {
        input.cancelRebind();
        SoundManager::play(SoundEffect::UI_CLICK);
        input.resetDefaultBindings();
        input.saveBindings();
        return;
    }

    bool showPad = shouldShowGamepadControls();
    for (int i = 0; i < (int)GameAction::COUNT; ++i) {
        if (UIButtons::controlRowButton(i).contains(mx, my)) {
            if (input.isRebinding() &&
                input.getRebindAction() == (GameAction)i &&
                input.isRebindingGamepad() == showPad) {
                input.cancelRebind();
                SoundManager::play(SoundEffect::UI_BACK);
            } else {
                SoundManager::play(SoundEffect::UI_CLICK);
                selectedButtonIndex = i;
                if (showPad) input.beginPadRebind((GameAction)i);
                else         input.beginKeyRebind((GameAction)i);
            }
            return;
        }
    }

    if (input.isRebinding()) {
        input.cancelRebind();
        SoundManager::play(SoundEffect::UI_BACK);
    }
}

void Game::startMatch() {
    playerWins  = 0;
    enemyWins   = 0;
    roundNumber = 1;
    matchInProgress = true;
    isStartGameFlow = false;
    popupType       = PopupType::NONE;
    stats.beginMatch();
    background.randomizeThemeAndLocation();
    startRound();
}

void Game::startRound() {
    player = Player(PLAYER_START_X, Config::GROUND_HEIGHT, playerCharacter);
    enemy  = Enemy(ENEMY_START_X, Config::GROUND_HEIGHT, enemyCharacter);
    enemy.setDifficulty(gameMode);

    projectiles.clear();
    input.clearJustPressed();
    effects.reset();
    stats.beginRound();

    roundResult     = RoundResult::NONE;
    roundIntroTimer = ROUND_INTRO_TIME;
    roundOverTimer  = 0.0f;
    roundElapsedSec = 0.0f;
    state           = GameState::FIGHT;
    SoundManager::play(SoundEffect::ROUND_START);
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

    stats.recordRoundEnd(roundResult == RoundResult::PLAYER_WON, enemyDown,
                         player.getHealth(), player.getMaxHealth(),
                         roundElapsedSec, gameMode);

    roundOverTimer = 0.0f;
    state = GameState::ROUND_OVER;
}

void Game::update(float deltaTime) {
    input.pollGamepad();
    handleGamepadNavigation();

    showcaseTimer += deltaTime;
    background.update(deltaTime);
    effects.update(deltaTime);

    switch (state) {
        case GameState::FIGHT:      updateFight(deltaTime);     break;
        case GameState::ROUND_OVER: updateRoundOver(deltaTime); break;
        default:
            input.clearJustPressed();
            break;
    }
}

void Game::handleGamepadNavigation() {
    if (input.isRebinding()) return;

    bool navUp    = input.wasGamepadNavPressed(GamepadNav::UP);
    bool navDown  = input.wasGamepadNavPressed(GamepadNav::DOWN);
    bool navLeft  = input.wasGamepadNavPressed(GamepadNav::LEFT);
    bool navRight = input.wasGamepadNavPressed(GamepadNav::RIGHT);
    bool navOk    = input.wasGamepadNavPressed(GamepadNav::CONFIRM);
    bool navBack  = input.wasGamepadNavPressed(GamepadNav::CANCEL);
    bool navStart = input.wasGamepadNavPressed(GamepadNav::START);
    bool navY     = input.wasGamepadNavPressed(GamepadNav::SPECIAL_Y);
    bool navLB    = input.wasGamepadNavPressed(GamepadNav::LB);
    bool navRB    = input.wasGamepadNavPressed(GamepadNav::RB);

    auto clickBtn = [&](const RectButton& btn, bool isPause) {
        float cx = btn.x + btn.width * 0.5f;
        float cy = btn.y + btn.height * 0.5f;
        if (isPause) handlePauseClick(cx, cy);
        else         handleMenuClick(cx, cy);
    };

    auto handleControlsNav = [&](bool inPause) {
        const int count = (int)GameAction::COUNT;
        if (selectedButtonIndex < 0 || selectedButtonIndex >= count) selectedButtonIndex = 0;
        if (navUp)   selectedButtonIndex = (selectedButtonIndex + count - 1) % count;
        if (navDown) selectedButtonIndex = (selectedButtonIndex + 1) % count;
        if (navLeft || navRight || navLB || navRB) {
            bool curPad = shouldShowGamepadControls();
            clickBtn(curPad ? UIButtons::BTN_CTRL_TAB_KB : UIButtons::BTN_CTRL_TAB_PAD, inPause);
        } else if (navOk || navStart) {
            clickBtn(UIButtons::controlRowButton(selectedButtonIndex), inPause);
        } else if (navY) {
            clickBtn(UIButtons::BTN_CTRL_RESET, inPause);
        } else if (navBack) {
            clickBtn(UIButtons::BTN_CTRL_BACK, inPause);
            selectedButtonIndex = 0;
        }
    };

    if (state == GameState::FIGHT) {
        if (navStart) {
            SoundManager::play(SoundEffect::UI_CLICK);
            state = GameState::PAUSED;
            pauseScreen = PauseScreen::PAUSE_MAIN;
            selectedButtonIndex = 0;
        }
        return;
    }

    if (state == GameState::GAME_OVER) {
        if (navOk || navStart) {
            SoundManager::play(SoundEffect::UI_START);
            startMatch();
        } else if (navBack) {
            SoundManager::play(SoundEffect::UI_BACK);
            matchInProgress = false;
            popupType = PopupType::NONE;
            state = GameState::MENU;
            menuScreen = MenuScreen::MAIN;
            selectedButtonIndex = 0;
        }
        return;
    }

    if (state == GameState::MENU) {
        if (menuScreen == MenuScreen::MAIN) {
            if (selectedButtonIndex < 0 || selectedButtonIndex > 4) {
                if (navUp || navDown) selectedButtonIndex = 0;
            } else {
                if (navUp)   selectedButtonIndex = (selectedButtonIndex + 4) % 5;
                if (navDown) selectedButtonIndex = (selectedButtonIndex + 1) % 5;
            }
            if (navOk || navStart) {
                int idx = (selectedButtonIndex >= 0 && selectedButtonIndex <= 4) ? selectedButtonIndex : 0;
                const RectButton* btns[5] = {
                    &UIButtons::BTN_MAIN_START, &UIButtons::BTN_MAIN_CHARACTERS,
                    &UIButtons::BTN_MAIN_STATS, &UIButtons::BTN_MAIN_OPTIONS,
                    &UIButtons::BTN_MAIN_EXIT
                };
                clickBtn(*btns[idx], false);
                selectedButtonIndex = 0;
                charSelectColumn = 0;
            } else if (navBack) {
                SoundManager::play(SoundEffect::UI_BACK);
                quitRequested = true;
            }
        } else if (menuScreen == MenuScreen::CHARACTERS) {
            if (navLeft || navLB) {
                int idx = ((int)showcaseCharacter + Characters::COUNT - 1) % Characters::COUNT;
                showcaseCharacter = (CharacterId)idx;
                SoundManager::play(SoundEffect::UI_CLICK);
            } else if (navRight || navRB) {
                int idx = ((int)showcaseCharacter + 1) % Characters::COUNT;
                showcaseCharacter = (CharacterId)idx;
                SoundManager::play(SoundEffect::UI_CLICK);
            }
            if (navY) {
                showcasePoseCombat = !showcasePoseCombat;
                SoundManager::play(SoundEffect::UI_CLICK);
            } else if (navOk || navBack) {
                clickBtn(UIButtons::BTN_ROSTER_BACK, false);
                selectedButtonIndex = 1;
            }
        } else if (menuScreen == MenuScreen::CHARACTER_SELECT) {
            if (navLeft || navRight || navLB || navRB) {
                charSelectColumn = 1 - charSelectColumn;
                SoundManager::play(SoundEffect::UI_CLICK);
            }
            int step = navUp ? -1 : (navDown ? 1 : 0);
            if (step != 0) {
                if (charSelectColumn == 0) {
                    int idx = ((int)playerCharacter + step + Characters::COUNT) % Characters::COUNT;
                    playerCharacter = Characters::getByIndex(idx).id;
                    if (enemyCharacter == playerCharacter) {
                        enemyCharacter = Characters::getByIndex((idx + 1) % Characters::COUNT).id;
                    }
                } else {
                    int idx = ((int)enemyCharacter + step + Characters::COUNT) % Characters::COUNT;
                    if (idx == (int)playerCharacter) {
                        idx = (idx + step + Characters::COUNT) % Characters::COUNT;
                    }
                    enemyCharacter = Characters::getByIndex(idx).id;
                }
                SoundManager::play(SoundEffect::UI_CLICK);
            }
            if (navStart) {
                clickBtn(UIButtons::BTN_CHAR_NEXT, false);
                selectedButtonIndex = 0;
            } else if (navOk) {
                if (charSelectColumn == 0) {
                    charSelectColumn = 1;
                    SoundManager::play(SoundEffect::UI_CLICK);
                } else {
                    clickBtn(UIButtons::BTN_CHAR_NEXT, false);
                    selectedButtonIndex = 0;
                }
            } else if (navBack) {
                if (charSelectColumn == 1) {
                    charSelectColumn = 0;
                    SoundManager::play(SoundEffect::UI_BACK);
                } else {
                    clickBtn(UIButtons::BTN_CHAR_BACK, false);
                    selectedButtonIndex = 0;
                }
            }
        } else if (menuScreen == MenuScreen::STATS) {
            if (confirmStatsReset) {
                if (navOk)        clickBtn(UIButtons::BTN_CONFIRM_YES, false);
                else if (navBack) clickBtn(UIButtons::BTN_CONFIRM_NO, false);
            } else {
                if (navY)                    clickBtn(UIButtons::BTN_STATS_RESET, false);
                else if (navOk || navBack) { clickBtn(UIButtons::BTN_STATS_BACK, false); selectedButtonIndex = 1; }
            }
        } else if (menuScreen == MenuScreen::OPTIONS) {
            if (selectedButtonIndex < 0 || selectedButtonIndex > 2) selectedButtonIndex = 0;
            if (navUp)   selectedButtonIndex = (selectedButtonIndex + 2) % 3;
            if (navDown) selectedButtonIndex = (selectedButtonIndex + 1) % 3;
            if (navOk || navStart) {
                const RectButton* btns[3] = {
                    &UIButtons::BTN_OPT_CONTROLS, &UIButtons::BTN_OPT_MODE, &UIButtons::BTN_OPT_BACK
                };
                clickBtn(*btns[selectedButtonIndex], false);
                selectedButtonIndex = 0;
            } else if (navBack) {
                clickBtn(UIButtons::BTN_OPT_BACK, false);
                selectedButtonIndex = 2;
            }
        } else if (menuScreen == MenuScreen::CONTROLS) {
            handleControlsNav(false);
        } else if (menuScreen == MenuScreen::DIFFICULTY) {
            if (navLeft) {
                int m = ((int)selectedDifficulty + 2) % 3;
                selectedDifficulty = (GameMode)m;
                SoundManager::play(SoundEffect::UI_CLICK);
            } else if (navRight) {
                int m = ((int)selectedDifficulty + 1) % 3;
                selectedDifficulty = (GameMode)m;
                SoundManager::play(SoundEffect::UI_CLICK);
            }
            if (navOk || navStart) {
                clickBtn(UIButtons::BTN_DIFF_OK, false);
                selectedButtonIndex = 0;
            } else if (navBack) {
                clickBtn(UIButtons::BTN_DIFF_BACK, false);
                selectedButtonIndex = 0;
            }
        }
        return;
    }

    if (state == GameState::PAUSED) {
        if (popupType == PopupType::ROUND1_RESTART_CONFIRM) {
            if (navOk || navStart) clickBtn(UIButtons::BTN_ALERT_OK, true);
            else if (navBack) {
                SoundManager::play(SoundEffect::UI_BACK);
                popupType = PopupType::NONE;
                selectedDifficulty = gameMode;
            }
            return;
        }
        if (popupType == PopupType::ROUND2_RESTART_PROMPT) {
            if (navOk)        clickBtn(UIButtons::BTN_CONFIRM_YES, true);
            else if (navBack) clickBtn(UIButtons::BTN_CONFIRM_NO, true);
            return;
        }

        if (pauseScreen == PauseScreen::PAUSE_MAIN) {
            if (selectedButtonIndex < 0 || selectedButtonIndex > 3) selectedButtonIndex = 0;
            if (navUp)   selectedButtonIndex = (selectedButtonIndex + 3) % 4;
            if (navDown) selectedButtonIndex = (selectedButtonIndex + 1) % 4;
            if (navStart || navBack) {
                clickBtn(UIButtons::BTN_PAUSE_RESUME, true);
            } else if (navOk) {
                const RectButton* btns[4] = {
                    &UIButtons::BTN_PAUSE_RESUME, &UIButtons::BTN_PAUSE_RESTART,
                    &UIButtons::BTN_PAUSE_OPTIONS, &UIButtons::BTN_PAUSE_EXIT
                };
                clickBtn(*btns[selectedButtonIndex], true);
                selectedButtonIndex = 0;
            }
        } else if (pauseScreen == PauseScreen::PAUSE_OPTIONS) {
            if (selectedButtonIndex < 0 || selectedButtonIndex > 2) selectedButtonIndex = 0;
            if (navUp)   selectedButtonIndex = (selectedButtonIndex + 2) % 3;
            if (navDown) selectedButtonIndex = (selectedButtonIndex + 1) % 3;
            if (navOk || navStart) {
                const RectButton* btns[3] = {
                    &UIButtons::BTN_OPT_CONTROLS, &UIButtons::BTN_OPT_MODE, &UIButtons::BTN_OPT_BACK
                };
                clickBtn(*btns[selectedButtonIndex], true);
                selectedButtonIndex = 0;
            } else if (navBack) {
                clickBtn(UIButtons::BTN_OPT_BACK, true);
                selectedButtonIndex = 2;
            }
        } else if (pauseScreen == PauseScreen::PAUSE_CONTROLS) {
            handleControlsNav(true);
        } else if (pauseScreen == PauseScreen::PAUSE_DIFFICULTY) {
            if (navLeft) {
                int m = ((int)selectedDifficulty + 2) % 3;
                selectedDifficulty = (GameMode)m;
                SoundManager::play(SoundEffect::UI_CLICK);
            } else if (navRight) {
                int m = ((int)selectedDifficulty + 1) % 3;
                selectedDifficulty = (GameMode)m;
                SoundManager::play(SoundEffect::UI_CLICK);
            }
            if (navOk || navStart) {
                clickBtn(UIButtons::BTN_DIFF_OK, true);
            } else if (navBack) {
                clickBtn(UIButtons::BTN_DIFF_BACK, true);
                selectedButtonIndex = 0;
            }
        }
    }
}

void Game::updateFight(float deltaTime) {
    if (roundIntroTimer > 0.0f) {
        bool wasFightBanner = (roundIntroTimer <= FIGHT_BANNER_TIME);
        roundIntroTimer -= deltaTime;
        if (!wasFightBanner && roundIntroTimer <= FIGHT_BANNER_TIME) {
            SoundManager::play(SoundEffect::FIGHT_START);
        }
        input.clearJustPressed();
        return;
    }

    roundElapsedSec += deltaTime;

    float playerXBefore = player.getHurtbox().x;
    float enemyXBefore  = enemy.getHurtbox().x;

    player.setTargetPositionHint(enemy.getCenterX(), Config::GROUND_HEIGHT);
    enemy.setTargetPositionHint(player.getCenterX(), Config::GROUND_HEIGHT);

    player.update(input, deltaTime);
    enemy.update(player, deltaTime);

    updateFanSpecialDriver(player, enemy, deltaTime);
    updateFanSpecialDriver(enemy, player, deltaTime);

    CombatSystem::separateFighters(player, playerXBefore, enemy, enemyXBefore);

    spawnProjectilesFrom(player, true);
    spawnProjectilesFrom(enemy, false);
    updateProjectiles(deltaTime);

    HitOutcome pHit = { false, false, false, false, 0 };
    HitOutcome eHit = { false, false, false, false, 0 };
    CombatSystem::resolveAttack(player, enemy, &effects, &pHit);
    CombatSystem::resolveAttack(enemy, player, &effects, &eHit);

    if (pHit.connected && pHit.damageDealt > 0) {
        stats.recordPlayerHit(pHit.damageDealt, pHit.wasSpecial && !pHit.blocked, gameMode);
    }
    if (eHit.dodged) {
        stats.recordPlayerDodged(gameMode);
    } else if (eHit.connected) {
        if (eHit.blocked) {
            stats.recordPlayerBlocked(eHit.damageDealt, gameMode);
        } else {
            stats.recordEnemyHitPlayer(eHit.damageDealt);
        }
    }

    input.clearJustPressed();

    if (player.isDead() || enemy.isDead()) {
        endRound();
    }
}

void Game::spawnProjectilesFrom(Fighter& owner, bool fromPlayer) {
    std::vector<ProjectileSpawn> spawns = owner.drainProjectileSpawns();
    for (size_t i = 0; i < spawns.size(); ++i) {
        const ProjectileSpawn& s = spawns[i];
        Projectile p;
        p.kind       = s.kind;
        p.fromPlayer = fromPlayer;
        p.x          = s.x;
        p.y          = s.y;
        p.vx         = s.vx;
        p.vy         = s.vy;
        p.width      = s.width;
        p.height     = s.height;
        p.damage     = s.damage;
        p.knockback  = s.knockback;
        p.hitStun    = s.hitStun;
        p.r          = s.r;
        p.g          = s.g;
        p.b          = s.b;
        p.spinAngle  = 0.0f;
        p.alive      = true;
        projectiles.push_back(p);
    }
}

void Game::updateFanSpecialDriver(Fighter& attacker, Fighter& defender, float deltaTime) {
    if (attacker.isDead() || attacker.getState() != FighterState::SPECIAL ||
        attacker.getCharacterId() != CharacterId::FAN_DANCER) {
        return;
    }

    int stage = attacker.getFanSpecialStage();
    if (stage == 0) return;

    Rect attBox = attacker.getHurtbox();
    Rect defBox = defender.getHurtbox();
    float attCenter = attBox.x + attBox.width * 0.5f;
    float defCenter = defBox.x + defBox.width * 0.5f;
    float dirToDef = (defCenter >= attCenter) ? 1.0f : -1.0f;
    float gap = std::fabs(defCenter - attCenter) - (attBox.width + defBox.width) * 0.5f;

    attacker.setFacingRight(dirToDef > 0.0f);

    if (stage == 1) {
        // Run directly straight toward the opponent
        attacker.pushHorizontally(dirToDef * 740.0f * deltaTime);
        attBox = attacker.getHurtbox();
        attCenter = attBox.x + attBox.width * 0.5f;
        gap = std::fabs(defCenter - attCenter) - (attBox.width + defBox.width) * 0.5f;

        if (gap <= 48.0f) {
            attacker.advanceFanSpecialStage(2, 430.0f);
            SoundManager::play(SoundEffect::JUMP);
        }
    } else if (stage == 2) {
        // Keep close in front of opponent while leaping up to grab head and knee face
        float desiredCenter = defCenter - dirToDef * (attBox.width * 0.5f + defBox.width * 0.5f + 10.0f);
        attacker.pushHorizontally((desiredCenter - attCenter) * std::min(1.0f, deltaTime * 14.0f));

        if (attacker.consumeKneeImpactReady() && !defender.isDead()) {
            int dmg = std::max(1, (int)std::round(26.0f * attacker.getDamageMultiplier()));
            float stun = 0.68f;
            float kbScale = 1.0f;
            if (defender.isRolling()) {
                dmg = std::max(1, (int)std::round(dmg * 0.35f));
                stun = 0.20f;
                kbScale = 0.40f;
            }
            int hpBefore = defender.getHealth();
            defender.receiveHit(attacker.getProfile().specialAttack, dirToDef, dmg, stun, kbScale);
            int actualDmg = std::max(0, hpBefore - defender.getHealth());
            if (&attacker == &player) {
                stats.recordPlayerHit(actualDmg, true, gameMode);
            } else if (defender.getState() == FighterState::BLOCK && !defender.isRolling()) {
                stats.recordPlayerBlocked(actualDmg, gameMode);
            } else {
                stats.recordEnemyHitPlayer(actualDmg);
            }
            float hitX = (attCenter + defCenter) * 0.5f;
            float hitY = defBox.y + defBox.height * 0.85f; // Face height!
            effects.triggerHit(hitX, hitY, dirToDef, defender.isDead() ? SparkStyle::KO : SparkStyle::SPECIAL);
            SoundManager::play(defender.isDead() ? SoundEffect::KO : SoundEffect::HIT_SPECIAL);
        }
    }
}

void Game::updateProjectiles(float deltaTime) {
    int playerSkyCount = 0;
    int enemySkyCount  = 0;

    for (size_t i = 0; i < projectiles.size(); ++i) {
        Projectile& p = projectiles[i];
        if (!p.alive) continue;

        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;
        p.spinAngle += 20.0f * deltaTime;

        if (p.kind == ProjectileKind::SKY_SPEAR && p.y <= Config::GROUND_HEIGHT) {
            p.alive = false;
            effects.triggerDust(p.x + p.width * 0.5f, Config::GROUND_HEIGHT, 1.0f);
            continue;
        }
        if (p.x + p.width < -60.0f || p.x > Config::WORLD_WIDTH + 60.0f || p.y < 0.0f) {
            p.alive = false;
            continue;
        }

        Fighter& attacker = p.fromPlayer ? (Fighter&)player : (Fighter&)enemy;
        Fighter& defender = p.fromPlayer ? (Fighter&)enemy : (Fighter&)player;

        if (!defender.isDead() && !defender.isInvulnerable()) {
            Rect pBox = { p.x, p.y, p.width, p.height };
            Rect dBox = defender.getHurtbox();
            if (Collision::overlaps(pBox, dBox)) {
                float pushDir = (p.vx != 0.0f) ? ((p.vx > 0.0f) ? 1.0f : -1.0f)
                                               : ((defender.getCenterX() >= attacker.getCenterX()) ? 1.0f : -1.0f);

                AttackData tempAtk = { 0.0f, 0.1f, 0.1f, p.width, p.height, 0.0f,
                                       p.damage, p.knockback, p.hitStun, 0.0f, true, 0.30f };
                int rawDmg = std::max(1, (int)std::round(p.damage * attacker.getDamageMultiplier()));
                float stun = p.hitStun;
                float kbScale = 1.0f;

                if (defender.isRolling()) {
                    rawDmg = std::max(1, (int)std::round(rawDmg * 0.35f));
                    stun = 0.18f;
                    kbScale = 0.35f;
                }

                int hpBefore = defender.getHealth();
                defender.receiveHit(tempAtk, pushDir, rawDmg, stun, kbScale);
                int actualDmg = std::max(0, hpBefore - defender.getHealth());

                if (p.fromPlayer) {
                    stats.recordPlayerHit(actualDmg, true, gameMode);
                } else if (defender.getState() == FighterState::BLOCK && !defender.isRolling()) {
                    stats.recordPlayerBlocked(actualDmg, gameMode);
                } else {
                    stats.recordEnemyHitPlayer(actualDmg);
                }

                if (p.kind == ProjectileKind::CHINESE_FAN) {
                    attacker.onFanProjectileHit();
                }

                SparkStyle style = SparkStyle::PUNCH;
                if (defender.isDead()) {
                    style = SparkStyle::KO;
                } else if (defender.getState() == FighterState::BLOCK) {
                    style = SparkStyle::BLOCK;
                } else if (p.kind == ProjectileKind::WAR_ARROW || p.kind == ProjectileKind::THROWN_SPEAR) {
                    style = SparkStyle::SPECIAL;
                }
                effects.triggerHit(p.x + p.width * 0.5f, p.y + p.height * 0.5f, pushDir, style);

                if (defender.isDead()) {
                    SoundManager::play(SoundEffect::KO);
                } else if (defender.getState() == FighterState::BLOCK) {
                    SoundManager::play(SoundEffect::HIT_BLOCK);
                } else if (p.kind == ProjectileKind::WAR_ARROW || p.kind == ProjectileKind::THROWN_SPEAR) {
                    SoundManager::play(SoundEffect::HIT_SPECIAL);
                } else {
                    SoundManager::play(SoundEffect::HIT_PUNCH);
                }

                p.alive = false;
                continue;
            }
        }

        if (p.alive && p.kind == ProjectileKind::SKY_SPEAR) {
            if (p.fromPlayer) playerSkyCount++;
            else              enemySkyCount++;
        }
    }

    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
                       [](const Projectile& pr) { return !pr.alive; }),
        projectiles.end()
    );

    player.setActiveSkySpearsCount(playerSkyCount);
    enemy.setActiveSkySpearsCount(enemySkyCount);
}

void Game::drawProjectiles() const {
    for (size_t i = 0; i < projectiles.size(); ++i) {
        const Projectile& p = projectiles[i];
        float cx = p.x + p.width * 0.5f;
        float cy = p.y + p.height * 0.5f;
        float dir = (p.vx >= 0.0f) ? 1.0f : -1.0f;

        if (p.kind == ProjectileKind::WAR_ARROW) {
            // Blazing Heavy War Arrow
            renderer.drawLine(p.x, cy, p.x + p.width, cy, 0.95f, 0.95f, 1.0f, 1.0f, 3.2f);
            float tipX = (dir > 0.0f) ? (p.x + p.width + 8.0f) : (p.x - 8.0f);
            float baseX = (dir > 0.0f) ? (p.x + p.width - 8.0f) : (p.x + 8.0f);
            renderer.drawTriangle(baseX, cy + 7.0f, baseX, cy - 7.0f, tipX, cy, p.r, p.g, p.b);
            // Energy trail behind arrow
            renderer.drawLine(cx - dir * 38.0f, cy, cx, cy, p.r, p.g, p.b, 0.70f, 6.0f);
        } else if (p.kind == ProjectileKind::CHINESE_FAN) {
            // Spinning Chinese War Fan
            float rad = 16.0f;
            for (int r = 0; r < 5; ++r) {
                float a0 = p.spinAngle + r * 0.35f;
                float a1 = a0 + 0.32f;
                renderer.drawTriangle(cx, cy,
                                      cx + std::cos(a0) * rad, cy + std::sin(a0) * rad,
                                      cx + std::cos(a1) * rad, cy + std::sin(a1) * rad,
                                      p.r, p.g, p.b);
            }
            renderer.drawCircleOutline(cx, cy, rad, 1.0f, 0.92f, 0.45f, 0.85f, 2.0f, 14);
        } else if (p.kind == ProjectileKind::SKY_SPEAR) {
            // Vertical golden energy spear dropping from the sky
            renderer.drawLine(cx, p.y + p.height, cx, p.y + 16.0f, p.r, p.g, p.b, 0.95f, 4.2f);
            renderer.drawTriangle(cx - 5.5f, p.y + 18.0f, cx + 5.5f, p.y + 18.0f, cx, p.y - 4.0f, 1.0f, 0.95f, 0.75f);
            renderer.drawLine(cx, p.y + p.height + 24.0f, cx, p.y + 20.0f, 1.0f, 0.9f, 0.4f, 0.45f, 7.0f);
        } else if (p.kind == ProjectileKind::THROWN_SPEAR) {
            // Warrior's Main Spear thrown horizontally with full force
            renderer.drawLine(p.x, cy, p.x + p.width, cy, 0.22f, 0.16f, 0.16f, 1.0f, 5.0f);
            float tipX = (dir > 0.0f) ? (p.x + p.width + 14.0f) : (p.x - 14.0f);
            float baseX = (dir > 0.0f) ? (p.x + p.width - 10.0f) : (p.x + 10.0f);
            renderer.drawTriangle(baseX, cy + 7.5f, baseX, cy - 7.5f, tipX, cy, p.r, p.g, p.b);
            renderer.drawLine(cx - dir * 55.0f, cy, cx + dir * 20.0f, cy, p.r, p.g, p.b, 0.65f, 8.0f);
        }
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
            stats.recordMatchEnd(playerWins >= Config::ROUNDS_TO_WIN, playerCharacter, gameMode);
            state = GameState::GAME_OVER;
            SoundManager::play(playerWins >= Config::ROUNDS_TO_WIN ? SoundEffect::VICTORY : SoundEffect::DEFEAT);
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
    drawProjectiles();

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

    int liveScore = stats.getMatch().score;
    int bestScore = stats.getCareer().highScore;
    bool gpConn   = input.isGamepadConnected();
    const char* gpName = input.getGamepadBackendName();

    switch (state) {
        case GameState::MENU:
            if (menuScreen == MenuScreen::MAIN) {
                ui.drawMainMenu(mouseX, mouseY, selectedButtonIndex, &stats.getCareer(), gpConn, gpName);
            } else if (menuScreen == MenuScreen::CHARACTERS) {
                ui.drawCharactersPage(mouseX, mouseY, showcaseCharacter, showcaseTimer, showcasePoseCombat, gpConn);
            } else if (menuScreen == MenuScreen::CHARACTER_SELECT) {
                ui.drawCharacterSelect(mouseX, mouseY, playerCharacter, enemyCharacter, gpConn, charSelectColumn);
            } else if (menuScreen == MenuScreen::STATS) {
                ui.drawStatsScreen(mouseX, mouseY, stats.getCareer(), confirmStatsReset);
            } else if (menuScreen == MenuScreen::OPTIONS) {
                ui.drawOptionsMenu(mouseX, mouseY, selectedButtonIndex, gameMode);
            } else if (menuScreen == MenuScreen::CONTROLS) {
                ui.drawControlsScreen(mouseX, mouseY, selectedButtonIndex, &input, shouldShowGamepadControls());
            } else if (menuScreen == MenuScreen::DIFFICULTY) {
                ui.drawGameModeScreen(mouseX, mouseY, selectedButtonIndex, selectedDifficulty, isStartGameFlow);
            }
            break;

        case GameState::FIGHT:
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins, mouseX, mouseY, liveScore, bestScore);
            if (roundIntroTimer > 0.0f) {
                char stageInfo[64];
                std::snprintf(stageInfo, sizeof(stageInfo), "%s  -  %s",
                              background.getLocationName(), background.getThemeName());
                ui.drawRoundIntro(roundNumber, roundIntroTimer <= FIGHT_BANNER_TIME, stageInfo);
            }
            break;

        case GameState::PAUSED:
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins, mouseX, mouseY, liveScore, bestScore);
            if (pauseScreen == PauseScreen::PAUSE_MAIN) {
                ui.drawPauseMenu(mouseX, mouseY, selectedButtonIndex);
            } else if (pauseScreen == PauseScreen::PAUSE_OPTIONS) {
                ui.drawOptionsMenu(mouseX, mouseY, selectedButtonIndex, gameMode);
            } else if (pauseScreen == PauseScreen::PAUSE_CONTROLS) {
                ui.drawControlsScreen(mouseX, mouseY, selectedButtonIndex, &input, shouldShowGamepadControls());
            } else if (pauseScreen == PauseScreen::PAUSE_DIFFICULTY) {
                ui.drawGameModeScreen(mouseX, mouseY, selectedButtonIndex, selectedDifficulty, false);
                if (popupType == PopupType::ROUND1_RESTART_CONFIRM) {
                    ui.drawAlertDialog("NOTICE",
                                       "The match will restart in this difficulty",
                                       "Click OK to confirm and restart from Round 1.",
                                       mouseX, mouseY, "OK");
                } else if (popupType == PopupType::ROUND2_RESTART_PROMPT) {
                    ui.drawConfirmDialog("DIFFICULTY LOCKED",
                                         "You cannot change after first round... if you want the game will restart.",
                                         "Do you want to restart the game?",
                                         mouseX, mouseY, "YES", "NO");
                }
            }
            break;

        case GameState::ROUND_OVER: {
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins, mouseX, mouseY, liveScore, bestScore);
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
            ui.drawHUD(player, enemy, roundNumber, playerWins, enemyWins, mouseX, mouseY, liveScore, bestScore);
            ui.drawGameOver(playerWins >= Config::ROUNDS_TO_WIN, playerWins, enemyWins,
                            stats.getMatch(), stats.getCareer());
            break;
    }

    renderer.endFrame();
}

void Game::handleMenuClick(float mx, float my) {
    if (menuScreen == MenuScreen::MAIN) {
        if (UIButtons::BTN_MAIN_START.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            menuScreen = MenuScreen::CHARACTER_SELECT;
            isStartGameFlow = true;
            selectedDifficulty = gameMode;
            if (enemyCharacter == playerCharacter) {
                int nextIdx = ((int)playerCharacter + 1) % Characters::COUNT;
                enemyCharacter = Characters::getByIndex(nextIdx).id;
            }
        } else if (UIButtons::BTN_MAIN_CHARACTERS.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            menuScreen = MenuScreen::CHARACTERS;
            selectedButtonIndex = 0;
        } else if (UIButtons::BTN_MAIN_STATS.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            confirmStatsReset = false;
            menuScreen = MenuScreen::STATS;
        } else if (UIButtons::BTN_MAIN_OPTIONS.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            menuScreen = MenuScreen::OPTIONS;
        } else if (UIButtons::BTN_MAIN_EXIT.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_BACK);
            quitRequested = true;
        }
    } else if (menuScreen == MenuScreen::CHARACTERS) {
        handleCharactersClick(mx, my);
    } else if (menuScreen == MenuScreen::STATS) {
        if (confirmStatsReset) {
            if (UIButtons::BTN_CONFIRM_YES.contains(mx, my)) {
                SoundManager::play(SoundEffect::UI_CLICK);
                stats.resetAll();
                confirmStatsReset = false;
            } else if (UIButtons::BTN_CONFIRM_NO.contains(mx, my)) {
                SoundManager::play(SoundEffect::UI_BACK);
                confirmStatsReset = false;
            }
        } else if (UIButtons::BTN_STATS_BACK.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_BACK);
            menuScreen = MenuScreen::MAIN;
        } else if (UIButtons::BTN_STATS_RESET.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            confirmStatsReset = true;
        }
    } else if (menuScreen == MenuScreen::CHARACTER_SELECT) {
        for (int i = 0; i < Characters::COUNT; i++) {
            if (UIButtons::BTN_CHAR_PLAYER[i].contains(mx, my)) {
                SoundManager::play(SoundEffect::UI_CLICK);
                playerCharacter = Characters::getByIndex(i).id;
                if (enemyCharacter == playerCharacter) {
                    int nextIdx = (i + 1) % Characters::COUNT;
                    enemyCharacter = Characters::getByIndex(nextIdx).id;
                }
                return;
            }
            if (UIButtons::BTN_CHAR_OPPONENT[i].contains(mx, my)) {
                if (i != (int)playerCharacter) {
                    SoundManager::play(SoundEffect::UI_CLICK);
                    enemyCharacter = Characters::getByIndex(i).id;
                }
                return;
            }
        }
        if (UIButtons::BTN_CHAR_BACK.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_BACK);
            isStartGameFlow = false;
            menuScreen = MenuScreen::MAIN;
        } else if (UIButtons::BTN_CHAR_NEXT.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            menuScreen = MenuScreen::DIFFICULTY;
            isStartGameFlow = true;
            selectedDifficulty = gameMode;
        }
    } else if (menuScreen == MenuScreen::OPTIONS) {
        if (UIButtons::BTN_OPT_CONTROLS.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            controlsTabOverride = -1;
            input.cancelRebind();
            selectedButtonIndex = -1;
            menuScreen = MenuScreen::CONTROLS;
        } else if (UIButtons::BTN_OPT_MODE.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            menuScreen = MenuScreen::DIFFICULTY;
            isStartGameFlow = false;
            selectedDifficulty = gameMode;
        } else if (UIButtons::BTN_OPT_BACK.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_BACK);
            menuScreen = MenuScreen::MAIN;
        }
    } else if (menuScreen == MenuScreen::CONTROLS) {
        handleControlsClick(mx, my, false);
    } else if (menuScreen == MenuScreen::DIFFICULTY) {
        if (UIButtons::BTN_DIFF_EASY.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            selectedDifficulty = GameMode::EASY;
        } else if (UIButtons::BTN_DIFF_NORMAL.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            selectedDifficulty = GameMode::NORMAL;
        } else if (UIButtons::BTN_DIFF_HARD.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            selectedDifficulty = GameMode::HARD;
        } else if (UIButtons::BTN_DIFF_BACK.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_BACK);
            if (isStartGameFlow) {
                menuScreen = MenuScreen::CHARACTER_SELECT;
            } else {
                menuScreen = MenuScreen::OPTIONS;
            }
        } else if (UIButtons::BTN_DIFF_OK.contains(mx, my)) {
            gameMode = selectedDifficulty;
            enemy.setDifficulty(gameMode);
            if (isStartGameFlow) {
                SoundManager::play(SoundEffect::UI_START);
                startMatch();
            } else {
                SoundManager::play(SoundEffect::UI_CLICK);
                menuScreen = MenuScreen::OPTIONS;
            }
        }
    }
}

void Game::handleCharactersClick(float mx, float my) {
    for (int i = 0; i < Characters::COUNT; ++i) {
        if (UIButtons::BTN_ROSTER_TABS[i].contains(mx, my)) {
            showcaseCharacter = (CharacterId)i;
            SoundManager::play(SoundEffect::UI_CLICK);
            return;
        }
    }
    if (UIButtons::BTN_ROSTER_PREV.contains(mx, my)) {
        int idx = ((int)showcaseCharacter + Characters::COUNT - 1) % Characters::COUNT;
        showcaseCharacter = (CharacterId)idx;
        SoundManager::play(SoundEffect::UI_CLICK);
        return;
    }
    if (UIButtons::BTN_ROSTER_NEXT.contains(mx, my)) {
        int idx = ((int)showcaseCharacter + 1) % Characters::COUNT;
        showcaseCharacter = (CharacterId)idx;
        SoundManager::play(SoundEffect::UI_CLICK);
        return;
    }
    if (UIButtons::BTN_ROSTER_POSE.contains(mx, my)) {
        showcasePoseCombat = !showcasePoseCombat;
        SoundManager::play(SoundEffect::UI_CLICK);
        return;
    }
    if (UIButtons::BTN_ROSTER_BACK.contains(mx, my)) {
        SoundManager::play(SoundEffect::UI_BACK);
        menuScreen = MenuScreen::MAIN;
        selectedButtonIndex = 1;
        return;
    }
}

void Game::handlePauseClick(float mx, float my) {
    if (popupType == PopupType::ROUND1_RESTART_CONFIRM) {
        if (UIButtons::BTN_ALERT_OK.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_START);
            popupType = PopupType::NONE;
            gameMode = selectedDifficulty;
            enemy.setDifficulty(gameMode);
            startMatch();
        }
        return;
    }

    if (popupType == PopupType::ROUND2_RESTART_PROMPT) {
        if (UIButtons::BTN_CONFIRM_YES.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            popupType = PopupType::NONE;
            matchInProgress = false;
            state = GameState::MENU;
            menuScreen = MenuScreen::DIFFICULTY;
            isStartGameFlow = true;
            selectedDifficulty = gameMode;
        } else if (UIButtons::BTN_CONFIRM_NO.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_BACK);
            popupType = PopupType::NONE;
            selectedDifficulty = gameMode;
            pauseScreen = PauseScreen::PAUSE_DIFFICULTY;
        }
        return;
    }

    if (pauseScreen == PauseScreen::PAUSE_MAIN) {
        if (UIButtons::BTN_PAUSE_RESUME.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            state = GameState::FIGHT;
        } else if (UIButtons::BTN_PAUSE_RESTART.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_START);
            startMatch();
        } else if (UIButtons::BTN_PAUSE_OPTIONS.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            pauseScreen = PauseScreen::PAUSE_OPTIONS;
        } else if (UIButtons::BTN_PAUSE_EXIT.contains(mx, my)) {
            // Exit returns to Home screen (Main Menu), NOT closing full game!
            SoundManager::play(SoundEffect::UI_BACK);
            matchInProgress = false;
            popupType       = PopupType::NONE;
            state = GameState::MENU;
            menuScreen = MenuScreen::MAIN;
        }
    } else if (pauseScreen == PauseScreen::PAUSE_OPTIONS) {
        if (UIButtons::BTN_OPT_CONTROLS.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            controlsTabOverride = -1;
            input.cancelRebind();
            selectedButtonIndex = -1;
            pauseScreen = PauseScreen::PAUSE_CONTROLS;
        } else if (UIButtons::BTN_OPT_MODE.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            pauseScreen = PauseScreen::PAUSE_DIFFICULTY;
            selectedDifficulty = gameMode;
            popupType = PopupType::NONE;
        } else if (UIButtons::BTN_OPT_BACK.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_BACK);
            pauseScreen = PauseScreen::PAUSE_MAIN;
        }
    } else if (pauseScreen == PauseScreen::PAUSE_CONTROLS) {
        handleControlsClick(mx, my, true);
    } else if (pauseScreen == PauseScreen::PAUSE_DIFFICULTY) {
        if (UIButtons::BTN_DIFF_EASY.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            selectedDifficulty = GameMode::EASY;
        } else if (UIButtons::BTN_DIFF_NORMAL.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            selectedDifficulty = GameMode::NORMAL;
        } else if (UIButtons::BTN_DIFF_HARD.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            selectedDifficulty = GameMode::HARD;
        } else if (UIButtons::BTN_DIFF_BACK.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_BACK);
            selectedDifficulty = gameMode;
            pauseScreen = PauseScreen::PAUSE_OPTIONS;
        } else if (UIButtons::BTN_DIFF_OK.contains(mx, my)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            if (selectedDifficulty != gameMode) {
                bool isRoundOne = (roundNumber == 1 && playerWins == 0 && enemyWins == 0);
                if (isRoundOne) {
                    popupType = PopupType::ROUND1_RESTART_CONFIRM;
                } else {
                    popupType = PopupType::ROUND2_RESTART_PROMPT;
                }
            } else {
                pauseScreen = PauseScreen::PAUSE_OPTIONS;
            }
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
                SoundManager::play(SoundEffect::UI_CLICK);
                state = GameState::PAUSED;
                pauseScreen = PauseScreen::PAUSE_MAIN;
            }
        } else if (state == GameState::PAUSED) {
            handlePauseClick(worldX, worldY);
        } else if (state == GameState::GAME_OVER) {
            SoundManager::play(SoundEffect::UI_START);
            startMatch();
        }
    }
}

void Game::onKeyDown(unsigned char key) {
    if (input.isRebinding()) {
        if (key == KEY_ESCAPE || key == KEY_ENTER) {
            input.cancelRebind();
            SoundManager::play(SoundEffect::UI_BACK);
            return;
        }
        if (!input.isRebindingGamepad() && input.feedKeyForRebind((int)key)) {
            SoundManager::play(SoundEffect::UI_CLICK);
            return;
        }
        if (input.isRebindingGamepad()) {
            return; // Ignore other keyboard presses while waiting for gamepad button
        }
    }

    input.keyDown(key);

    switch (state) {
        case GameState::MENU:
            if (menuScreen == MenuScreen::STATS && confirmStatsReset) {
                if (key == 'y' || key == 'Y' || key == KEY_ENTER) {
                    SoundManager::play(SoundEffect::UI_CLICK);
                    stats.resetAll();
                    confirmStatsReset = false;
                } else if (key == 'n' || key == 'N' || key == KEY_ESCAPE) {
                    SoundManager::play(SoundEffect::UI_BACK);
                    confirmStatsReset = false;
                }
                break;
            }
            if (menuScreen == MenuScreen::CHARACTERS) {
                if (key == KEY_ESCAPE || key == KEY_ENTER) {
                    SoundManager::play(SoundEffect::UI_BACK);
                    menuScreen = MenuScreen::MAIN;
                    selectedButtonIndex = 1;
                } else if (key == 'a' || key == 'A' || key == 'q' || key == 'Q') {
                    int idx = ((int)showcaseCharacter + Characters::COUNT - 1) % Characters::COUNT;
                    showcaseCharacter = (CharacterId)idx;
                    SoundManager::play(SoundEffect::UI_CLICK);
                } else if (key == 'd' || key == 'D' || key == 'e' || key == 'E') {
                    int idx = ((int)showcaseCharacter + 1) % Characters::COUNT;
                    showcaseCharacter = (CharacterId)idx;
                    SoundManager::play(SoundEffect::UI_CLICK);
                } else if (key == ' ' || key == 'p' || key == 'P' || key == 't' || key == 'T') {
                    showcasePoseCombat = !showcasePoseCombat;
                    SoundManager::play(SoundEffect::UI_CLICK);
                }
                break;
            }
            if (key == KEY_ENTER) {
                if (menuScreen == MenuScreen::MAIN) {
                    SoundManager::play(SoundEffect::UI_CLICK);
                    menuScreen = MenuScreen::CHARACTER_SELECT;
                    isStartGameFlow = true;
                    selectedDifficulty = gameMode;
                } else if (menuScreen == MenuScreen::CHARACTER_SELECT) {
                    SoundManager::play(SoundEffect::UI_CLICK);
                    menuScreen = MenuScreen::DIFFICULTY;
                    isStartGameFlow = true;
                    selectedDifficulty = gameMode;
                } else if (menuScreen == MenuScreen::STATS) {
                    SoundManager::play(SoundEffect::UI_BACK);
                    menuScreen = MenuScreen::MAIN;
                } else if (menuScreen == MenuScreen::OPTIONS) {
                    SoundManager::play(SoundEffect::UI_CLICK);
                    menuScreen = MenuScreen::MAIN;
                } else if (menuScreen == MenuScreen::CONTROLS) {
                    SoundManager::play(SoundEffect::UI_BACK);
                    menuScreen = MenuScreen::OPTIONS;
                } else if (menuScreen == MenuScreen::DIFFICULTY) {
                    gameMode = selectedDifficulty;
                    enemy.setDifficulty(gameMode);
                    if (isStartGameFlow) {
                        SoundManager::play(SoundEffect::UI_START);
                        startMatch();
                    } else {
                        SoundManager::play(SoundEffect::UI_CLICK);
                        menuScreen = MenuScreen::OPTIONS;
                    }
                }
            } else if (key == KEY_ESCAPE) {
                SoundManager::play(SoundEffect::UI_BACK);
                if (menuScreen == MenuScreen::MAIN) quitRequested = true;
                else if (menuScreen == MenuScreen::CHARACTER_SELECT) {
                    isStartGameFlow = false;
                    menuScreen = MenuScreen::MAIN;
                }
                else if (menuScreen == MenuScreen::STATS) menuScreen = MenuScreen::MAIN;
                else if (menuScreen == MenuScreen::OPTIONS) menuScreen = MenuScreen::MAIN;
                else if (menuScreen == MenuScreen::CONTROLS) menuScreen = MenuScreen::OPTIONS;
                else if (menuScreen == MenuScreen::DIFFICULTY) {
                    if (isStartGameFlow) menuScreen = MenuScreen::CHARACTER_SELECT;
                    else menuScreen = MenuScreen::OPTIONS;
                }
            }
            break;

        case GameState::FIGHT:
            if (key == 'p' || key == 'P' || key == KEY_ESCAPE) {
                SoundManager::play(SoundEffect::UI_CLICK);
                state = GameState::PAUSED;
                pauseScreen = PauseScreen::PAUSE_MAIN;
            }
            if (key == 'h' || key == 'H') showDebugBoxes = !showDebugBoxes;
            if (key == 't' || key == 'T') {
                SoundManager::play(SoundEffect::UI_CLICK);
                player.restoreHealth();
                enemy.restoreHealth();
                player.restoreEnergy();
                enemy.restoreEnergy();
            }
            break;

        case GameState::PAUSED:
            if (popupType == PopupType::ROUND1_RESTART_CONFIRM) {
                if (key == KEY_ENTER || key == ' ') {
                    SoundManager::play(SoundEffect::UI_START);
                    popupType = PopupType::NONE;
                    gameMode = selectedDifficulty;
                    enemy.setDifficulty(gameMode);
                    startMatch();
                } else if (key == KEY_ESCAPE) {
                    SoundManager::play(SoundEffect::UI_BACK);
                    popupType = PopupType::NONE;
                    selectedDifficulty = gameMode;
                }
                break;
            } else if (popupType == PopupType::ROUND2_RESTART_PROMPT) {
                if (key == 'y' || key == 'Y' || key == KEY_ENTER) {
                    SoundManager::play(SoundEffect::UI_CLICK);
                    popupType = PopupType::NONE;
                    matchInProgress = false;
                    state = GameState::MENU;
                    menuScreen = MenuScreen::DIFFICULTY;
                    isStartGameFlow = true;
                    selectedDifficulty = gameMode;
                } else if (key == 'n' || key == 'N' || key == KEY_ESCAPE) {
                    SoundManager::play(SoundEffect::UI_BACK);
                    popupType = PopupType::NONE;
                    selectedDifficulty = gameMode;
                    pauseScreen = PauseScreen::PAUSE_DIFFICULTY;
                }
                break;
            }

            if (key == 'p' || key == 'P' || key == KEY_ESCAPE) {
                SoundManager::play(SoundEffect::UI_BACK);
                if (pauseScreen == PauseScreen::PAUSE_MAIN) {
                    state = GameState::FIGHT;
                } else if (pauseScreen == PauseScreen::PAUSE_OPTIONS) {
                    pauseScreen = PauseScreen::PAUSE_MAIN;
                } else if (pauseScreen == PauseScreen::PAUSE_DIFFICULTY) {
                    selectedDifficulty = gameMode;
                    pauseScreen = PauseScreen::PAUSE_OPTIONS;
                } else {
                    pauseScreen = PauseScreen::PAUSE_OPTIONS;
                }
            }
            if (key == 'r' || key == 'R') {
                SoundManager::play(SoundEffect::UI_START);
                startMatch();
            }
            if (key == 'm' || key == 'M') {
                SoundManager::play(SoundEffect::UI_BACK);
                matchInProgress = false;
                popupType       = PopupType::NONE;
                state = GameState::MENU;
                menuScreen = MenuScreen::MAIN;
            }
            break;

        case GameState::ROUND_OVER:
            break;

        case GameState::GAME_OVER:
            if (key == KEY_ENTER) {
                SoundManager::play(SoundEffect::UI_START);
                startMatch();
            }
            if (key == 'm' || key == 'M' || key == KEY_ESCAPE) {
                SoundManager::play(SoundEffect::UI_BACK);
                matchInProgress = false;
                popupType       = PopupType::NONE;
                state = GameState::MENU;
                menuScreen = MenuScreen::MAIN;
            }
            break;
    }
}

void Game::onKeyUp(unsigned char key) {
    input.keyUp(key);
}

void Game::onSpecialKey(int key, bool released) {
    if (released) {
        input.specialKeyUp(key);
    } else {
        if (state == GameState::MENU && menuScreen == MenuScreen::CHARACTERS) {
            if (key == 100 || key == KeyCodes::ARROW_LEFT) {
                int idx = ((int)showcaseCharacter + Characters::COUNT - 1) % Characters::COUNT;
                showcaseCharacter = (CharacterId)idx;
                SoundManager::play(SoundEffect::UI_CLICK);
                return;
            } else if (key == 102 || key == KeyCodes::ARROW_RIGHT) {
                int idx = ((int)showcaseCharacter + 1) % Characters::COUNT;
                showcaseCharacter = (CharacterId)idx;
                SoundManager::play(SoundEffect::UI_CLICK);
                return;
            }
        }
        bool wasRebinding = input.isRebinding() && !input.isRebindingGamepad();
        input.specialKeyDown(key);
        if (wasRebinding && !input.isRebinding()) {
            SoundManager::play(SoundEffect::UI_CLICK);
        }
    }
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
