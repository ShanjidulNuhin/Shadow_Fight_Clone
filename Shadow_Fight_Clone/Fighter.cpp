#include "Fighter.h"
#include "Renderer.h"
#include "Animation.h"
#include "Config.h"

namespace {
    const float CROUCH_HEIGHT_RATIO = 0.6f;  // crouching body is 60% as tall
    const float CROUCH_SPEED_RATIO  = 0.4f;  // and walks at 40% speed

    const int   MAX_HEALTH = 100;

    const float MAX_ENERGY   = 100.0f;
    const float ENERGY_REGEN = 10.0f;        // energy gained per second

    // Getting hit
    const float KNOCKBACK_DECEL = 1600.0f;   // how fast a knockback fades (pixels/second^2)

    // Blocking
    const int   BLOCK_DAMAGE_PERCENT  = 20;     // a blocked hit still deals 20% damage
    const float BLOCK_KNOCKBACK_SPEED = 150.0f; // and pushes only a little
    const float BLOCK_FLASH_TIME      = 0.15f;  // how long the shield flashes after a block

    // Which attack data belongs to which state (nullptr = that state isn't an attack).
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
      blockFlashTimer(0.0f),
      colorR(1.0f), colorG(1.0f), colorB(1.0f),
      health(MAX_HEALTH), maxHealth(MAX_HEALTH),
      energy(MAX_ENERGY), maxEnergy(MAX_ENERGY), attackHasHit(false) {
}

void Fighter::keepInsideWorld() {
    if (x < 0.0f) x = 0.0f;
    if (x + width > Config::WORLD_WIDTH) x = Config::WORLD_WIDTH - width;
}

void Fighter::moveHorizontal(float direction, float deltaTime) {
    // can't walk or turn around while attacking, stunned, or holding a guard
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

    // can only crouch while standing on the ground
    if (wantsToCrouch && onGround) {
        crouching = true;
        height = crouchHeight;
    } else {
        crouching = false;
        height = standingHeight;
    }
}

void Fighter::setBlocking(bool wantsToBlock) {
    // only while standing on the ground and free to act
    if (!canAct() || crouching || !onGround) wantsToBlock = false;
    blocking = wantsToBlock;
}

void Fighter::startAttack(FighterState attackState) {
    const AttackData* data = attackDataFor(attackState);
    if (data == nullptr) return;

    // only from a calm, grounded state (not crouching, not mid-air, not already attacking)
    if (state != FighterState::IDLE && state != FighterState::WALK) return;
    if (!onGround) return;     // the state can lag one frame behind, so check the ground too

    // can we afford it?
    if (energy < data->energyCost) return;
    energy -= data->energyCost;

    state = attackState;
    stateTime = 0.0f;
    attackHasHit = false;      // a fresh swing that hasn't hit anyone yet
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
    // vertical: gravity changes the speed, and the speed changes the position
    velocityY += Config::GRAVITY * deltaTime;
    y += velocityY * deltaTime;

    // landed on (or sunk into) the ground?
    if (y <= Config::GROUND_HEIGHT) {
        y = Config::GROUND_HEIGHT;
        velocityY = 0.0f;
        onGround = true;
    }

    // horizontal knockback: slide sideways, slowing down until we stop
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

    // energy slowly refills
    energy += ENERGY_REGEN * deltaTime;
    if (energy > maxEnergy) energy = maxEnergy;

    const AttackData* attack = getCurrentAttack();
    FighterState newState;

    if (state == FighterState::HIT && stateTime < hitStunDuration) {
        newState = FighterState::HIT;     // still stunned: nothing else can happen yet
    } else if (attack != nullptr && stateTime < attack->totalTime()) {
        newState = state;                 // attack still in progress: nothing interrupts it
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
        state = newState;
        stateTime = 0.0f;          // entered a new state: restart its animation clock
    } else {
        stateTime += deltaTime;
    }
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
    if (attackHasHit) return false;                          // this swing already landed

    const AttackData* attack = getCurrentAttack();
    if (attack == nullptr) return false;
    if (getAttackPhase() != AttackPhase::ACTIVE) return false;

    outBox.width  = attack->reach;
    outBox.height = attack->hitboxHeight;
    outBox.x = facingRight ? (x + width) : (x - attack->reach);
    outBox.y = y + attack->hitboxOffsetY;   // measured up from the attacker's feet
    return true;
}

const AttackData* Fighter::getCurrentAttack() const {
    return attackDataFor(state);
}

void Fighter::onAttackConnected() {
    attackHasHit = true;
}

void Fighter::receiveHit(const AttackData& attack, float pushDirection) {
    // Where is the attacker? If we are pushed to the right (+1), it hit us from the left.
    bool attackerIsOnRight = (pushDirection < 0.0f);
    bool facingAttacker = (attackerIsOnRight == facingRight);

    // A guard only works against attacks coming from the front.
    if (blocking && facingAttacker) {
        int chipDamage = attack.damage * BLOCK_DAMAGE_PERCENT / 100;
        if (chipDamage < 1) chipDamage = 1;
        takeDamage(chipDamage);

        knockbackVelocityX = pushDirection * BLOCK_KNOCKBACK_SPEED;
        blockFlashTimer = BLOCK_FLASH_TIME;
        return;   // no stun, no interruption: we keep our guard up
    }

    takeDamage(attack.damage);

    // Being hit interrupts whatever we were doing, even an attack in progress.
    state = FighterState::HIT;
    stateTime = 0.0f;
    hitStunDuration = attack.hitStun;
    knockbackVelocityX = pushDirection * attack.knockbackSpeed;

    // a stunned fighter is neither crouching nor blocking
    crouching = false;
    blocking = false;
    height = standingHeight;
}

void Fighter::takeDamage(int amount) {
    health -= amount;
    if (health < 0) health = 0;
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

bool Fighter::canAct() const {
    return !isAttacking() && state != FighterState::HIT;
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
    }
    return "UNKNOWN";
}

void Fighter::draw(const Renderer& renderer) const {
    Pose pose = Animation::getPose(state, stateTime);
    AttackPhase phase = getAttackPhase();

    // normal body colour, but flash red while stunned, and glow while charging a special
    float bodyR = colorR, bodyG = colorG, bodyB = colorB;
    if (state == FighterState::HIT) {
        bodyR = 1.0f; bodyG = 0.25f; bodyB = 0.25f;
    } else if (state == FighterState::SPECIAL && phase == AttackPhase::STARTUP) {
        bodyR = 0.6f; bodyG = 0.9f; bodyB = 1.0f;
    }

    // The body, scaled around its bottom-centre so the feet stay in place.
    float drawW = width  * pose.scaleX;
    float drawH = height * pose.scaleY;
    float drawX = x + (width - drawW) / 2.0f;
    float drawY = y + pose.offsetY;
    renderer.drawRect(drawX, drawY, drawW, drawH, bodyR, bodyG, bodyB);

    // small dark "eye" on the front so we can see which way we're facing
    const float eyeSize = 10.0f;
    float eyeX = facingRight ? (drawX + drawW - eyeSize - 4.0f) : (drawX + 4.0f);
    float eyeY = drawY + drawH - eyeSize - 8.0f;
    renderer.drawRect(eyeX, eyeY, eyeSize, eyeSize, 0.1f, 0.1f, 0.1f);

    // guard: a blue shield bar in front of the body, which flashes bright when it absorbs a hit
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

    // the attacking limb (fist, foot, or special blast): length and colour show the phase
    const AttackData* attack = getCurrentAttack();
    if (attack != nullptr) {
        float limbLength = attack->reach * 0.25f;            // winding up
        float limbR = colorR, limbG = colorG, limbB = colorB;
        if (phase == AttackPhase::RECOVERY) limbLength = attack->reach * 0.55f;
        if (phase == AttackPhase::ACTIVE) {
            limbLength = attack->reach;                      // fully extended...
            if (state == FighterState::SPECIAL) {
                limbR = 1.0f; limbG = 0.7f; limbB = 0.1f;    // ...golden for the special
            } else {
                limbR = 1.0f; limbG = 0.35f; limbB = 0.35f;  // ...red: the dangerous moment
            }
        }

        float limbThickness = attack->hitboxHeight * 0.6f;
        float limbY = drawY + attack->hitboxOffsetY + (attack->hitboxHeight - limbThickness) / 2.0f;
        float limbX = facingRight ? (drawX + drawW) : (drawX - limbLength);
        renderer.drawRect(limbX, limbY, limbLength, limbThickness, limbR, limbG, limbB);
    }
}
