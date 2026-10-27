#include "Fighter.h"
#include "Renderer.h"
#include "Animation.h"
#include "Config.h"
#include <cmath>

namespace {
    const float CROUCH_HEIGHT_RATIO = 0.6f;
    const float CROUCH_SPEED_RATIO  = 0.4f;

    const int   MAX_HEALTH = 100;

    const float MAX_ENERGY   = 100.0f;
    const float ENERGY_REGEN = 10.0f;

    // Getting hit
    const float KNOCKBACK_DECEL = 1600.0f;

    // Blocking
    const int   BLOCK_DAMAGE_PERCENT  = 20;
    const float BLOCK_KNOCKBACK_SPEED = 150.0f;
    const float BLOCK_FLASH_TIME      = 0.15f;

    const float RECOVERY_INVULNERABILITY = 0.5f;
    const float BLINK_INTERVAL           = 0.07f;

    struct Vec2 {
        float x;
        float y;
    };

    const AttackData* attackDataFor(FighterState s) {
        switch (s) {
            case FighterState::PUNCH:   return &Attacks::PUNCH;
            case FighterState::KICK:    return &Attacks::KICK;
            case FighterState::SPECIAL: return &Attacks::SPECIAL;
            default:                    return nullptr;
        }
    }
}

Fighter::Fighter(float startX, float startY, float bodyWidth, float bodyHeight)
    : x(startX), y(startY), width(bodyWidth), height(bodyHeight),
      standingHeight(bodyHeight), crouchHeight(bodyHeight * CROUCH_HEIGHT_RATIO),
      moveSpeed(250.0f), velocityY(0.0f), knockbackVelocityX(0.0f),
      moveDirection(0.0f), facingRight(true),
      onGround(false), crouching(false), blocking(false),
      state(FighterState::IDLE), stateTime(0.0f), hitStunDuration(0.0f),
      blockFlashTimer(0.0f), invulnerableTimer(0.0f),
      colorR(0.08f), colorG(0.09f), colorB(0.12f),
      accentR(0.25f), accentG(0.85f), accentB(0.95f),
      health(MAX_HEALTH), maxHealth(MAX_HEALTH),
      energy(MAX_ENERGY), maxEnergy(MAX_ENERGY), attackHasHit(false) {
}

void Fighter::keepInsideWorld() {
    if (x < 0.0f) x = 0.0f;
    if (x + width > Config::WORLD_WIDTH) x = Config::WORLD_WIDTH - width;
}

void Fighter::moveHorizontal(float direction, float deltaTime) {
    if (!canAct() || blocking) direction = 0.0f;
    moveDirection = direction;

    float speed = crouching ? moveSpeed * CROUCH_SPEED_RATIO : moveSpeed;
    x += direction * speed * deltaTime;

    if (direction > 0.0f) facingRight = true;
    else if (direction < 0.0f) facingRight = false;

    keepInsideWorld();
}

void Fighter::jump() {
    if (onGround && canAct()) {
        velocityY = Config::JUMP_SPEED;
        onGround = false;
    }
}

void Fighter::setCrouching(bool wantsToCrouch) {
    if (!canAct()) wantsToCrouch = false;

    if (wantsToCrouch && onGround) {
        crouching = true;
        height = crouchHeight;
    } else {
        crouching = false;
        height = standingHeight;
    }
}

void Fighter::setBlocking(bool wantsToBlock) {
    if (!canAct() || crouching || !onGround) wantsToBlock = false;
    blocking = wantsToBlock;
}

void Fighter::startAttack(FighterState attackState) {
    const AttackData* data = attackDataFor(attackState);
    if (data == nullptr) return;
    if (isDead()) return;

    if (state != FighterState::IDLE && state != FighterState::WALK) return;
    if (!onGround) return;

    if (energy < data->energyCost) return;
    energy -= data->energyCost;

    state = attackState;
    stateTime = 0.0f;
    attackHasHit = false;
}

void Fighter::startPunch() {
    startAttack(FighterState::PUNCH);
}

void Fighter::startKick() {
    startAttack(FighterState::KICK);
}

void Fighter::startSpecial() {
    startAttack(FighterState::SPECIAL);
}

