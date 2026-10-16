#include "Fighter.h"
#include "Renderer.h"
#include "Animation.h"
#include "Config.h"

namespace {
    const float CROUCH_HEIGHT_RATIO = 0.6f;  // crouching body is 60% as tall
    const float CROUCH_SPEED_RATIO  = 0.4f;  // and walks at 40% speed

    const int   MAX_HEALTH = 100;

    // Punch timing, in seconds
    const float PUNCH_STARTUP   = 0.08f;
    const float PUNCH_ACTIVE    = 0.10f;
    const float PUNCH_RECOVERY  = 0.17f;
    const float PUNCH_TOTAL     = PUNCH_STARTUP + PUNCH_ACTIVE + PUNCH_RECOVERY;

    // Punch shape and power
    const float PUNCH_REACH         = 50.0f;  // how far the fist sticks out from the body
    const float PUNCH_HITBOX_HEIGHT = 24.0f;
    const int   PUNCH_DAMAGE        = 10;

    // Getting hit
    const float HIT_STUN_TIME      = 0.30f;    // seconds the victim can't act
    const float KNOCKBACK_SPEED    = 400.0f;   // initial sideways speed (pixels/second)
    const float KNOCKBACK_DECEL    = 1600.0f;  // how fast that speed fades (pixels/second^2)
}

Fighter::Fighter(float startX, float startY, float bodyWidth, float bodyHeight)
    : x(startX), y(startY), width(bodyWidth), height(bodyHeight),
      standingHeight(bodyHeight), crouchHeight(bodyHeight * CROUCH_HEIGHT_RATIO),
      moveSpeed(250.0f), velocityY(0.0f), knockbackVelocityX(0.0f),
      moveDirection(0.0f), facingRight(true),
      onGround(false), crouching(false), state(FighterState::IDLE), stateTime(0.0f),
      colorR(1.0f), colorG(1.0f), colorB(1.0f),
      health(MAX_HEALTH), maxHealth(MAX_HEALTH), attackHasHit(false) {
}

void Fighter::keepInsideWorld() {
    if (x < 0.0f) x = 0.0f;
    if (x + width > Config::WORLD_WIDTH) x = Config::WORLD_WIDTH - width;
}

void Fighter::moveHorizontal(float direction, float deltaTime) {
    if (!canAct()) direction = 0.0f;   // can't walk or turn around mid-attack or while stunned
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

void Fighter::startPunch() {
    // only from a calm, grounded state (not crouching, not mid-air, not already attacking)
    if (state == FighterState::IDLE || state == FighterState::WALK) {
        state = FighterState::PUNCH;
        stateTime = 0.0f;
        attackHasHit = false;      // a fresh swing that hasn't hit anyone yet
    }
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
    FighterState newState;

    if (state == FighterState::HIT && stateTime < HIT_STUN_TIME) {
        newState = FighterState::HIT;     // still stunned: nothing else can happen yet
    } else if (state == FighterState::PUNCH && stateTime < PUNCH_TOTAL) {
        newState = FighterState::PUNCH;   // attack still in progress: nothing interrupts it
    } else if (!onGround) {
        newState = (velocityY > 0.0f) ? FighterState::JUMP : FighterState::FALL;
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
    if (getAttackPhase() != AttackPhase::ACTIVE) return false;

    outBox.width  = PUNCH_REACH;
    outBox.height = PUNCH_HITBOX_HEIGHT;
    outBox.x = facingRight ? (x + width) : (x - PUNCH_REACH);
    outBox.y = y + height * 0.6f - 5.0f;   // around the height of the fist
    return true;
}

int Fighter::getAttackDamage() const {
    return PUNCH_DAMAGE;   // only one attack so far
}

void Fighter::onAttackConnected() {
    attackHasHit = true;
}

void Fighter::receiveHit(int damage, float pushDirection) {
    takeDamage(damage);

    // Being hit interrupts whatever we were doing, even a punch in progress.
    state = FighterState::HIT;
    stateTime = 0.0f;
    knockbackVelocityX = pushDirection * KNOCKBACK_SPEED;

    // a stunned fighter is never crouching
    crouching = false;
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

bool Fighter::canAct() const {
    return state != FighterState::PUNCH && state != FighterState::HIT;
}

bool Fighter::isAttacking() const {
    return state == FighterState::PUNCH;
}

AttackPhase Fighter::getAttackPhase() const {
    if (state != FighterState::PUNCH)               return AttackPhase::NONE;
    if (stateTime < PUNCH_STARTUP)                  return AttackPhase::STARTUP;
    if (stateTime < PUNCH_STARTUP + PUNCH_ACTIVE)   return AttackPhase::ACTIVE;
    if (stateTime < PUNCH_TOTAL)                    return AttackPhase::RECOVERY;
    return AttackPhase::NONE;
}

FighterState Fighter::getState() const {
    return state;
}

const char* Fighter::getStateName() const {
    switch (state) {
        case FighterState::IDLE:   return "IDLE";
        case FighterState::WALK:   return "WALK";
        case FighterState::JUMP:   return "JUMP";
        case FighterState::FALL:   return "FALL";
        case FighterState::CROUCH: return "CROUCH";
        case FighterState::PUNCH:  return "PUNCH";
        case FighterState::HIT:    return "HIT";
    }
    return "UNKNOWN";
}

void Fighter::draw(const Renderer& renderer) const {
    Pose pose = Animation::getPose(state, stateTime);

    // normal body colour, but flash red while stunned
    float bodyR = colorR, bodyG = colorG, bodyB = colorB;
    if (state == FighterState::HIT) {
        bodyR = 1.0f; bodyG = 0.25f; bodyB = 0.25f;
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

    // punching arm: its length and colour show which phase of the attack we're in
    if (state == FighterState::PUNCH) {
        AttackPhase phase = getAttackPhase();

        float armLength = 20.0f;
        float armR = colorR, armG = colorG, armB = colorB;
        if (phase == AttackPhase::STARTUP)  armLength = 12.0f;
        if (phase == AttackPhase::RECOVERY) armLength = 28.0f;
        if (phase == AttackPhase::ACTIVE) {
            armLength = PUNCH_REACH;           // fully extended...
            armR = 1.0f; armG = 0.35f; armB = 0.35f;   // ...and red: the dangerous moment
        }

        const float armThickness = 14.0f;
        float armY = drawY + drawH * 0.6f;
        float armX = facingRight ? (drawX + drawW) : (drawX - armLength);
        renderer.drawRect(armX, armY, armLength, armThickness, armR, armG, armB);
    }
}
