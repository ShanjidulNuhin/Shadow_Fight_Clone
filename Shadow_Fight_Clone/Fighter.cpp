#include "Fighter.h"
#include "Renderer.h"
#include "Animation.h"
#include "Config.h"
#include <cmath>
#include <algorithm>

namespace {
    const float CROUCH_HEIGHT_RATIO = 0.62f;
    const float CROUCH_SPEED_RATIO  = 0.45f;

    const int   MAX_HEALTH = 100;

    const float MAX_ENERGY   = 100.0f;
    const float ENERGY_REGEN = 10.0f;

    // Getting hit
    const float KNOCKBACK_DECEL = 1600.0f;

    // Blocking
    const int   BLOCK_DAMAGE_PERCENT  = 20;
    const float BLOCK_KNOCKBACK_SPEED = 160.0f;
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
      moveSpeed(260.0f), velocityY(0.0f), knockbackVelocityX(0.0f),
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

    if (attackState == FighterState::SPECIAL) {
        if (energy < data->energyCost) return;
        energy -= data->energyCost;
    }

    state = attackState;
    stateTime = 0.0f;
    attackHasHit = false;
    crouching = false;
    blocking = false;
    height = standingHeight;
}

void Fighter::startPunch() {
    if (!canAct() || !onGround) return;
    startAttack(FighterState::PUNCH);
}

void Fighter::startKick() {
    if (!canAct() || !onGround) return;
    startAttack(FighterState::KICK);
}

void Fighter::startSpecial() {
    if (!canAct() || !onGround) return;
    startAttack(FighterState::SPECIAL);
}

void Fighter::applyPhysics(float deltaTime) {
    if (!onGround) {
        velocityY += Config::GRAVITY * deltaTime;
        y += velocityY * deltaTime;

        if (y <= Config::GROUND_HEIGHT) {
            y = Config::GROUND_HEIGHT;
            velocityY = 0.0f;
            onGround = true;
        }
    }

    if (knockbackVelocityX != 0.0f) {
        x += knockbackVelocityX * deltaTime;
        keepInsideWorld();

        float sign = (knockbackVelocityX > 0.0f) ? 1.0f : -1.0f;
        float reduction = KNOCKBACK_DECEL * deltaTime;
        if (std::fabs(knockbackVelocityX) <= reduction) {
            knockbackVelocityX = 0.0f;
        } else {
            knockbackVelocityX -= sign * reduction;
        }
    }
}