void Fighter::applyPhysics(float deltaTime) {
    velocityY += Config::GRAVITY * deltaTime;
    y += velocityY * deltaTime;

    if (y <= Config::GROUND_HEIGHT) {
        y = Config::GROUND_HEIGHT;
        velocityY = 0.0f;
        onGround = true;
    }

    if (knockbackVelocityX != 0.0f) {
        x += knockbackVelocityX * deltaTime;

        float slowdown = KNOCKBACK_DECEL * deltaTime;
        if (knockbackVelocityX > 0.0f) {
            knockbackVelocityX -= slowdown;
            if (knockbackVelocityX < 0.0f) knockbackVelocityX = 0.0f;
        } else {
            knockbackVelocityX += slowdown;
            if (knockbackVelocityX > 0.0f) knockbackVelocityX = 0.0f;
        }

        keepInsideWorld();
    }
}

void Fighter::updateState(float deltaTime) {
    if (blockFlashTimer > 0.0f) blockFlashTimer -= deltaTime;
    if (invulnerableTimer > 0.0f) invulnerableTimer -= deltaTime;

    energy += ENERGY_REGEN * deltaTime;
    if (energy > maxEnergy) energy = maxEnergy;

    const AttackData* attack = getCurrentAttack();
    FighterState newState;

    if (isDead()) {
        newState = FighterState::DEAD;
    } else if (state == FighterState::HIT && stateTime < hitStunDuration) {
        newState = FighterState::HIT;
    } else if (attack != nullptr && stateTime < attack->totalTime()) {
        newState = state;
    } else if (!onGround) {
        newState = (velocityY > 0.0f) ? FighterState::JUMP : FighterState::FALL;
    } else if (blocking) {
        newState = FighterState::BLOCK;
    } else if (crouching) {
        newState = FighterState::CROUCH;
    } else if (moveDirection != 0.0f) {
        newState = FighterState::WALK;
    } else {
        newState = FighterState::IDLE;
    }

    if (newState != state) {
        if (state == FighterState::HIT) invulnerableTimer = RECOVERY_INVULNERABILITY;

        state = newState;
        stateTime = 0.0f;
    } else {
        stateTime += deltaTime;
    }
}

float Fighter::pushHorizontally(float dx) {
    float oldX = x;
    x += dx;
    keepInsideWorld();
    return x - oldX;
}

void Fighter::setColor(float r, float g, float b) {
    colorR = r;
    colorG = g;
    colorB = b;
}

void Fighter::setAccentColor(float r, float g, float b) {
    accentR = r;
    accentG = g;
    accentB = b;
}

void Fighter::setFacingRight(bool faceRight) {
    facingRight = faceRight;
}

Rect Fighter::getHurtbox() const {
    Rect box = { x, y, width, height };
    return box;
}

bool Fighter::getHitbox(Rect& outBox) const {
    if (attackHasHit) return false;

    const AttackData* attack = getCurrentAttack();
    if (attack == nullptr) return false;
    if (getAttackPhase() != AttackPhase::ACTIVE) return false;

    outBox.width  = attack->reach;
    outBox.height = attack->hitboxHeight;
    outBox.x = facingRight ? (x + width) : (x - attack->reach);
    outBox.y = y + attack->hitboxOffsetY;
    return true;
}

const AttackData* Fighter::getCurrentAttack() const {
    return attackDataFor(state);
}

void Fighter::onAttackConnected() {
    attackHasHit = true;
}

void Fighter::receiveHit(const AttackData& attack, float pushDirection) {
    if (isDead()) return;

    bool attackerIsOnRight = (pushDirection < 0.0f);
    bool facingAttacker = (attackerIsOnRight == facingRight);

    if (blocking && facingAttacker && attack.blockable) {
        int chipDamage = attack.damage * BLOCK_DAMAGE_PERCENT / 100;
        if (chipDamage < 1) chipDamage = 1;
        takeDamage(chipDamage);

        knockbackVelocityX = pushDirection * BLOCK_KNOCKBACK_SPEED;
        blockFlashTimer = BLOCK_FLASH_TIME;
        return;
    }

    takeDamage(attack.damage);

    if (isDead()) {
        state = FighterState::DEAD;
        stateTime = 0.0f;
        knockbackVelocityX = pushDirection * attack.knockbackSpeed;
        crouching = false;
        blocking = false;
        height = standingHeight;
        return;
    }
    state = FighterState::HIT;
    stateTime = 0.0f;
    hitStunDuration = attack.hitStun;
    knockbackVelocityX = pushDirection * attack.knockbackSpeed;

    crouching = false;
    blocking = false;
    height = standingHeight;
}

