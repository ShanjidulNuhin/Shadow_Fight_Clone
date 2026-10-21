#include "Fighter.h"
#include "Renderer.h"
#include "Animation.h"
#include "Config.h"

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
      colorR(1.0f), colorG(1.0f), colorB(1.0f),
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

void Fighter::receiveHit(const AttackData& attack, float pushDirection)
{
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
    if (attack == nullptr)                                  return AttackPhase::NONE;
    if (stateTime < attack->startup)                        return AttackPhase::STARTUP;
    if (stateTime < attack->startup + attack->active)       return AttackPhase::ACTIVE;
    if (stateTime < attack->totalTime())                    return AttackPhase::RECOVERY;
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
        bodyR = 1.0f; bodyG = 0.25f; bodyB = 0.25f;
    } else if (state == FighterState::SPECIAL && phase == AttackPhase::STARTUP) {
        bodyR = 0.6f; bodyG = 0.9f; bodyB = 1.0f;
    }

    if (state == FighterState::DEAD) {
        bodyR *= 0.4f; bodyG *= 0.4f; bodyB *= 0.4f;
    } else if (isInvulnerable() && ((int)(invulnerableTimer / BLINK_INTERVAL) % 2 == 0)) {
        bodyR *= 0.45f; bodyG *= 0.45f; bodyB *= 0.45f;
    }

    float drawW = width  * pose.scaleX;
    float drawH = height * pose.scaleY;
    float drawX = x + (width - drawW) / 2.0f;
    float drawY = y + pose.offsetY;
    renderer.drawRect(drawX, drawY, drawW, drawH, bodyR, bodyG, bodyB);

    if (state != FighterState::DEAD) {
        const float eyeSize = 10.0f;
        float eyeX = facingRight ? (drawX + drawW - eyeSize - 4.0f) : (drawX + 4.0f);
        float eyeY = drawY + drawH - eyeSize - 8.0f;
        renderer.drawRect(eyeX, eyeY, eyeSize, eyeSize, 0.1f, 0.1f, 0.1f);
    }

    if (state == FighterState::BLOCK) {
        bool flashing = blockFlashTimer > 0.0f;
        float shieldWidth = flashing ? 14.0f : 8.0f;
        float shieldX = facingRight ? (drawX + drawW + 2.0f) : (drawX - 2.0f - shieldWidth);
        if (flashing) {
            renderer.drawRect(shieldX, drawY - 4.0f, shieldWidth, drawH + 8.0f, 1.0f, 1.0f, 0.7f);
        } else {
            renderer.drawRect(shieldX, drawY - 4.0f, shieldWidth, drawH + 8.0f, 0.3f, 0.6f, 1.0f);
        }
    }

    const AttackData* attack = getCurrentAttack();
    if (attack != nullptr) {
        float limbLength = attack->reach * 0.25f;
        float limbR = colorR, limbG = colorG, limbB = colorB;
        if (phase == AttackPhase::RECOVERY) limbLength = attack->reach * 0.55f;
        if (phase == AttackPhase::ACTIVE) {
            limbLength = attack->reach;
            if (state == FighterState::SPECIAL) {
                limbR = 1.0f; limbG = 0.7f; limbB = 0.1f;
            } else {
                limbR = 1.0f; limbG = 0.35f; limbB = 0.35f;
            }
        }

        float limbThickness = attack->hitboxHeight * 0.6f;
        float limbY = drawY + attack->hitboxOffsetY + (attack->hitboxHeight - limbThickness) / 2.0f;
        float limbX = facingRight ? (drawX + drawW) : (drawX - limbLength);
        renderer.drawRect(limbX, limbY, limbLength, limbThickness, limbR, limbG, limbB);
    }
}