void Fighter::updateState(float deltaTime) {
    stateTime += deltaTime;

    if (blockFlashTimer > 0.0f) blockFlashTimer -= deltaTime;
    if (invulnerableTimer > 0.0f) invulnerableTimer -= deltaTime;

    energy += ENERGY_REGEN * deltaTime;
    if (energy > maxEnergy) energy = maxEnergy;

    if (state == FighterState::DEAD) return;

    if (state == FighterState::HIT) {
        if (stateTime >= hitStunDuration) {
            state = FighterState::IDLE;
            stateTime = 0.0f;
            invulnerableTimer = RECOVERY_INVULNERABILITY;
        }
        return;
    }

    if (isAttacking()) {
        const AttackData* attack = getCurrentAttack();
        if (attack && stateTime >= attack->totalTime()) {
            state = FighterState::IDLE;
            stateTime = 0.0f;
        }
        return;
    }

    if (!onGround) {
        state = (velocityY >= 0.0f) ? FighterState::JUMP : FighterState::FALL;
        return;
    }

    if (blocking) {
        state = FighterState::BLOCK;
        return;
    }

    if (crouching) {
        state = FighterState::CROUCH;
        return;
    }

    if (std::fabs(moveDirection) > 0.001f) {
        state = FighterState::WALK;
        return;
    }

    state = FighterState::IDLE;
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
    if (health <= 0) return;
    health -= amount;
    if (health <= 0) {
        health = 0;
        state = FighterState::DEAD;
        stateTime = 0.0f;
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
    return state != FighterState::HIT &&
           state != FighterState::DEAD &&
           !isAttacking();
}

bool Fighter::isAttacking() const {
    return state == FighterState::PUNCH ||
           state == FighterState::KICK  ||
           state == FighterState::SPECIAL;
}

AttackPhase Fighter::getAttackPhase() const {
    const AttackData* attack = getCurrentAttack();
    if (attack == nullptr) return AttackPhase::NONE;

    if (stateTime < attack->startup) return AttackPhase::STARTUP;
    if (stateTime < attack->startup + attack->active) return AttackPhase::ACTIVE;
    return AttackPhase::RECOVERY;
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

// =============================================================================
// REALISTIC HUMAN FIGHTER DRAWING (LARGE ATHLETIC ANATOMY - COMPLETELY UNARMED)
// =============================================================================
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

    float headRadius = 11.5f;
    bool  drawEye    = (state != FighterState::DEAD);

    // =========================================================================
    // DYNAMIC HUMAN JOINT SOLVER (PROPORTIONED FOR 145px HEIGHT, 68px WIDTH)
    // =========================================================================
    if (state == FighterState::DEAD) {
        // Smooth dynamic ragdoll collapse: interpolates standing to flat on ground
        float col = pose.collapse;
        float invCol = 1.0f - col;

        headCenter = { cx + dir * (8.0f * invCol + 46.0f * col), drawY + 133.0f * invCol + 10.0f * col };
        neck       = { cx + dir * (6.5f * invCol + 35.0f * col), drawY + 120.0f * invCol + 8.0f * col };
        chest      = { cx + dir * (5.0f * invCol + 20.0f * col), drawY + 108.0f * invCol + 8.0f * col };
        waist      = { cx + dir * (2.0f * invCol + 2.0f * col),  drawY + 82.0f * invCol + 7.0f * col };
        hips       = { cx - dir * (14.0f * col),                 drawY + 68.0f * invCol + 7.0f * col };

        // Front leg collapses
        hipF   = { hips.x + dir * 4.0f, hips.y };
        kneeF  = { cx + dir * (20.0f * invCol - 32.0f * col), drawY + 36.0f * invCol + 6.0f * col };
        ankleF = { cx + dir * (22.0f * invCol - 50.0f * col), drawY + 5.0f };
        toeF   = { ankleF.x + dir * (10.0f * invCol - 8.0f * col), ankleF.y - 1.0f };

        // Back leg collapses
        hipB   = { hips.x - dir * 4.0f, hips.y };
        kneeB  = { cx - dir * (12.0f * invCol + 26.0f * col), drawY + 36.0f * invCol + 8.0f * col };
        ankleB = { cx - dir * (20.0f * invCol + 44.0f * col), drawY + 5.0f * invCol + 7.0f * col };
        toeB   = { ankleB.x - dir * 8.0f, ankleB.y - 1.0f };

        // Arms drop limp to the floor
        shoulderF = { chest.x + dir * 4.0f, chest.y };
        elbowF    = { cx + dir * (22.0f * invCol + 12.0f * col), drawY + 88.0f * invCol + 5.0f * col };
        handF     = { cx + dir * (28.0f * invCol + 2.0f * col),  drawY + 104.0f * invCol + 3.0f * col };

        shoulderB = { chest.x - dir * 4.0f, chest.y };
        elbowB    = { cx + dir * (-5.0f * invCol + 26.0f * col), drawY + 84.0f * invCol + 9.0f * col };
        handB     = { cx + dir * (10.0f * invCol + 38.0f * col), drawY + 96.0f * invCol + 9.0f * col };

        if (col >= 0.85f) drawEye = false;

    } else if (state == FighterState::WALK) {
        // Continuous martial-arts stride cycle with swinging limbs & counter-sway
        hips       = { cx + dir * pose.torsoLeanX * 0.5f, drawY + 68.0f + pose.offsetY };
        waist      = { cx + dir * pose.torsoLeanX * 0.8f, drawY + 82.0f + pose.offsetY };
        chest      = { cx + dir * (4.0f + pose.torsoLeanX * 1.2f), drawY + 108.0f + pose.offsetY };
        neck       = { chest.x + dir * 2.5f, chest.y + 12.0f };
        headCenter = { neck.x + dir * 2.0f, neck.y + 13.0f };

        // Front leg stride & lift
        hipF   = { hips.x + dir * 6.0f, hips.y };
        ankleF = { cx + dir * (22.0f + pose.strideLegF * 1.4f), drawY + 5.0f + pose.liftLegF * 1.4f };
        toeF   = { ankleF.x + dir * 10.0f, ankleF.y - 1.5f };
        kneeF  = { (hipF.x + ankleF.x) * 0.5f + dir * 6.0f, (hipF.y + ankleF.y) * 0.5f + pose.liftLegF * 0.8f };

        // Back leg stride & lift (opposite phase)
        hipB   = { hips.x - dir * 6.0f, hips.y };
        ankleB = { cx + dir * (-20.0f + pose.strideLegB * 1.4f), drawY + 5.0f + pose.liftLegB * 1.4f };
        toeB   = { ankleB.x + dir * 8.0f, ankleB.y - 1.5f };
        kneeB  = { (hipB.x + ankleB.x) * 0.5f + dir * 4.0f, (hipB.y + ankleB.y) * 0.5f + pose.liftLegB * 0.8f };

        // Arms swinging in rhythm with strides
        shoulderF = { chest.x + dir * 6.0f, chest.y };
        elbowF    = { cx + dir * (22.0f + pose.armSwingF * 0.9f), drawY + 88.0f + pose.offsetY };
        handF     = { cx + dir * (28.0f + pose.armSwingF * 1.5f), drawY + 104.0f + pose.offsetY };

        shoulderB = { chest.x - dir * 5.0f, chest.y };
        elbowB    = { cx + dir * (-5.0f + pose.armSwingB * 0.9f), drawY + 84.0f + pose.offsetY };
        handB     = { cx + dir * (10.0f + pose.armSwingB * 1.5f), drawY + 96.0f + pose.offsetY };

    } else if (state == FighterState::CROUCH) {
        // Deep crouching martial-arts tiger guard
        hips       = { cx - dir * 6.0f,  drawY + 34.0f };
        waist      = { cx,               drawY + 46.0f };
        chest      = { cx + dir * 12.0f, drawY + 62.0f };
        neck       = { cx + dir * 18.0f, drawY + 72.0f };
        headCenter = { cx + dir * 24.0f, drawY + 84.0f };

        hipF   = { hips.x + dir * 6.0f, hips.y };
        kneeF  = { cx + dir * 28.0f, drawY + 22.0f };
        ankleF = { cx + dir * 22.0f, drawY + 5.0f };
        toeF   = { cx + dir * 32.0f, drawY + 3.0f };

        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * 16.0f, drawY + 18.0f };
        ankleB = { cx - dir * 24.0f, drawY + 5.0f };
        toeB   = { cx - dir * 16.0f, drawY + 3.0f };

        shoulderF = { chest.x + dir * 4.0f, chest.y };
        elbowF    = { cx + dir * 28.0f, drawY + 44.0f };
        handF     = { cx + dir * 34.0f, drawY + 54.0f };

        shoulderB = { chest.x - dir * 4.0f, chest.y };
        elbowB    = { cx + dir * 10.0f, drawY + 38.0f };
        handB     = { cx + dir * 18.0f, drawY + 44.0f };

    } else if (state == FighterState::JUMP || state == FighterState::FALL) {
        // Airborne martial-arts tuck
        hips       = { cx,               drawY + 66.0f };
        waist      = { cx + dir * 3.0f,  drawY + 78.0f };
        chest      = { cx + dir * 8.0f,  drawY + 102.0f };
        neck       = { cx + dir * 11.0f, drawY + 114.0f };
        headCenter = { cx + dir * 14.0f, drawY + 128.0f };

        hipF   = { hips.x + dir * 6.0f, hips.y };
        kneeF  = { cx + dir * 22.0f, drawY + 42.0f };
        ankleF = { cx + dir * 12.0f, drawY + 22.0f };
        toeF   = { cx + dir * 20.0f, drawY + 16.0f };

        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * 16.0f, drawY + 34.0f };
        ankleB = { cx - dir * 24.0f, drawY + 14.0f };
        toeB   = { cx - dir * 18.0f, drawY + 8.0f };

        shoulderF = { chest.x + dir * 5.0f, chest.y };
        elbowF    = { cx + dir * 24.0f, drawY + 108.0f };
        handF     = { cx + dir * 34.0f, drawY + 118.0f };

        shoulderB = { chest.x - dir * 4.0f, chest.y };
        elbowB    = { cx - dir * 16.0f, drawY + 88.0f };
        handB     = { cx - dir * 24.0f, drawY + 76.0f };

    } else if (state == FighterState::BLOCK) {
        // High double-forearm iron guard protecting head & torso
        float b = pose.blockBrace;
        float invB = 1.0f - b;

        hips       = { cx - dir * (4.0f * b),               drawY + 68.0f - 3.0f * b };
        waist      = { cx + dir * (2.0f * invB - 3.0f * b),  drawY + 82.0f - 3.0f * b };
        chest      = { cx + dir * (5.0f * invB),            drawY + 108.0f - 3.0f * b };
        neck       = { cx + dir * (6.5f * invB + 2.0f * b),  drawY + 120.0f - 4.0f * b };
        headCenter = { cx + dir * (8.0f * invB + 3.0f * b),  drawY + 133.0f - 4.0f * b };

        hipF   = { hips.x + dir * 6.0f, hips.y };
        kneeF  = { cx + dir * (20.0f * invB + 18.0f * b), drawY + 36.0f - 3.0f * b };
        ankleF = { cx + dir * (22.0f * invB + 20.0f * b), drawY + 5.0f };
        toeF   = { cx + dir * 30.0f, drawY + 3.0f };

        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * (12.0f * invB + 15.0f * b), drawY + 36.0f - 3.0f * b };
        ankleB = { cx - dir * (20.0f * invB + 24.0f * b), drawY + 5.0f };
        toeB   = { cx - dir * 14.0f, drawY + 3.0f };

        // Raised crossed fists into high guard
        shoulderF = { chest.x + dir * 5.0f, chest.y };
        elbowF    = { cx + dir * (22.0f * invB + 20.0f * b), drawY + 88.0f * invB + 96.0f * b };
        handF     = { cx + dir * (28.0f * invB + 24.0f * b), drawY + 104.0f * invB + 120.0f * b };

        shoulderB = { chest.x - dir * 4.0f, chest.y };
        elbowB    = { cx + dir * (-5.0f * invB + 14.0f * b), drawY + 84.0f * invB + 92.0f * b };
        handB     = { cx + dir * (10.0f * invB + 18.0f * b), drawY + 96.0f * invB + 116.0f * b };

    } else if (state == FighterState::HIT) {
        // Shockwave impact recoil with damped harmonic recovery
        hips       = { cx + dir * (pose.hitRecoilX * 0.5f), drawY + 66.0f };
        waist      = { cx + dir * (pose.hitRecoilX * 0.9f), drawY + 80.0f };
        chest      = { cx + dir * (pose.hitRecoilX * 1.4f), drawY + 105.0f };
        neck       = { chest.x - dir * 4.0f, chest.y + 12.0f };
        headCenter = { neck.x - dir * 5.0f, neck.y + 13.0f };

        hipF   = { hips.x + dir * 6.0f, hips.y };
        kneeF  = { cx + dir * (10.0f + pose.hitRecoilX * 0.3f), drawY + 30.0f };
        ankleF = { cx + dir * 15.0f, drawY + 5.0f };
        toeF   = { cx + dir * 24.0f, drawY + 3.0f };

        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * (20.0f - pose.hitRecoilX * 0.3f), drawY + 28.0f };
        ankleB = { cx - dir * (26.0f - pose.hitRecoilX * 0.2f), drawY + 5.0f };
        toeB   = { cx - dir * 18.0f, drawY + 3.0f };

        shoulderF = { chest.x + dir * 4.0f, chest.y };
        elbowF    = { cx + dir * (4.0f + pose.hitRecoilX * 0.5f),  drawY + 98.0f };
        handF     = { cx + dir * (12.0f + pose.hitRecoilX * 0.6f), drawY + 96.0f };

        shoulderB = { chest.x - dir * 5.0f, chest.y };
        elbowB    = { cx - dir * (26.0f - pose.hitRecoilX * 0.6f), drawY + 92.0f };
        handB     = { cx - dir * (36.0f - pose.hitRecoilX * 0.8f), drawY + 80.0f };

    } else if (state == FighterState::PUNCH) {
        // Dynamic continuous straight punch / thrust
        hips       = { cx + dir * (4.0f + pose.torsoLeanX * 0.4f),  drawY + 68.0f };
        waist      = { cx + dir * (8.0f + pose.torsoLeanX * 0.8f),  drawY + 82.0f };
        chest      = { cx + dir * (14.0f + pose.torsoLeanX * 1.3f), drawY + 108.0f };
        neck       = { chest.x + dir * 3.5f,                        chest.y + 12.0f };
        headCenter = { neck.x + dir * 3.5f,                         neck.y + 13.0f };

        // Front lunge legs
        hipF   = { hips.x + dir * 6.0f, hips.y };
        kneeF  = { cx + dir * 26.0f, drawY + 36.0f };
        ankleF = { cx + dir * 28.0f, drawY + 5.0f };
        toeF   = { cx + dir * 38.0f, drawY + 3.0f };

        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * 16.0f, drawY + 32.0f };
        ankleB = { cx - dir * 26.0f, drawY + 5.0f };
        toeB   = { cx - dir * 16.0f, drawY + 3.0f };

        shoulderF = { chest.x + dir * 6.0f, chest.y };
        shoulderB = { chest.x - dir * 5.0f, chest.y };

        float maxReachHandX = (facingRight ? (drawX + drawW) : drawX) + dir * Attacks::PUNCH.reach * 0.98f;
        float punchY        = drawY + Attacks::PUNCH.hitboxOffsetY + Attacks::PUNCH.hitboxHeight * 0.5f;
        float readyHandX    = cx + dir * 28.0f;
        float coiledHandX   = cx + dir * 8.0f;

        if (phase == AttackPhase::STARTUP) {
            float t = pose.actionProgress;
            handF  = { readyHandX * (1.0f - t) + coiledHandX * t, (drawY + 104.0f) * (1.0f - t) + punchY * t };
            elbowF = { cx + dir * (22.0f * (1.0f - t) + 12.0f * t), drawY + (88.0f * (1.0f - t) + 98.0f * t) };
        } else if (phase == AttackPhase::ACTIVE) {
            float t = pose.actionProgress;
            float snap = t * (2.0f - t); // explosive thrust
            handF  = { coiledHandX * (1.0f - snap) + maxReachHandX * snap, punchY };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, punchY - 3.0f * (1.0f - snap) };
        } else { // RECOVERY
            float t = pose.actionProgress;
            handF  = { maxReachHandX * (1.0f - t) + readyHandX * t, punchY * (1.0f - t) + (drawY + 104.0f) * t };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, (punchY - 3.0f) * (1.0f - t) + (drawY + 88.0f) * t };
        }

        // Back arm tightly chambered
        elbowB = { cx - dir * 8.0f, drawY + 90.0f };
        handB  = { cx + dir * 4.0f, drawY + 92.0f };

    } else if (state == FighterState::KICK) {
        // Dynamic martial roundhouse / side kick
        hips       = { cx + dir * (pose.torsoLeanX * 0.3f), drawY + 68.0f };
        waist      = { cx + dir * (pose.torsoLeanX * 0.7f), drawY + 82.0f };
        chest      = { cx + dir * (pose.torsoLeanX * 1.3f), drawY + 105.0f };
        neck       = { chest.x - dir * 3.0f,                chest.y + 12.0f };
        headCenter = { neck.x - dir * 3.0f,                 neck.y + 13.0f };

        // Supporting leg firmly rooted
        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * 10.0f, drawY + 34.0f };
        ankleB = { cx - dir * 14.0f, drawY + 5.0f };
        toeB   = { cx - dir * 6.0f,  drawY + 3.0f };

        // Kicking leg
        hipF = { hips.x + dir * 6.0f, hips.y };
        float maxReachFootX = (facingRight ? (drawX + drawW) : drawX) + dir * Attacks::KICK.reach * 0.98f;
        float kickY         = drawY + Attacks::KICK.hitboxOffsetY + Attacks::KICK.hitboxHeight * 0.5f;
        float readyFootX    = cx + dir * 22.0f;
        float chamberFootX  = cx + dir * 12.0f;
        float chamberFootY  = drawY + 55.0f;
        float chamberKneeX  = cx + dir * 22.0f;
        float chamberKneeY  = drawY + 70.0f;

        if (phase == AttackPhase::STARTUP) {
            float t = pose.actionProgress;
            float ease = t * t;
            ankleF = { readyFootX * (1.0f - ease) + chamberFootX * ease, (drawY + 5.0f) * (1.0f - ease) + chamberFootY * ease };
            kneeF  = { (cx + dir * 20.0f) * (1.0f - ease) + chamberKneeX * ease, (drawY + 36.0f) * (1.0f - ease) + chamberKneeY * ease };
            toeF   = { ankleF.x + dir * 8.0f, ankleF.y + 3.0f * ease };
        } else if (phase == AttackPhase::ACTIVE) {
            float t = pose.actionProgress;
            float snap = t * (2.0f - t);
            ankleF = { chamberFootX * (1.0f - snap) + (maxReachFootX - dir * 6.0f) * snap, chamberFootY * (1.0f - snap) + kickY * snap };
            kneeF  = { (hipF.x + ankleF.x) * 0.50f, chamberKneeY * (1.0f - snap) + (kickY + 8.0f) * snap };
            toeF   = { ankleF.x + dir * 8.0f, ankleF.y };
        } else { // RECOVERY
            float t = pose.actionProgress;
            ankleF = { (maxReachFootX - dir * 6.0f) * (1.0f - t) + readyFootX * t, kickY * (1.0f - t) + (drawY + 5.0f) * t };
            kneeF  = { (hipF.x + ankleF.x) * 0.50f + dir * 4.0f, (drawY + 46.0f) * (1.0f - t) + (drawY + 36.0f) * t };
            toeF   = { ankleF.x + dir * 8.0f, ankleF.y - 1.5f };
        }

        shoulderF = { chest.x + dir * 5.0f, chest.y };
        elbowF    = { cx + dir * 6.0f,  drawY + 98.0f };
        handF     = { cx + dir * 15.0f, drawY + 102.0f };

        shoulderB = { chest.x - dir * 5.0f, chest.y };
        elbowB    = { cx - dir * 24.0f, drawY + 88.0f };
        handB     = { cx - dir * 30.0f, drawY + 78.0f };

    } else if (state == FighterState::SPECIAL) {
        // Continuous powerhouse special strike
        hips       = { cx + dir * (4.0f + pose.torsoLeanX * 0.35f), drawY + 68.0f };
        waist      = { cx + dir * (9.0f + pose.torsoLeanX * 0.7f),  drawY + 82.0f };
        chest      = { cx + dir * (15.0f + pose.torsoLeanX * 1.3f), drawY + 108.0f };
        neck       = { chest.x + dir * 3.5f,                        chest.y + 12.0f };
        headCenter = { neck.x + dir * 3.5f,                         neck.y + 13.0f };

        hipF   = { hips.x + dir * 6.0f, hips.y };
        kneeF  = { cx + dir * 28.0f, drawY + 34.0f };
        ankleF = { cx + dir * 32.0f, drawY + 5.0f };
        toeF   = { cx + dir * 42.0f, drawY + 3.0f };

        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * 18.0f, drawY + 30.0f };
        ankleB = { cx - dir * 28.0f, drawY + 5.0f };
        toeB   = { cx - dir * 18.0f, drawY + 3.0f };

        shoulderF = { chest.x + dir * 7.0f, chest.y };
        shoulderB = { chest.x - dir * 6.0f, chest.y };

        float maxReachHandX = (facingRight ? (drawX + drawW) : drawX) + dir * Attacks::SPECIAL.reach * 0.98f;
        float specY         = drawY + Attacks::SPECIAL.hitboxOffsetY + Attacks::SPECIAL.hitboxHeight * 0.5f;

        if (phase == AttackPhase::STARTUP) {
            float t = pose.actionProgress;
            elbowF = { cx + dir * (22.0f * (1.0f - t) + 6.0f * t),  drawY + 88.0f * (1.0f - t) + 94.0f * t };
            handF  = { cx + dir * (28.0f * (1.0f - t) + 12.0f * t), drawY + 104.0f * (1.0f - t) + 98.0f * t };
        } else if (phase == AttackPhase::ACTIVE) {
            float t = pose.actionProgress;
            float snap = t * (2.0f - t);
            handF  = { (cx + dir * 12.0f) * (1.0f - snap) + maxReachHandX * snap, specY };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, specY - 3.0f };
        } else { // RECOVERY
            float t = pose.actionProgress;
            handF  = { maxReachHandX * (1.0f - t) + (cx + dir * 28.0f) * t, specY * (1.0f - t) + (drawY + 104.0f) * t };
            elbowF = { (shoulderF.x + handF.x) * 0.5f, specY * (1.0f - t) + (drawY + 88.0f) * t };
        }

        elbowB = { cx - dir * 12.0f, drawY + 88.0f };
        handB  = { cx - dir * 18.0f, drawY + 82.0f };

    } else {
        // IDLE: Poised combat stance with breathing rhythm
        hips       = { cx,                drawY + 68.0f + pose.breathY * 0.4f };
        waist      = { cx + dir * 2.0f,   drawY + 82.0f + pose.breathY * 0.7f };
        chest      = { cx + dir * 5.0f,   drawY + 108.0f + pose.breathY };
        neck       = { cx + dir * 6.5f,   drawY + 120.0f + pose.breathY * 0.9f };
        headCenter = { cx + dir * 8.0f,   drawY + 133.0f + pose.breathY * 0.9f };

        hipF   = { hips.x + dir * 6.0f, hips.y };
        kneeF  = { cx + dir * 20.0f, drawY + 36.0f };
        ankleF = { cx + dir * 22.0f, drawY + 5.0f };
        toeF   = { cx + dir * 32.0f, drawY + 3.0f };

        hipB   = { hips.x - dir * 6.0f, hips.y };
        kneeB  = { cx - dir * 12.0f, drawY + 36.0f };
        ankleB = { cx - dir * 20.0f, drawY + 5.0f };
        toeB   = { cx - dir * 10.0f, drawY + 3.0f };

        shoulderF = { chest.x + dir * 6.0f, chest.y };
        elbowF    = { cx + dir * 22.0f, drawY + 88.0f + pose.breathY * 0.8f };
        handF     = { cx + dir * 28.0f, drawY + 104.0f + pose.breathY * 1.2f };

        shoulderB = { chest.x - dir * 5.0f, chest.y };
        elbowB    = { cx - dir * 5.0f,  drawY + 84.0f + pose.breathY * 0.6f };
        handB     = { cx + dir * 10.0f, drawY + 96.0f + pose.breathY * 0.9f };
    }

    // =========================================================================
    // LAYER 1: BACK LIMBS & BILLOWING ATTIRE (Depth-shaded behind torso)
    // =========================================================================
    // Muscular Back Leg (Thigh, Calf, Foot)
    renderer.drawLimb(hipB.x, hipB.y, kneeB.x, kneeB.y, 8.5f, 7.0f, backR, backG, backB);
    renderer.drawLimb(kneeB.x, kneeB.y, ankleB.x, ankleB.y, 7.0f, 5.2f, backR, backG, backB);
    renderer.drawLimb(ankleB.x, ankleB.y, toeB.x, toeB.y, 5.2f, 4.0f, backR, backG, backB);
    renderer.drawCircle(ankleB.x - dir * 2.0f, ankleB.y - 1.0f, 4.5f, backR, backG, backB);

    // Athletic Back Arm & Bare Fighter Fist (NO WEAPONS)
    renderer.drawLimb(shoulderB.x, shoulderB.y, elbowB.x, elbowB.y, 6.2f, 5.2f, backR, backG, backB);
    renderer.drawLimb(elbowB.x, elbowB.y, handB.x, handB.y, 5.2f, 4.5f, backR, backG, backB);
    // Bare Clenched Fist
    renderer.drawCircle(handB.x, handB.y, 5.2f, backR, backG, backB);
    renderer.drawCircle(handB.x + dir * 2.5f, handB.y + 0.5f, 4.2f, backR, backG, backB);

    // Character-specific back silhouette
    if (role == FighterRole::ARMORED_SPEARMAN) {
        // Heavy Striker / Warrior: Billowing coat-tails / tunic skirts trailing backwards (from reference image!)
        float skirtWave = (state == FighterState::DEAD) ? 0.0f : std::sin(stateTime * 7.0f) * 4.0f;
        renderer.drawQuad(waist.x - dir * 4.0f, waist.y,
                          waist.x - dir * 10.0f, waist.y,
                          cx - dir * (36.0f + skirtWave), drawY + 10.0f,
                          cx - dir * (24.0f + skirtWave), drawY + 10.0f,
                          backR, backG, backB);
        renderer.drawQuad(waist.x - dir * 8.0f, waist.y - 4.0f,
                          waist.x - dir * 14.0f, waist.y - 4.0f,
                          cx - dir * (46.0f + skirtWave * 1.3f), drawY + 6.0f,
                          cx - dir * (34.0f + skirtWave * 1.3f), drawY + 6.0f,
                          backR * 0.85f, backG * 0.85f, backB * 0.85f);
    }

    // =========================================================================
    // LAYER 2: TORSO & ANATOMICAL MUSCLE STRUCTURE (Middle layer)
    // =========================================================================
    // Muscular Upper Chest / Pectorals / Shoulders (Tapered athletic V-taper)
    renderer.drawQuad(chest.x - 13.5f, chest.y,
                      chest.x + 13.5f, chest.y,
                      waist.x + 9.5f,  waist.y,
                      waist.x - 9.5f,  waist.y,
                      bodyR, bodyG, bodyB);

    // Lower Abdomen & Muscular Hips
    renderer.drawQuad(waist.x - 9.5f,  waist.y,
                      waist.x + 9.5f,  waist.y,
                      hips.x + 11.5f,  hips.y,
                      hips.x - 11.5f,  hips.y,
                      bodyR, bodyG, bodyB);

    // Muscular Neck (Tapered into shoulders)
    renderer.drawLimb(neck.x, neck.y - 3.0f, headCenter.x, headCenter.y - 6.0f, 4.6f, 5.4f, bodyR, bodyG, bodyB);

    // Athletic Belt / Sash
    renderer.drawRect(waist.x - 10.5f, waist.y - 3.5f, 21.0f, 7.0f, accentR, accentG, accentB);

    if (role == FighterRole::NINJA) {
        // Ninja: Hanging sash ribbon tails fluttering in the wind
        if (state != FighterState::DEAD) {
            float sashWave = std::sin(stateTime * 6.0f) * 3.0f;
            renderer.drawQuad(waist.x - dir * 3.0f, waist.y - 2.0f,
                              waist.x - dir * 9.0f, waist.y - 2.0f,
                              waist.x - dir * 14.0f + sashWave, waist.y - 20.0f,
                              waist.x - dir * 8.0f + sashWave,  waist.y - 20.0f,
                              accentR, accentG, accentB);
        }
    } else {
        // Warrior: Belt buckle outline and front tunic skirt
        renderer.drawRectOutline(waist.x - 11.0f, waist.y - 4.0f, 22.0f, 8.0f, 0.45f, 0.45f, 0.50f);
        float skirtWave = (state == FighterState::DEAD) ? 0.0f : std::sin(stateTime * 7.0f) * 3.0f;
        renderer.drawQuad(waist.x, waist.y,
                          waist.x - dir * 7.0f, waist.y,
                          cx - dir * (20.0f + skirtWave * 0.8f), drawY + 16.0f,
                          cx - dir * (12.0f + skirtWave * 0.8f), drawY + 16.0f,
                          bodyR, bodyG, bodyB);
    }

    // =========================================================================
    // LAYER 3: FRONT LIMBS & BARE FIGHTER HANDS (NO WEAPONS - REAL HUMAN HANDS)
    // =========================================================================
    // Muscular Front Leg (Thigh, Knee, Calf, Foot)
    renderer.drawLimb(hipF.x, hipF.y, kneeF.x, kneeF.y, 8.5f, 7.0f, bodyR, bodyG, bodyB);
    renderer.drawLimb(kneeF.x, kneeF.y, ankleF.x, ankleF.y, 7.0f, 5.2f, bodyR, bodyG, bodyB);
    renderer.drawLimb(ankleF.x, ankleF.y, toeF.x, toeF.y, 5.2f, 4.0f, bodyR, bodyG, bodyB);
    renderer.drawCircle(ankleF.x - dir * 2.0f, ankleF.y - 1.0f, 4.5f, bodyR, bodyG, bodyB);

    // Front Arm (Deltoid, Biceps, Forearm)
    renderer.drawCircle(shoulderF.x, shoulderF.y, 7.0f, bodyR, bodyG, bodyB);
    renderer.drawLimb(shoulderF.x, shoulderF.y, elbowF.x, elbowF.y, 6.2f, 5.2f, bodyR, bodyG, bodyB);
    renderer.drawLimb(elbowF.x, elbowF.y, handF.x, handF.y, 5.2f, 4.5f, bodyR, bodyG, bodyB);

    // Real Human Fighter Hand (Clenched Fist / Striking Knuckles) - NO WEAPONS
    renderer.drawCircle(handF.x, handF.y, 5.5f, bodyR, bodyG, bodyB);
    renderer.drawLimb(handF.x, handF.y, handF.x + dir * 4.0f, handF.y + 0.5f, 5.0f, 4.0f, bodyR, bodyG, bodyB);
    renderer.drawCircle(handF.x + dir * 1.5f, handF.y - 2.5f, 2.8f, bodyR, bodyG, bodyB);

    // =========================================================================
    // LAYER 4: HEAD, HOOD / HELMET & GLOWING EYES
    // =========================================================================
    if (role == FighterRole::NINJA) {
        // Ninja: Proportional head with cowl / hood and tapered cloth chin
        renderer.drawCircle(headCenter.x, headCenter.y, headRadius, bodyR, bodyG, bodyB);
        renderer.drawTriangle(headCenter.x - 8.5f, headCenter.y - 3.0f,
                              headCenter.x + 8.5f, headCenter.y - 3.0f,
                              headCenter.x + dir * 2.5f, headCenter.y - 14.0f,
                              bodyR, bodyG, bodyB);

        if (state != FighterState::DEAD) {
            // Headband
            renderer.drawRect(headCenter.x - 11.0f, headCenter.y + 0.5f, 22.0f, 4.5f, accentR, accentG, accentB);

            // Dynamic fluttering ribbons
            float tailWave1 = std::sin(stateTime * 10.0f) * 3.5f;
            float tailWave2 = std::cos(stateTime * 8.0f) * 4.0f;

            renderer.drawQuad(headCenter.x - dir * 10.0f, headCenter.y + 4.0f,
                              headCenter.x - dir * 10.0f, headCenter.y + 1.0f,
                              headCenter.x - dir * 26.0f, headCenter.y - 2.0f + tailWave1,
                              headCenter.x - dir * 26.0f, headCenter.y + 1.5f + tailWave1,
                              accentR, accentG, accentB);
            renderer.drawQuad(headCenter.x - dir * 10.0f, headCenter.y + 2.0f,
                              headCenter.x - dir * 10.0f, headCenter.y - 1.0f,
                              headCenter.x - dir * 32.0f, headCenter.y - 8.0f + tailWave2,
                              headCenter.x - dir * 32.0f, headCenter.y - 5.0f + tailWave2,
                              accentR * 0.85f, accentG * 0.85f, accentB * 0.85f);

            // Razor-sharp glowing cyan eye slit
            if (drawEye) {
                float eyeX = headCenter.x + dir * 5.0f;
                float eyeY = headCenter.y + 0.3f;
                renderer.drawLine(eyeX - dir * 3.0f, eyeY - 1.0f,
                                  eyeX + dir * 4.0f, eyeY + 1.0f,
                                  accentR, accentG, accentB, 1.0f, 2.5f);
                renderer.drawCircle(eyeX + dir * 1.5f, eyeY + 0.4f, 1.6f, 1.0f, 1.0f, 1.0f, 0.95f, 8);
            }
        }
    } else {
        // Warrior: Proportional head with warrior cowl / combat helmet
        renderer.drawCircle(headCenter.x, headCenter.y, headRadius, bodyR, bodyG, bodyB);

        // Pointed helmet crest
        renderer.drawTriangle(headCenter.x - dir * 9.0f, headCenter.y + 7.0f,
                              headCenter.x + dir * 8.0f, headCenter.y + 7.0f,
                              headCenter.x - dir * 2.0f, headCenter.y + 22.0f,
                              bodyR * 1.15f, bodyG * 1.15f, bodyB * 1.15f);

        // Angled brow visor protruding over face
        renderer.drawQuad(headCenter.x - dir * 3.0f, headCenter.y + 5.0f,
                          headCenter.x + dir * 13.0f, headCenter.y + 2.0f,
                          headCenter.x + dir * 11.0f, headCenter.y - 1.5f,
                          headCenter.x - dir * 2.0f, headCenter.y + 1.5f,
                          bodyR * 1.3f, bodyG * 1.3f, bodyB * 1.3f);

        // Flared neck guard (aventail) sloping down behind neck
        renderer.drawQuad(headCenter.x - dir * 3.0f, headCenter.y + 2.0f,
                          headCenter.x - dir * 14.0f, headCenter.y - 8.0f,
                          headCenter.x - dir * 11.0f, headCenter.y - 11.0f,
                          headCenter.x, headCenter.y - 4.0f,
                          bodyR * 1.1f, bodyG * 1.1f, bodyB * 1.1f);

        // Tapered chin guard
        renderer.drawTriangle(headCenter.x - 8.0f, headCenter.y - 3.0f,
                              headCenter.x + 8.0f, headCenter.y - 3.0f,
                              headCenter.x + dir * 2.0f, headCenter.y - 12.0f,
                              bodyR, bodyG, bodyB);

        // Piercing amber eye slit
        if (drawEye) {
            float eyeX = headCenter.x + dir * 4.5f;
            float eyeY = headCenter.y - 0.2f;
            renderer.drawLine(eyeX - dir * 3.0f, eyeY - 0.8f,
                              eyeX + dir * 4.0f, eyeY + 0.8f,
                              accentR, accentG, accentB, 1.0f, 2.5f);
            renderer.drawCircle(eyeX + dir * 1.5f, eyeY + 0.2f, 1.6f, 1.0f, 0.95f, 0.5f, 0.95f, 8);
        }
    }

    // =========================================================================
    // LAYER 5: COMBAT AURAS (Shield & Special attack ki energy)
    // =========================================================================
    if (state == FighterState::BLOCK) {
        bool flashing = (blockFlashTimer > 0.0f);
        float shieldX = cx + dir * 28.0f;
        float shieldY = drawY + 26.0f;
        float shieldW = flashing ? 16.0f : 10.0f;
        float shieldH = drawH * 0.78f;

        if (flashing) {
            renderer.drawRectAlpha(shieldX - shieldW * 0.5f, shieldY, shieldW, shieldH,
                                   1.0f, 1.0f, 0.70f, 0.85f);
            renderer.drawCircle(shieldX, shieldY + shieldH * 0.5f, 32.0f, 1.0f, 0.95f, 0.5f, 0.30f, 20);
        } else {
            renderer.drawRectAlpha(shieldX - shieldW * 0.5f, shieldY, shieldW, shieldH,
                                   0.30f, 0.65f, 1.0f, 0.65f);
        }
    }

    if (state == FighterState::SPECIAL && phase == AttackPhase::ACTIVE) {
        // Fiery dragon energy aura gathering on striking hand
        renderer.drawCircle(handF.x, handF.y, 24.0f, 1.0f, 0.65f, 0.15f, 0.40f, 18);
        renderer.drawCircle(handF.x, handF.y, 14.0f, 1.0f, 0.90f, 0.35f, 0.80f, 14);
        renderer.drawCircle(handF.x, handF.y,  6.0f, 1.0f, 1.0f,  1.0f,  0.95f, 10);
    }
}