void Fighter::takeDamage(int amount) {
    health -= amount;
    if (health < 0) {
        health = 0;
    }
}

void Fighter::restoreHealth() {
    health = maxHealth;
}

int Fighter::getHealth() const {
    return health;
}

int Fighter::getMaxHealth() const {
    return maxHealth;
}

bool Fighter::isDead() const {
    return health <= 0;
}

void Fighter::restoreEnergy() {
    energy = maxEnergy;
}

float Fighter::getEnergy() const {
    return energy;
}

float Fighter::getMaxEnergy() const {
    return maxEnergy;
}

bool Fighter::isInvulnerable() const {
    return invulnerableTimer > 0.0f;
}

bool Fighter::canAct() const {
    return !isAttacking() && state != FighterState::HIT &&
           state != FighterState::DEAD && !isDead();
}

bool Fighter::isAttacking() const {
    return getCurrentAttack() != nullptr;
}

AttackPhase Fighter::getAttackPhase() const {
    const AttackData* attack = getCurrentAttack();
    if (attack == nullptr)                            return AttackPhase::NONE;
    if (stateTime < attack->startup)                  return AttackPhase::STARTUP;
    if (stateTime < attack->startup + attack->active) return AttackPhase::ACTIVE;
    if (stateTime < attack->totalTime())              return AttackPhase::RECOVERY;
    return AttackPhase::NONE;
}

FighterState Fighter::getState() const {
    return state;
}

const char* Fighter::getStateName() const {
    switch (state) {
        case FighterState::IDLE:    return "IDLE";
        case FighterState::WALK:    return "WALK";
        case FighterState::JUMP:    return "JUMP";
        case FighterState::FALL:    return "FALL";
        case FighterState::CROUCH:  return "CROUCH";
        case FighterState::BLOCK:   return "BLOCK";
        case FighterState::PUNCH:   return "PUNCH";
        case FighterState::KICK:    return "KICK";
        case FighterState::SPECIAL: return "SPECIAL";
        case FighterState::HIT:     return "HIT";
        case FighterState::DEAD:    return "DEAD";
    }
    return "UNKNOWN";
}

