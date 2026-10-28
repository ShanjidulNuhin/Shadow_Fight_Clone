#include "Fighter.h"
#include "Renderer.h"
#include "Animation.h"
#include "Config.h"
#include <cmath>
#include <algorithm>

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
    : role(FighterRole::NINJA), damageMultiplier(1.0f),
      x(startX), y(startY), width(bodyWidth), height(bodyHeight),
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

void Fighter::setRole(FighterRole r) {
    role = r;
}

FighterRole Fighter::getRole() const {
    return role;
}

void Fighter::setDamageMultiplier(float mult) {
    damageMultiplier = mult;
}

float Fighter::getDamageMultiplier() const {
    return damageMultiplier;
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

void Fighter::receiveHit(const AttackData& attack, float pushDirection, int customDamage) {
    if (isDead()) return;

    bool attackerIsOnRight = (pushDirection < 0.0f);
    bool facingAttacker = (attackerIsOnRight == facingRight);

    int rawDamage = (customDamage >= 0) ? customDamage : attack.damage;

    if (blocking && facingAttacker && attack.blockable) {
        int chipDamage = rawDamage * BLOCK_DAMAGE_PERCENT / 100;
        if (chipDamage < 1) chipDamage = 1;
        takeDamage(chipDamage);

        knockbackVelocityX = pushDirection * BLOCK_KNOCKBACK_SPEED;
        blockFlashTimer = BLOCK_FLASH_TIME;
        return;
    }

    takeDamage(rawDamage);

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
    Pose pose = Animation::getPose(state, stateTime, getCurrentAttack(), hitStunDuration);
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
    // DYNAMIC JOINT SOLVER WITH CONTINUOUS PROCEDURAL ANIMATION
    // =========================================================================
    if (state == FighterState::DEAD) {
        // Smooth dynamic ragdoll collapse: interpolates from standing to flat
        float col = pose.collapse;
        float invCol = 1.0f - col;

        headCenter = { cx + dir * (5.0f * invCol + 32.0f * col), drawY + 90.0f * invCol + 8.0f * col };
        neck       = { cx + dir * (4.0f * invCol + 24.0f * col), drawY + 80.0f * invCol + 6.0f * col };
        chest      = { cx + dir * (3.0f * invCol + 14.0f * col), drawY + 72.0f * invCol + 6.0f * col };
        waist      = { cx + dir * (1.0f * invCol),               drawY + 54.0f * invCol + 5.0f * col };
        hips       = { cx - dir * (10.0f * col),                 drawY + 46.0f * invCol + 5.0f * col };

        // Front leg collapses
        hipF   = { hips.x + dir * 2.0f, hips.y };
        kneeF  = { cx + dir * (13.0f * invCol - 22.0f * col), drawY + 24.0f * invCol + 4.0f * col };
        ankleF = { cx + dir * (15.0f * invCol - 34.0f * col), drawY + 3.0f };
        toeF   = { ankleF.x + dir * (6.0f * invCol - 5.0f * col), ankleF.y - 1.0f };

        // Back leg collapses
        hipB   = { hips.x - dir * 2.0f, hips.y };
        kneeB  = { cx - dir * (8.0f * invCol + 18.0f * col),  drawY + 24.0f * invCol + 6.0f * col };
        ankleB = { cx - dir * (14.0f * invCol + 30.0f * col), drawY + 3.0f * invCol + 5.0f * col };
        toeB   = { ankleB.x - dir * 5.0f, ankleB.y - 1.0f };

        // Arms drop limp to the floor
        shoulderF = { chest.x + dir * 2.0f, chest.y };
        elbowF    = { cx + dir * (14.0f * invCol + 8.0f * col), drawY + 58.0f * invCol + 3.0f * col };
        handF     = { cx + dir * (18.0f * invCol),              drawY + 68.0f * invCol + 2.0f * col };

        shoulderB = { chest.x - dir * 2.0f, chest.y };
        elbowB    = { cx + dir * (-4.0f * invCol + 18.0f * col), drawY + 56.0f * invCol + 7.0f * col };
        handB     = { cx + dir * (6.0f * invCol + 26.0f * col),  drawY + 64.0f * invCol + 7.0f * col };

        if (col >= 0.85f) drawEye = false;

    } else if (state == FighterState::WALK) {
        // Continuous martial-arts stride cycle with swinging limbs & counter-sway
        hips       = { cx + dir * pose.torsoLeanX * 0.4f, drawY + 46.0f + pose.offsetY };
        waist      = { cx + dir * pose.torsoLeanX * 0.7f, drawY + 54.0f + pose.offsetY };
        chest      = { cx + dir * pose.torsoLeanX,        drawY + 72.0f + pose.offsetY };
        neck       = { chest.x + dir * 2.0f,              chest.y + 8.0f };
        headCenter = { neck.x + dir * 1.5f,               neck.y + 10.0f };

        // Front leg stride & lift
        hipF   = { hips.x + dir * 4.0f, hips.y };
        ankleF = { cx + dir * (15.0f + pose.strideLegF), drawY + 3.0f + pose.liftLegF };
        toeF   = { ankleF.x + dir * 6.0f, ankleF.y - 1.0f };
        kneeF  = { (hipF.x + ankleF.x) * 0.5f + dir * 4.0f, (hipF.y + ankleF.y) * 0.5f + pose.liftLegF * 0.6f };

        // Back leg stride & lift (opposite phase)
        hipB   = { hips.x - dir * 4.0f, hips.y };
        ankleB = { cx + dir * (-14.0f + pose.strideLegB), drawY + 3.0f + pose.liftLegB };
        toeB   = { ankleB.x + dir * 5.0f, ankleB.y - 1.0f };
        kneeB  = { (hipB.x + ankleB.x) * 0.5f + dir * 3.0f, (hipB.y + ankleB.y) * 0.5f + pose.liftLegB * 0.6f };

        // Arms swinging in rhythm with strides
        shoulderF = { chest.x + dir * 4.0f, chest.y };
        elbowF    = { cx + dir * (14.0f + pose.armSwingF * 0.6f), drawY + 58.0f + pose.offsetY };
        handF     = { cx + dir * (18.0f + pose.armSwingF),        drawY + 68.0f + pose.offsetY };

        shoulderB = { chest.x - dir * 3.0f, chest.y };
        elbowB    = { cx + dir * (-4.0f + pose.armSwingB * 0.6f), drawY + 56.0f + pose.offsetY };
        handB     = { cx + dir * (6.0f + pose.armSwingB),         drawY + 64.0f + pose.offsetY };

    } else if (state == FighterState::CROUCH) {
        // Deep crouching tiger martial-arts guard
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
        // Fluid transition into iron guard using blockBrace
        float b = pose.blockBrace;
        float invB = 1.0f - b;

        hips       = { cx - dir * (3.0f * b),              drawY + 46.0f - 2.0f * b };
        waist      = { cx + dir * (1.0f * invB - 2.0f * b), drawY + 54.0f - 2.0f * b };
        chest      = { cx + dir * (3.0f * invB),           drawY + 72.0f - 2.0f * b };
        neck       = { cx + dir * (4.0f * invB + 1.0f * b), drawY + 80.0f - 3.0f * b };
        headCenter = { cx + dir * (5.0f * invB + 2.0f * b), drawY + 90.0f - 3.0f * b };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * (13.0f * invB + 12.0f * b), drawY + 24.0f - 2.0f * b };
        ankleF = { cx + dir * (15.0f * invB + 14.0f * b), drawY + 3.0f };
        toeF   = { cx + dir * 20.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * (8.0f * invB + 10.0f * b),  drawY + 24.0f - 2.0f * b };
        ankleB = { cx - dir * (14.0f * invB + 16.0f * b), drawY + 3.0f };
        toeB   = { cx - dir * 10.0f, drawY + 2.0f };

        // Crossed arms raised into guard
        shoulderF = { chest.x + dir * 3.0f, chest.y };
        elbowF    = { cx + dir * (14.0f * invB + 13.0f * b), drawY + 58.0f * invB + 64.0f * b };
        handF     = { cx + dir * (18.0f * invB + 15.0f * b), drawY + 68.0f * invB + 78.0f * b };

        shoulderB = { chest.x - dir * 2.0f, chest.y };
        elbowB    = { cx + dir * (-4.0f * invB + 9.0f * b), drawY + 56.0f * invB + 60.0f * b };
        handB     = { cx + dir * (6.0f * invB + 11.0f * b), drawY + 64.0f * invB + 76.0f * b };

    } else if (state == FighterState::HIT) {
        // Shockwave impact recoil with damped harmonic recovery
        hips       = { cx + dir * (pose.hitRecoilX * 0.4f), drawY + 44.0f };
        waist      = { cx + dir * (pose.hitRecoilX * 0.7f), drawY + 53.0f };
        chest      = { cx + dir * (pose.hitRecoilX + pose.torsoLeanX * 0.5f), drawY + 70.0f };
        neck       = { chest.x - dir * 3.0f, chest.y + 8.0f };
        headCenter = { neck.x - dir * 3.0f, neck.y + 9.0f };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * (6.0f + pose.hitRecoilX * 0.2f),  drawY + 20.0f };
        ankleF = { cx + dir * 10.0f, drawY + 3.0f };
        toeF   = { cx + dir * 16.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * (14.0f - pose.hitRecoilX * 0.2f), drawY + 18.0f };
        ankleB = { cx - dir * 18.0f, drawY + 3.0f };
        toeB   = { cx - dir * 12.0f, drawY + 2.0f };

        shoulderF = { chest.x + dir * 2.0f, chest.y };
        elbowF    = { cx + dir * (2.0f + pose.hitRecoilX * 0.3f),  drawY + 66.0f };
        handF     = { cx + dir * (8.0f + pose.hitRecoilX * 0.4f),  drawY + 64.0f };

        shoulderB = { chest.x - dir * 3.0f, chest.y };
        elbowB    = { cx - dir * (18.0f - pose.hitRecoilX * 0.4f), drawY + 62.0f };
        handB     = { cx - dir * (24.0f - pose.hitRecoilX * 0.5f), drawY + 54.0f };

    } else if (state == FighterState::PUNCH) {
        // Dynamic continuous punch animation (windup -> snap -> recovery)
        hips       = { cx + dir * (2.0f + pose.torsoLeanX * 0.3f), drawY + 46.0f };
        waist      = { cx + dir * (5.0f + pose.torsoLeanX * 0.6f), drawY + 54.0f };
        chest      = { cx + dir * (8.0f + pose.torsoLeanX),        drawY + 72.0f };
        neck       = { chest.x + dir * 2.0f,                       chest.y + 8.0f };
        headCenter = { neck.x + dir * 2.0f,                        neck.y + 10.0f };

        // Front lunge legs
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

        float maxReachHandX = (facingRight ? (drawX + drawW) : drawX) + dir * Attacks::PUNCH.reach * 0.95f;
        float punchY        = drawY + Attacks::PUNCH.hitboxOffsetY + Attacks::PUNCH.hitboxHeight * 0.5f;
        float readyHandX    = cx + dir * 18.0f;
        float coiledHandX   = cx + dir * 6.0f;

        if (phase == AttackPhase::STARTUP) {
            float t = pose.actionProgress;
            handF  = { readyHandX * (1.0f - t) + coiledHandX * t, (drawY + 68.0f) * (1.0f - t) + punchY * t };
            elbowF = { cx + dir * (14.0f * (1.0f - t) + 8.0f * t), drawY + (58.0f * (1.0f - t) + 64.0f * t) };
        } else if (phase == AttackPhase::ACTIVE) {
            float t = pose.actionProgress;
            float snap = t * (2.0f - t); // snappy explosive thrust
            handF  = { coiledHandX * (1.0f - snap) + maxReachHandX * snap, punchY };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, punchY - 2.0f * (1.0f - snap) };
        } else { // RECOVERY
            float t = pose.actionProgress;
            handF  = { maxReachHandX * (1.0f - t) + readyHandX * t, punchY * (1.0f - t) + (drawY + 68.0f) * t };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, (punchY - 2.0f) * (1.0f - t) + (drawY + 58.0f) * t };
        }

        // Back arm tightly chambered
        elbowB = { cx - dir * 5.0f, drawY + 60.0f };
        handB  = { cx + dir * 2.0f, drawY + 62.0f };

    } else if (state == FighterState::KICK) {
        // Dynamic continuous kick animation (chamber -> snap -> whip-back)
        hips       = { cx + dir * (pose.torsoLeanX * 0.2f), drawY + 46.0f };
        waist      = { cx + dir * (pose.torsoLeanX * 0.5f), drawY + 54.0f };
        chest      = { cx + dir * (pose.torsoLeanX),        drawY + 70.0f };
        neck       = { chest.x - dir * 2.0f,                chest.y + 8.0f };
        headCenter = { neck.x - dir * 2.0f,                 neck.y + 9.0f };

        // Supporting leg firmly rooted
        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 6.0f,  drawY + 22.0f };
        ankleB = { cx - dir * 8.0f,  drawY + 3.0f };
        toeB   = { cx - dir * 3.0f,  drawY + 2.0f };

        // Kicking leg
        hipF = { hips.x + dir * 4.0f, hips.y };
        float maxReachFootX = (facingRight ? (drawX + drawW) : drawX) + dir * Attacks::KICK.reach * 0.95f;
        float kickY         = drawY + Attacks::KICK.hitboxOffsetY + Attacks::KICK.hitboxHeight * 0.5f;
        float readyFootX    = cx + dir * 15.0f;
        float chamberFootX  = cx + dir * 8.0f;
        float chamberFootY  = drawY + 36.0f;
        float chamberKneeX  = cx + dir * 14.0f;
        float chamberKneeY  = drawY + 46.0f;

        if (phase == AttackPhase::STARTUP) {
            float t = pose.actionProgress;
            float ease = t * t;
            ankleF = { readyFootX * (1.0f - ease) + chamberFootX * ease, (drawY + 3.0f) * (1.0f - ease) + chamberFootY * ease };
            kneeF  = { (cx + dir * 13.0f) * (1.0f - ease) + chamberKneeX * ease, (drawY + 24.0f) * (1.0f - ease) + chamberKneeY * ease };
            toeF   = { ankleF.x + dir * 5.0f, ankleF.y + 2.0f * ease };
        } else if (phase == AttackPhase::ACTIVE) {
            float t = pose.actionProgress;
            float snap = t * (2.0f - t); // explosive snap
            ankleF = { chamberFootX * (1.0f - snap) + (maxReachFootX - dir * 4.0f) * snap, chamberFootY * (1.0f - snap) + kickY * snap };
            kneeF  = { (hipF.x + ankleF.x) * 0.50f, chamberKneeY * (1.0f - snap) + (kickY + 6.0f) * snap };
            toeF   = { ankleF.x + dir * 4.0f, ankleF.y };
        } else { // RECOVERY
            float t = pose.actionProgress;
            ankleF = { (maxReachFootX - dir * 4.0f) * (1.0f - t) + readyFootX * t, kickY * (1.0f - t) + (drawY + 3.0f) * t };
            kneeF  = { (hipF.x + ankleF.x) * 0.50f + dir * 3.0f, (drawY + 30.0f) * (1.0f - t) + (drawY + 24.0f) * t };
            toeF   = { ankleF.x + dir * 5.0f, ankleF.y - 1.0f };
        }

        shoulderF = { chest.x + dir * 3.0f, chest.y };
        elbowF    = { cx + dir * 4.0f,  drawY + 66.0f };
        handF     = { cx + dir * 10.0f, drawY + 68.0f };

        shoulderB = { chest.x - dir * 4.0f, chest.y };
        elbowB    = { cx - dir * 16.0f, drawY + 58.0f };
        handB     = { cx - dir * 20.0f, drawY + 52.0f };

    } else if (state == FighterState::SPECIAL) {
        // Continuous powerhouse special strike
        hips       = { cx + dir * (3.0f + pose.torsoLeanX * 0.25f), drawY + 46.0f };
        waist      = { cx + dir * (6.0f + pose.torsoLeanX * 0.5f),  drawY + 54.0f };
        chest      = { cx + dir * (10.0f + pose.torsoLeanX),        drawY + 72.0f };
        neck       = { chest.x + dir * 2.0f,                        chest.y + 8.0f };
        headCenter = { neck.x + dir * 2.0f,                         neck.y + 10.0f };

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

        float maxReachHandX = (facingRight ? (drawX + drawW) : drawX) + dir * Attacks::SPECIAL.reach * 0.95f;
        float specY         = drawY + Attacks::SPECIAL.hitboxOffsetY + Attacks::SPECIAL.hitboxHeight * 0.5f;

        if (phase == AttackPhase::STARTUP) {
            float t = pose.actionProgress;
            elbowF = { cx + dir * (14.0f * (1.0f - t) + 4.0f * t), drawY + 58.0f * (1.0f - t) + 62.0f * t };
            handF  = { cx + dir * (18.0f * (1.0f - t) + 8.0f * t), drawY + 68.0f * (1.0f - t) + 64.0f * t };
        } else if (phase == AttackPhase::ACTIVE) {
            float t = pose.actionProgress;
            float snap = t * (2.0f - t);
            handF  = { (cx + dir * 8.0f) * (1.0f - snap) + maxReachHandX * snap, specY };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, specY - 2.0f };
        } else { // RECOVERY
            float t = pose.actionProgress;
            handF  = { maxReachHandX * (1.0f - t) + (cx + dir * 18.0f) * t, specY * (1.0f - t) + (drawY + 68.0f) * t };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, specY * (1.0f - t) + (drawY + 58.0f) * t };
        }

        elbowB = { cx - dir * 8.0f,  drawY + 58.0f };
        handB  = { cx - dir * 12.0f, drawY + 54.0f };

    } else {
        // IDLE: Poised combat stance with breathing rhythm
        hips       = { cx,              drawY + 46.0f + pose.breathY * 0.3f };
        waist      = { cx + dir * 1.0f, drawY + 54.0f + pose.breathY * 0.6f };
        chest      = { cx + dir * 3.0f, drawY + 72.0f + pose.breathY };
        neck       = { cx + dir * 4.0f, drawY + 80.0f + pose.breathY * 0.9f };
        headCenter = { cx + dir * 5.0f, drawY + 90.0f + pose.breathY * 0.9f };

        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * 13.0f, drawY + 24.0f };
        ankleF = { cx + dir * 15.0f, drawY + 3.0f };
        toeF   = { cx + dir * 21.0f, drawY + 2.0f };

        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * 8.0f,  drawY + 24.0f };
        ankleB = { cx - dir * 14.0f, drawY + 3.0f };
        toeB   = { cx - dir * 8.0f,  drawY + 2.0f };

        shoulderF = { chest.x + dir * 4.0f, chest.y };
        elbowF    = { cx + dir * 14.0f, drawY + 58.0f + pose.breathY * 0.8f };
        handF     = { cx + dir * 18.0f, drawY + 68.0f + pose.breathY * 1.2f };

        shoulderB = { chest.x - dir * 3.0f, chest.y };
        elbowB    = { cx - dir * 4.0f,  drawY + 56.0f + pose.breathY * 0.6f };
        handB     = { cx + dir * 6.0f,  drawY + 64.0f + pose.breathY * 0.9f };
    }

    // =========================================================================
    // LAYER 1: BACK LIMBS & BACK ATTIRE
    // =========================================================================
    // Back Leg
    renderer.drawLimb(hipB.x, hipB.y, kneeB.x, kneeB.y, 5.5f, 4.5f, backR, backG, backB);
    renderer.drawLimb(kneeB.x, kneeB.y, ankleB.x, ankleB.y, 4.5f, 3.5f, backR, backG, backB);
    renderer.drawLimb(ankleB.x, ankleB.y, toeB.x, toeB.y, 3.5f, 3.0f, backR, backG, backB);

    // Back Arm
    renderer.drawLimb(shoulderB.x, shoulderB.y, elbowB.x, elbowB.y, 4.2f, 3.5f, backR, backG, backB);
    renderer.drawLimb(elbowB.x, elbowB.y, handB.x, handB.y, 3.5f, 3.0f, backR, backG, backB);
    renderer.drawCircle(handB.x, handB.y, 3.5f, backR, backG, backB);

    // Character-specific back elements
    if (role == FighterRole::NINJA) {
        // Ninja: Twin daggers / ninja blades strapped across the back (as in reference image)
        if (state != FighterState::DEAD) {
            // Scabbard 1 & Hilt 1
            renderer.drawLine(shoulderB.x - dir * 2.0f, shoulderB.y + 2.0f,
                              shoulderB.x - dir * 14.0f, shoulderB.y + 20.0f,
                              0.15f, 0.15f, 0.18f, 1.0f, 3.2f);
            renderer.drawLine(shoulderB.x - dir * 12.0f, shoulderB.y + 18.0f,
                              shoulderB.x - dir * 16.0f, shoulderB.y + 24.0f,
                              accentR, accentG, accentB, 1.0f, 2.8f);
            // Scabbard 2 & Hilt 2 (slightly offset)
            renderer.drawLine(shoulderB.x - dir * 5.0f, shoulderB.y + 4.0f,
                              shoulderB.x - dir * 18.0f, shoulderB.y + 25.0f,
                              0.12f, 0.12f, 0.15f, 1.0f, 3.0f);
            renderer.drawLine(shoulderB.x - dir * 16.0f, shoulderB.y + 22.0f,
                              shoulderB.x - dir * 20.0f, shoulderB.y + 28.0f,
                              accentR * 0.8f, accentG * 0.8f, accentB * 0.8f, 1.0f, 2.5f);
        }
        // Off-hand reverse-grip defensive dagger
        float ddirX = -dir * 10.0f;
        float ddirY = -5.0f;
        renderer.drawLine(handB.x, handB.y, handB.x + ddirX, handB.y + ddirY,
                          0.82f, 0.88f, 0.95f, 1.0f, 2.0f);
    } else {
        // Spearman: Armored shoulder pauldron on back arm
        renderer.drawCircle(shoulderB.x, shoulderB.y, 5.5f, backR * 1.3f, backG * 1.3f, backB * 1.3f);
        renderer.drawQuad(shoulderB.x - dir * 4.0f, shoulderB.y + 4.0f,
                          shoulderB.x + dir * 4.0f, shoulderB.y + 4.0f,
                          shoulderB.x + dir * 5.0f, shoulderB.y - 3.0f,
                          shoulderB.x - dir * 5.0f, shoulderB.y - 3.0f,
                          backR * 1.15f, backG * 1.15f, backB * 1.15f);

        // Spearman: Billowing warrior coat-tails / tunic skirts trailing backwards (as in reference image!)
        float skirtWave = (state == FighterState::DEAD) ? 0.0f : std::sin(stateTime * 7.0f) * 3.5f;
        renderer.drawQuad(waist.x - dir * 3.0f, waist.y,
                          waist.x - dir * 7.0f, waist.y,
                          cx - dir * (26.0f + skirtWave), drawY + 8.0f,
                          cx - dir * (17.0f + skirtWave), drawY + 8.0f,
                          backR, backG, backB);
        renderer.drawQuad(waist.x - dir * 5.0f, waist.y - 2.0f,
                          waist.x - dir * 9.0f, waist.y - 2.0f,
                          cx - dir * (32.0f + skirtWave * 1.2f), drawY + 4.0f,
                          cx - dir * (24.0f + skirtWave * 1.2f), drawY + 4.0f,
                          backR * 0.85f, backG * 0.85f, backB * 0.85f);
    }

    // =========================================================================
    // LAYER 2: TORSO & BELT / ARMOR
    // =========================================================================
    // Upper chest & shoulders
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

    if (role == FighterRole::NINJA) {
        // Ninja: Belt sash (Obi) wrapped around waist
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
    } else {
        // Spearman: Segmented armor plates & heavy combat belt
        renderer.drawRect(waist.x - 8.0f, waist.y - 3.5f, 16.0f, 7.0f, accentR * 0.7f, accentG * 0.7f, accentB * 0.7f);
        renderer.drawRectOutline(waist.x - 8.0f, waist.y - 3.5f, 16.0f, 7.0f, 0.45f, 0.45f, 0.50f);
        // Front armored tunic flap
        float skirtWave = (state == FighterState::DEAD) ? 0.0f : std::sin(stateTime * 7.0f) * 2.5f;
        renderer.drawQuad(waist.x, waist.y,
                          waist.x - dir * 5.0f, waist.y,
                          cx - dir * (14.0f + skirtWave * 0.8f), drawY + 12.0f,
                          cx - dir * (8.0f + skirtWave * 0.8f), drawY + 12.0f,
                          bodyR, bodyG, bodyB);
    }

    // =========================================================================
    // LAYER 3: FRONT LIMBS & WEAPONS
    // =========================================================================
    // Front Leg
    renderer.drawLimb(hipF.x, hipF.y, kneeF.x, kneeF.y, 5.5f, 4.5f, bodyR, bodyG, bodyB);
    renderer.drawLimb(kneeF.x, kneeF.y, ankleF.x, ankleF.y, 4.5f, 3.5f, bodyR, bodyG, bodyB);
    renderer.drawLimb(ankleF.x, ankleF.y, toeF.x, toeF.y, 3.5f, 3.0f, bodyR, bodyG, bodyB);

    // Front Arm
    renderer.drawLimb(shoulderF.x, shoulderF.y, elbowF.x, elbowF.y, 4.2f, 3.5f, bodyR, bodyG, bodyB);
    renderer.drawLimb(elbowF.x, elbowF.y, handF.x, handF.y, 3.5f, 3.0f, bodyR, bodyG, bodyB);
    renderer.drawCircle(handF.x, handF.y, 3.5f, bodyR, bodyG, bodyB);

    // Character-specific front weapons & armor
    if (role == FighterRole::NINJA) {
        // Ninja: Combat Dagger / Sai in front hand
        float bladeAngle = 0.0f;
        float bladeLen   = 22.0f;
        if (state == FighterState::PUNCH || state == FighterState::SPECIAL) {
            bladeLen = 28.0f;
            bladeAngle = 0.0f;
        } else if (state == FighterState::BLOCK) {
            bladeAngle = 0.5f;
        } else {
            bladeAngle = 0.15f;
        }

        float tipX = handF.x + dir * (std::cos(bladeAngle) * bladeLen);
        float tipY = handF.y + std::sin(bladeAngle) * bladeLen;

        // Dagger guard
        renderer.drawLine(handF.x - dir * 2.0f, handF.y - 4.0f,
                          handF.x + dir * 2.0f, handF.y + 4.0f,
                          accentR, accentG, accentB, 1.0f, 2.5f);
        // Razor blade with bright steel edge
        renderer.drawLine(handF.x, handF.y, tipX, tipY,
                          0.88f, 0.92f, 0.98f, 1.0f, 2.5f);
        renderer.drawLine(handF.x, handF.y, tipX, tipY,
                          0.25f, 0.30f, 0.35f, 1.0f, 1.2f);
    } else {
        // Spearman: Front shoulder pauldron
        renderer.drawCircle(shoulderF.x, shoulderF.y, 6.0f, bodyR * 1.3f, bodyG * 1.3f, bodyB * 1.3f);
        renderer.drawQuad(shoulderF.x - dir * 4.0f, shoulderF.y + 5.0f,
                          shoulderF.x + dir * 5.0f, shoulderF.y + 5.0f,
                          shoulderF.x + dir * 6.0f, shoulderF.y - 3.0f,
                          shoulderF.x - dir * 5.0f, shoulderF.y - 3.0f,
                          bodyR * 1.15f, bodyG * 1.15f, bodyB * 1.15f);

        // Spearman: Long Polearm / Spear (Yari / Naginata)
        float spearTailX, spearTailY;
        float spearHeadX, spearHeadY;

        if (state == FighterState::PUNCH || state == FighterState::SPECIAL) {
            // Forward spear thrust strike!
            spearTailX = handB.x - dir * 28.0f;
            spearTailY = handB.y - 4.0f;
            spearHeadX = handF.x + dir * 44.0f;
            spearHeadY = handF.y + 1.0f;
        } else if (state == FighterState::BLOCK) {
            // Vertical spear guard
            spearTailX = handF.x - dir * 6.0f;
            spearTailY = drawY + 2.0f;
            spearHeadX = handF.x + dir * 4.0f;
            spearHeadY = drawY + 98.0f;
        } else {
            // Iconic stance from reference image: spear angles down and far behind him!
            spearTailX = handF.x - dir * 62.0f;
            spearTailY = drawY + 6.0f;
            spearHeadX = handF.x + dir * 24.0f;
            spearHeadY = handF.y + 14.0f;
        }

        // Wooden/steel spear shaft
        renderer.drawLine(spearTailX, spearTailY, spearHeadX, spearHeadY,
                          0.25f, 0.20f, 0.16f, 1.0f, 3.2f);
        renderer.drawLine(spearTailX, spearTailY, spearHeadX, spearHeadY,
                          0.12f, 0.10f, 0.08f, 1.0f, 1.5f);

        // Spearhead leaf blade & battle tassel
        float tipDirX = (spearHeadX - spearTailX);
        float tipDirY = (spearHeadY - spearTailY);
        float len = std::sqrt(tipDirX * tipDirX + tipDirY * tipDirY);
        if (len > 0.001f) {
            tipDirX /= len;
            tipDirY /= len;
        }
        float bladeTipX = spearHeadX + tipDirX * 18.0f;
        float bladeTipY = spearHeadY + tipDirY * 18.0f;
        float bladeNormX = -tipDirY * 3.5f;
        float bladeNormY =  tipDirX * 3.5f;

        // Razor steel spearhead
        renderer.drawQuad(spearHeadX, spearHeadY,
                          spearHeadX + tipDirX * 7.0f + bladeNormX, spearHeadY + tipDirY * 7.0f + bladeNormY,
                          bladeTipX, bladeTipY,
                          spearHeadX + tipDirX * 7.0f - bladeNormX, spearHeadY + tipDirY * 7.0f - bladeNormY,
                          0.88f, 0.92f, 0.98f);
        // Crimson battle tassel fluttering behind blade socket
        float tasselWave = std::sin(stateTime * 9.0f) * 2.5f;
        renderer.drawQuad(spearHeadX - tipDirX * 2.0f, spearHeadY - tipDirY * 2.0f,
                          spearHeadX, spearHeadY,
                          spearHeadX - dir * 8.0f + tasselWave, spearHeadY - 8.0f,
                          spearHeadX - dir * 12.0f + tasselWave, spearHeadY - 6.0f,
                          0.95f, 0.20f, 0.15f);
    }

    // =========================================================================
    // LAYER 4: HEAD, HELMET / HOOD & GLOWING EYES
    // =========================================================================
    if (role == FighterRole::NINJA) {
        // Ninja: Cowl / hood head shape
        renderer.drawCircle(headCenter.x, headCenter.y, headRadius, bodyR, bodyG, bodyB);
        // Tapered chin & cloth mask
        renderer.drawTriangle(headCenter.x - 6.5f, headCenter.y - 2.0f,
                              headCenter.x + 6.5f, headCenter.y - 2.0f,
                              headCenter.x + dir * 2.0f, headCenter.y - 10.0f,
                              bodyR, bodyG, bodyB);
        // Cowl peak on top
        renderer.drawTriangle(headCenter.x - 5.0f, headCenter.y + 6.0f,
                              headCenter.x + 4.0f, headCenter.y + 6.0f,
                              headCenter.x - dir * 1.5f, headCenter.y + 11.5f,
                              bodyR, bodyG, bodyB);

        if (state != FighterState::DEAD) {
            // Headband wrapped around forehead
            renderer.drawRect(headCenter.x - 8.5f, headCenter.y + 0.5f, 17.0f, 3.5f, accentR, accentG, accentB);

            // Wind-blown headband ribbons trailing behind head with dynamic sway
            float tailWave1 = std::sin(stateTime * 10.0f) * 2.5f;
            float tailWave2 = std::cos(stateTime * 8.0f) * 3.0f;

            renderer.drawQuad(headCenter.x - dir * 7.5f, headCenter.y + 3.0f,
                              headCenter.x - dir * 7.5f, headCenter.y + 1.0f,
                              headCenter.x - dir * 18.0f, headCenter.y - 2.0f + tailWave1,
                              headCenter.x - dir * 18.0f, headCenter.y + 0.5f + tailWave1,
                              accentR, accentG, accentB);
            renderer.drawQuad(headCenter.x - dir * 7.5f, headCenter.y + 1.5f,
                              headCenter.x - dir * 7.5f, headCenter.y - 0.5f,
                              headCenter.x - dir * 22.0f, headCenter.y - 7.0f + tailWave2,
                              headCenter.x - dir * 22.0f, headCenter.y - 5.0f + tailWave2,
                              accentR * 0.85f, accentG * 0.85f, accentB * 0.85f);

            // Razor-sharp glowing cyan eye slit
            if (drawEye) {
                float eyeX = headCenter.x + dir * 4.0f;
                float eyeY = headCenter.y + 0.2f;
                renderer.drawLine(eyeX - dir * 2.0f, eyeY - 0.8f,
                                  eyeX + dir * 3.0f, eyeY + 0.8f,
                                  accentR, accentG, accentB, 1.0f, 2.0f);
                renderer.drawCircle(eyeX + dir * 1.0f, eyeY + 0.3f, 1.2f, 1.0f, 1.0f, 1.0f, 0.95f, 8);
            }
        }
    } else {
        // Spearman: Distinctive pointed war helmet (matching user's reference image!)
        // Main helmet skull cap
        renderer.drawCircle(headCenter.x, headCenter.y, headRadius, bodyR, bodyG, bodyB);

        // Pointed conical helmet crest apex rising from top
        renderer.drawTriangle(headCenter.x - dir * 7.0f, headCenter.y + 5.0f,
                              headCenter.x + dir * 6.0f, headCenter.y + 5.0f,
                              headCenter.x - dir * 1.5f, headCenter.y + 16.5f,
                              bodyR * 1.15f, bodyG * 1.15f, bodyB * 1.15f);

        // Angled brow visor protruding forward over eyes
        renderer.drawQuad(headCenter.x - dir * 2.0f, headCenter.y + 4.0f,
                          headCenter.x + dir * 10.0f, headCenter.y + 1.5f,
                          headCenter.x + dir * 8.5f,  headCenter.y - 1.2f,
                          headCenter.x - dir * 1.0f, headCenter.y + 1.0f,
                          bodyR * 1.3f, bodyG * 1.3f, bodyB * 1.3f);

        // Flared neck guard (aventail) sloping down behind neck
        renderer.drawQuad(headCenter.x - dir * 2.0f, headCenter.y + 2.0f,
                          headCenter.x - dir * 10.5f, headCenter.y - 6.0f,
                          headCenter.x - dir * 8.5f,  headCenter.y - 8.5f,
                          headCenter.x, headCenter.y - 3.0f,
                          bodyR * 1.1f, bodyG * 1.1f, bodyB * 1.1f);

        // Tapered chin guard
        renderer.drawTriangle(headCenter.x - 6.0f, headCenter.y - 2.0f,
                              headCenter.x + 6.0f, headCenter.y - 2.0f,
                              headCenter.x + dir * 1.5f, headCenter.y - 9.0f,
                              bodyR, bodyG, bodyB);

        // Piercing amber eye slit underneath visor
        if (drawEye) {
            float eyeX = headCenter.x + dir * 3.5f;
            float eyeY = headCenter.y - 0.2f;
            renderer.drawLine(eyeX - dir * 2.0f, eyeY - 0.5f,
                              eyeX + dir * 3.0f, eyeY + 0.5f,
                              accentR, accentG, accentB, 1.0f, 2.2f);
            renderer.drawCircle(eyeX + dir * 1.0f, eyeY + 0.1f, 1.3f, 1.0f, 0.95f, 0.5f, 0.95f, 8);
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
        // Fiery dragon energy aura gathering on striking weapon
        float strikeX = (role == FighterRole::NINJA) ? handF.x + dir * 18.0f : handF.x + dir * 35.0f;
        float strikeY = handF.y;
        renderer.drawCircle(strikeX, strikeY, 18.0f, 1.0f, 0.65f, 0.15f, 0.40f, 18);
        renderer.drawCircle(strikeX, strikeY, 10.0f, 1.0f, 0.90f, 0.35f, 0.80f, 14);
        renderer.drawCircle(strikeX, strikeY,  4.5f, 1.0f, 1.0f,  1.0f,  0.95f, 10);
    }
}