void Fighter::draw(const Renderer& renderer) const {
    Pose pose = Animation::getPose(state, stateTime);
    AttackPhase phase = getAttackPhase();

    float bodyR = colorR, bodyG = colorG, bodyB = colorB;
    if (state == FighterState::HIT) {
        bodyR = 0.95f; bodyG = 0.20f; bodyB = 0.20f;
    } else if (state == FighterState::SPECIAL && phase == AttackPhase::STARTUP) {
        bodyR = 0.40f; bodyG = 0.85f; bodyB = 1.0f;
    }

    if (state == FighterState::DEAD) {
        bodyR *= 0.35f; bodyG *= 0.35f; bodyB *= 0.35f;
    } else if (isInvulnerable() && ((int)(invulnerableTimer / BLINK_INTERVAL) % 2 == 0)) {
        bodyR *= 0.45f; bodyG *= 0.45f; bodyB *= 0.45f;
    }

    // Depth-shading for back limbs (30% darker)
    float backR = bodyR * 0.70f;
    float backG = bodyG * 0.70f;
    float backB = bodyB * 0.70f;

    float drawW = width  * pose.scaleX;
    float drawH = height * pose.scaleY;
    float drawX = x + (width - drawW) / 2.0f;
    float drawY = y + pose.offsetY;
    float cx    = drawX + drawW * 0.5f;
    float dir   = facingRight ? 1.0f : -1.0f;

    // Joint declarations
    Vec2 headCenter, neck, chest, waist, hips;
    Vec2 shoulderF, elbowF, handF;
    Vec2 shoulderB, elbowB, handB;
    Vec2 hipF, kneeF, ankleF, toeF;
    Vec2 hipB, kneeB, ankleB, toeB;

    float headRadius = 8.5f;
    bool  drawEye    = (state != FighterState::DEAD);

    // =========================================================================
    // JOINT SOLVER BASED ON MARTIAL-ARTS STANCE / STATE
    // =========================================================================
    if (state == FighterState::DEAD) {
        // Collapsed silhouette resting on the arena stone floor
        headCenter = { cx + dir * 32.0f, drawY + 8.0f };
        neck       = { cx + dir * 24.0f, drawY + 6.0f };
        chest      = { cx + dir * 14.0f, drawY + 6.0f };
        waist      = { cx,               drawY + 5.0f };
        hips       = { cx - dir * 10.0f, drawY + 5.0f };

        // Sprawled legs
        hipF   = { hips.x + dir * 2.0f, hips.y };
        kneeF  = { cx - dir * 22.0f, drawY + 4.0f };
        ankleF = { cx - dir * 34.0f, drawY + 3.0f };
        toeF   = { cx - dir * 39.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 2.0f, hips.y };
        kneeB  = { cx - dir * 18.0f, drawY + 6.0f };
        ankleB = { cx - dir * 30.0f, drawY + 5.0f };
        toeB   = { cx - dir * 35.0f, drawY + 4.0f };

        // Limbs on floor
        shoulderF = { chest.x + dir * 2.0f, chest.y };
        elbowF    = { cx + dir * 8.0f,  drawY + 3.0f };
        handF     = { cx,               drawY + 2.0f };

        shoulderB = { chest.x - dir * 2.0f, chest.y };
        elbowB    = { cx + dir * 18.0f, drawY + 7.0f };
        handB     = { cx + dir * 26.0f, drawY + 7.0f };

    } else if (state == FighterState::CROUCH) {
        // Low crouching tiger martial-arts guard
        hips       = { cx - dir * 4.0f,  drawY + 22.0f };
        waist      = { cx,               drawY + 30.0f };
        chest      = { cx + dir * 8.0f,  drawY + 40.0f };
        neck       = { cx + dir * 12.0f, drawY + 46.0f };
        headCenter = { cx + dir * 16.0f, drawY + 54.0f };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * 18.0f, drawY + 14.0f };
        ankleF = { cx + dir * 14.0f, drawY + 4.0f };
        toeF   = { cx + dir * 21.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 10.0f, drawY + 11.0f };
        ankleB = { cx - dir * 16.0f, drawY + 4.0f };
        toeB   = { cx - dir * 11.0f, drawY + 2.0f };

        shoulderF = { chest.x + dir * 2.0f, chest.y };
        elbowF    = { cx + dir * 18.0f, drawY + 28.0f };
        handF     = { cx + dir * 22.0f, drawY + 34.0f };

        shoulderB = { chest.x - dir * 3.0f, chest.y };
        elbowB    = { cx + dir * 6.0f,  drawY + 24.0f };
        handB     = { cx + dir * 12.0f, drawY + 28.0f };

    } else if (state == FighterState::JUMP || state == FighterState::FALL) {
        // Airborne martial-arts tuck
        hips       = { cx,              drawY + 44.0f };
        waist      = { cx + dir * 2.0f, drawY + 52.0f };
        chest      = { cx + dir * 5.0f, drawY + 68.0f };
        neck       = { cx + dir * 7.0f, drawY + 76.0f };
        headCenter = { cx + dir * 9.0f, drawY + 86.0f };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * 14.0f, drawY + 28.0f };
        ankleF = { cx + dir * 8.0f,  drawY + 14.0f };
        toeF   = { cx + dir * 13.0f, drawY + 11.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 10.0f, drawY + 22.0f };
        ankleB = { cx - dir * 16.0f, drawY + 8.0f };
        toeB   = { cx - dir * 12.0f, drawY + 5.0f };

        shoulderF = { chest.x + dir * 3.0f, chest.y };
        elbowF    = { cx + dir * 16.0f, drawY + 72.0f };
        handF     = { cx + dir * 22.0f, drawY + 78.0f };

        shoulderB = { chest.x - dir * 3.0f, chest.y };
        elbowB    = { cx - dir * 10.0f, drawY + 58.0f };
        handB     = { cx - dir * 16.0f, drawY + 50.0f };

    } else if (state == FighterState::BLOCK) {
        // Reinforced crossed-forearm iron guard
        hips       = { cx - dir * 3.0f, drawY + 44.0f };
        waist      = { cx - dir * 2.0f, drawY + 52.0f };
        chest      = { cx,              drawY + 70.0f };
        neck       = { cx + dir * 1.0f, drawY + 77.0f };
        headCenter = { cx + dir * 2.0f, drawY + 87.0f };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * 12.0f, drawY + 22.0f };
        ankleF = { cx + dir * 14.0f, drawY + 3.0f };
        toeF   = { cx + dir * 20.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 10.0f, drawY + 22.0f };
        ankleB = { cx - dir * 16.0f, drawY + 3.0f };
        toeB   = { cx - dir * 10.0f, drawY + 2.0f };

        shoulderF = { chest.x + dir * 3.0f, chest.y };
        elbowF    = { cx + dir * 13.0f, drawY + 64.0f };
        handF     = { cx + dir * 15.0f, drawY + 78.0f };

        shoulderB = { chest.x - dir * 2.0f, chest.y };
        elbowB    = { cx + dir * 9.0f,  drawY + 60.0f };
        handB     = { cx + dir * 11.0f, drawY + 76.0f };

    } else if (state == FighterState::HIT) {
        // Shockwave impact recoil
        hips       = { cx - dir * 6.0f,  drawY + 44.0f };
        waist      = { cx - dir * 9.0f,  drawY + 53.0f };
        chest      = { cx - dir * 14.0f, drawY + 70.0f };
        neck       = { cx - dir * 17.0f, drawY + 78.0f };
        headCenter = { cx - dir * 20.0f, drawY + 87.0f };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * 6.0f,  drawY + 20.0f };
        ankleF = { cx + dir * 10.0f, drawY + 3.0f };
        toeF   = { cx + dir * 16.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 14.0f, drawY + 18.0f };
        ankleB = { cx - dir * 18.0f, drawY + 3.0f };
        toeB   = { cx - dir * 12.0f, drawY + 2.0f };

        shoulderF = { chest.x + dir * 2.0f, chest.y };
        elbowF    = { cx - dir * 2.0f,  drawY + 66.0f };
        handF     = { cx + dir * 8.0f,  drawY + 64.0f };

        shoulderB = { chest.x - dir * 3.0f, chest.y };
        elbowB    = { cx - dir * 20.0f, drawY + 62.0f };
        handB     = { cx - dir * 26.0f, drawY + 54.0f };

    } else if (state == FighterState::PUNCH) {
        // Driving martial-arts straight punch
        hips       = { cx + dir * 2.0f,  drawY + 46.0f };
        waist      = { cx + dir * 5.0f,  drawY + 54.0f };
        chest      = { cx + dir * 9.0f,  drawY + 72.0f };
        neck       = { cx + dir * 11.0f, drawY + 80.0f };
        headCenter = { cx + dir * 13.0f, drawY + 90.0f };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * 16.0f, drawY + 24.0f };
        ankleF = { cx + dir * 18.0f, drawY + 3.0f };
        toeF   = { cx + dir * 24.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 10.0f, drawY + 22.0f };
        ankleB = { cx - dir * 16.0f, drawY + 3.0f };
        toeB   = { cx - dir * 10.0f, drawY + 2.0f };

        shoulderF = { chest.x + dir * 4.0f, chest.y };
        shoulderB = { chest.x - dir * 4.0f, chest.y };

        if (phase == AttackPhase::STARTUP) {
            elbowF = { cx + dir * 8.0f,  drawY + 66.0f };
            handF  = { cx + dir * 12.0f, drawY + 70.0f };
        } else if (phase == AttackPhase::ACTIVE) {
            float punchReach = Attacks::PUNCH.reach;
            float punchY = drawY + Attacks::PUNCH.hitboxOffsetY + Attacks::PUNCH.hitboxHeight * 0.5f;
            float targetHandX = (facingRight ? (drawX + drawW) : drawX) + dir * punchReach * 0.95f;

            elbowF = { (shoulderF.x + targetHandX) * 0.5f, punchY - 2.0f };
            handF  = { targetHandX, punchY };
        } else { // RECOVERY
            elbowF = { cx + dir * 14.0f, drawY + 68.0f };
            handF  = { cx + dir * 20.0f, drawY + 70.0f };
        }

        // Back arm chambered tightly at ribcage
        elbowB = { cx - dir * 5.0f, drawY + 60.0f };
        handB  = { cx + dir * 2.0f, drawY + 62.0f };

    } else if (state == FighterState::KICK) {
        // Snapping martial-arts kick
        hips       = { cx,               drawY + 46.0f };
        waist      = { cx - dir * 4.0f,  drawY + 54.0f };
        chest      = { cx - dir * 10.0f, drawY + 70.0f };
        neck       = { cx - dir * 12.0f, drawY + 78.0f };
        headCenter = { cx - dir * 14.0f, drawY + 87.0f };

        // Supporting leg (back leg planted)
        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 6.0f,  drawY + 22.0f };
        ankleB = { cx - dir * 8.0f,  drawY + 3.0f };
        toeB   = { cx - dir * 3.0f,  drawY + 2.0f };

        // Kicking leg (front leg)
        hipF = { hips.x + dir * 4.0f, hips.y };
        if (phase == AttackPhase::STARTUP) {
            kneeF  = { cx + dir * 12.0f, drawY + 48.0f };
            ankleF = { cx + dir * 6.0f,  drawY + 36.0f };
            toeF   = { cx + dir * 10.0f, drawY + 38.0f };
        } else if (phase == AttackPhase::ACTIVE) {
            float kickReach = Attacks::KICK.reach;
            float kickY = drawY + Attacks::KICK.hitboxOffsetY + Attacks::KICK.hitboxHeight * 0.5f;
            float targetFootX = (facingRight ? (drawX + drawW) : drawX) + dir * kickReach * 0.95f;

            kneeF  = { (hipF.x + targetFootX) * 0.50f, kickY + 6.0f };
            ankleF = { targetFootX - dir * 4.0f, kickY };
            toeF   = { targetFootX, kickY };
        } else { // RECOVERY
            kneeF  = { cx + dir * 14.0f, drawY + 36.0f };
            ankleF = { cx + dir * 10.0f, drawY + 20.0f };
            toeF   = { cx + dir * 15.0f, drawY + 18.0f };
        }

        shoulderF = { chest.x + dir * 3.0f, chest.y };
        elbowF    = { cx + dir * 4.0f,  drawY + 66.0f };
        handF     = { cx + dir * 10.0f, drawY + 68.0f };

        shoulderB = { chest.x - dir * 4.0f, chest.y };
        elbowB    = { cx - dir * 16.0f, drawY + 58.0f };
        handB     = { cx - dir * 20.0f, drawY + 52.0f };

    } else if (state == FighterState::SPECIAL) {
        // Devastating powerhouse thrust
        hips       = { cx + dir * 3.0f,  drawY + 46.0f };
        waist      = { cx + dir * 6.0f,  drawY + 54.0f };
        chest      = { cx + dir * 10.0f, drawY + 72.0f };
        neck       = { cx + dir * 12.0f, drawY + 80.0f };
        headCenter = { cx + dir * 14.0f, drawY + 90.0f };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * 18.0f, drawY + 22.0f };
        ankleF = { cx + dir * 22.0f, drawY + 3.0f };
        toeF   = { cx + dir * 28.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 12.0f, drawY + 20.0f };
        ankleB = { cx - dir * 18.0f, drawY + 3.0f };
        toeB   = { cx - dir * 12.0f, drawY + 2.0f };

        shoulderF = { chest.x + dir * 5.0f, chest.y };
        shoulderB = { chest.x - dir * 4.0f, chest.y };

        if (phase == AttackPhase::STARTUP) {
            elbowF = { cx + dir * 4.0f,  drawY + 62.0f };
            handF  = { cx + dir * 8.0f,  drawY + 64.0f };
        } else if (phase == AttackPhase::ACTIVE) {
            float specReach = Attacks::SPECIAL.reach;
            float specY = drawY + Attacks::SPECIAL.hitboxOffsetY + Attacks::SPECIAL.hitboxHeight * 0.5f;
            float targetHandX = (facingRight ? (drawX + drawW) : drawX) + dir * specReach * 0.95f;

            elbowF = { (shoulderF.x + targetHandX) * 0.5f, specY - 2.0f };
            handF  = { targetHandX, specY };
        } else { // RECOVERY
            elbowF = { cx + dir * 16.0f, drawY + 68.0f };
            handF  = { cx + dir * 22.0f, drawY + 70.0f };
        }

        elbowB = { cx - dir * 8.0f,  drawY + 58.0f };
        handB  = { cx - dir * 12.0f, drawY + 54.0f };

    } else {
        // Default IDLE / WALK: Athletic martial-arts combat stance
        hips       = { cx,              drawY + 46.0f };
        waist      = { cx + dir * 1.0f, drawY + 54.0f };
        chest      = { cx + dir * 3.0f, drawY + 72.0f };
        neck       = { cx + dir * 4.0f, drawY + 80.0f };
        headCenter = { cx + dir * 5.0f, drawY + 90.0f };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * 13.0f, drawY + 24.0f };
        ankleF = { cx + dir * 15.0f, drawY + 3.0f };
        toeF   = { cx + dir * 21.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 8.0f,  drawY + 24.0f };
        ankleB = { cx - dir * 14.0f, drawY + 3.0f };
        toeB   = { cx - dir * 8.0f,  drawY + 2.0f };

        shoulderF = { chest.x + dir * 4.0f, chest.y };
        elbowF    = { cx + dir * 14.0f, drawY + 58.0f };
        handF     = { cx + dir * 18.0f, drawY + 68.0f };

        shoulderB = { chest.x - dir * 3.0f, chest.y };
        elbowB    = { cx - dir * 4.0f,  drawY + 56.0f };
        handB     = { cx + dir * 6.0f,  drawY + 64.0f };
    }

    // =========================================================================
    // LAYER 1: BACK LIMBS (Depth-sorted behind torso with darker shadow tint)
    // =========================================================================
    // Back Leg
    renderer.drawLimb(hipB.x, hipB.y, kneeB.x, kneeB.y, 5.5f, 4.5f, backR, backG, backB);
    renderer.drawLimb(kneeB.x, kneeB.y, ankleB.x, ankleB.y, 4.5f, 3.5f, backR, backG, backB);
    renderer.drawLimb(ankleB.x, ankleB.y, toeB.x, toeB.y, 3.5f, 3.0f, backR, backG, backB);

    // Back Arm
    renderer.drawLimb(shoulderB.x, shoulderB.y, elbowB.x, elbowB.y, 4.2f, 3.5f, backR, backG, backB);
    renderer.drawLimb(elbowB.x, elbowB.y, handB.x, handB.y, 3.5f, 3.0f, backR, backG, backB);
    renderer.drawCircle(handB.x, handB.y, 3.5f, backR, backG, backB);

    // =========================================================================
    // LAYER 2: TORSO & BELT SASH (Middle layer)
    // =========================================================================
    // Upper chest & shoulders (tapered athletic silhouette)
    renderer.drawQuad(chest.x - 9.0f, chest.y,
                      chest.x + 9.0f, chest.y,
                      waist.x + 6.5f, waist.y,
                      waist.x - 6.5f, waist.y,
                      bodyR, bodyG, bodyB);

    // Lower abdomen & hips
    renderer.drawQuad(waist.x - 6.5f, waist.y,
                      waist.x + 6.5f, waist.y,
                      hips.x + 7.5f,  hips.y,
                      hips.x - 7.5f,  hips.y,
                      bodyR, bodyG, bodyB);

    // Muscular neck
    renderer.drawLimb(neck.x, neck.y - 2.0f, headCenter.x, headCenter.y - 4.0f, 3.2f, 3.8f, bodyR, bodyG, bodyB);

    // Martial-arts belt / sash (Obi) wrapped around waist
    renderer.drawRect(waist.x - 7.5f, waist.y - 2.5f, 15.0f, 5.0f, accentR, accentG, accentB);

    // Hanging sash ribbon tails fluttering in the wind
    if (state != FighterState::DEAD) {
        float sashWave = std::sin(stateTime * 6.0f) * 2.0f;
        renderer.drawQuad(waist.x - dir * 2.0f, waist.y - 2.0f,
                          waist.x - dir * 6.0f, waist.y - 2.0f,
                          waist.x - dir * 9.0f + sashWave, waist.y - 14.0f,
                          waist.x - dir * 5.0f + sashWave, waist.y - 14.0f,
                          accentR, accentG, accentB);
    }

    // =========================================================================
    // LAYER 3: FRONT LIMBS (Foreground layer with primary silhouette tone)
    // =========================================================================
    // Front Leg
    renderer.drawLimb(hipF.x, hipF.y, kneeF.x, kneeF.y, 5.5f, 4.5f, bodyR, bodyG, bodyB);
    renderer.drawLimb(kneeF.x, kneeF.y, ankleF.x, ankleF.y, 4.5f, 3.5f, bodyR, bodyG, bodyB);
    renderer.drawLimb(ankleF.x, ankleF.y, toeF.x, toeF.y, 3.5f, 3.0f, bodyR, bodyG, bodyB);

    // Front Arm
    renderer.drawLimb(shoulderF.x, shoulderF.y, elbowF.x, elbowF.y, 4.2f, 3.5f, bodyR, bodyG, bodyB);
    renderer.drawLimb(elbowF.x, elbowF.y, handF.x, handF.y, 3.5f, 3.0f, bodyR, bodyG, bodyB);
    renderer.drawCircle(handF.x, handF.y, 3.5f, bodyR, bodyG, bodyB);

    // =========================================================================
    // LAYER 4: HEAD, HEADBAND & GLOWING EYE SLIT
    // =========================================================================
    // Ninja cowl / hood head shape
    renderer.drawCircle(headCenter.x, headCenter.y, headRadius, bodyR, bodyG, bodyB);
    // Tapered chin
    renderer.drawTriangle(headCenter.x - 6.5f, headCenter.y - 2.0f,
                          headCenter.x + 6.5f, headCenter.y - 2.0f,
                          headCenter.x + dir * 2.0f, headCenter.y - 10.0f,
                          bodyR, bodyG, bodyB);

    if (state != FighterState::DEAD) {
        // Headband (Hachimaki) wrapped around forehead
        renderer.drawRect(headCenter.x - 8.5f, headCenter.y + 0.5f, 17.0f, 3.5f, accentR, accentG, accentB);

        // Wind-blown headband ribbons trailing behind head
        float tailWave1 = std::sin(stateTime * 10.0f) * 2.5f;
        float tailWave2 = std::cos(stateTime * 8.0f) * 3.0f;

        // Tail 1
        renderer.drawQuad(headCenter.x - dir * 7.5f, headCenter.y + 3.0f,
                          headCenter.x - dir * 7.5f, headCenter.y + 1.0f,
                          headCenter.x - dir * 18.0f, headCenter.y - 2.0f + tailWave1,
                          headCenter.x - dir * 18.0f, headCenter.y + 0.5f + tailWave1,
                          accentR, accentG, accentB);

        // Tail 2 (slightly longer and darker)
        renderer.drawQuad(headCenter.x - dir * 7.5f, headCenter.y + 1.5f,
                          headCenter.x - dir * 7.5f, headCenter.y - 0.5f,
                          headCenter.x - dir * 22.0f, headCenter.y - 7.0f + tailWave2,
                          headCenter.x - dir * 22.0f, headCenter.y - 5.0f + tailWave2,
                          accentR * 0.85f, accentG * 0.85f, accentB * 0.85f);

        // Razor-sharp glowing eye slit
        if (drawEye) {
            float eyeX = headCenter.x + dir * 4.0f;
            float eyeY = headCenter.y + 0.2f;
            renderer.drawLine(eyeX - dir * 2.0f, eyeY - 0.8f,
                              eyeX + dir * 3.0f, eyeY + 0.8f,
                              accentR, accentG, accentB, 1.0f, 2.0f);
            // White-hot ocular center
            renderer.drawCircle(eyeX + dir * 1.0f, eyeY + 0.3f, 1.2f, 1.0f, 1.0f, 1.0f, 0.95f, 8);
        }
    }

    // =========================================================================
    // LAYER 5: COMBAT AURAS (Shield & Special attack energy)
    // =========================================================================
    if (state == FighterState::BLOCK) {
        bool flashing = (blockFlashTimer > 0.0f);
        float shieldX = cx + dir * 22.0f;
        float shieldY = drawY + 20.0f;
        float shieldW = flashing ? 12.0f : 8.0f;
        float shieldH = drawH * 0.75f;

        if (flashing) {
            renderer.drawRectAlpha(shieldX - shieldW * 0.5f, shieldY, shieldW, shieldH,
                                   1.0f, 1.0f, 0.70f, 0.85f);
            renderer.drawCircle(shieldX, shieldY + shieldH * 0.5f, 24.0f, 1.0f, 0.95f, 0.5f, 0.30f, 20);
        } else {
            renderer.drawRectAlpha(shieldX - shieldW * 0.5f, shieldY, shieldW, shieldH,
                                   0.30f, 0.65f, 1.0f, 0.65f);
        }
    }

    if (state == FighterState::SPECIAL && phase == AttackPhase::ACTIVE) {
        // Fiery dragon energy aura gathering on striking hand
        renderer.drawCircle(handF.x, handF.y, 16.0f, 1.0f, 0.65f, 0.15f, 0.40f, 18);
        renderer.drawCircle(handF.x, handF.y,  9.0f, 1.0f, 0.90f, 0.35f, 0.80f, 14);
        renderer.drawCircle(handF.x, handF.y,  4.0f, 1.0f, 1.0f,  1.0f,  0.95f, 10);
    }
}
