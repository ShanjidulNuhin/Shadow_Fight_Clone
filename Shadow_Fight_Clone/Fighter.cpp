#include "Fighter.h"
#include "Renderer.h"
#include "Animation.h"
#include "Config.h"

namespace {
    const float CROUCH_HEIGHT_RATIO = 0.6f;  // crouching body is 60% as tall
    const float CROUCH_SPEED_RATIO  = 0.4f;  // and walks at 40% speed

    // Punch timing, in seconds
    const float PUNCH_STARTUP   = 0.08f;
    const float PUNCH_ACTIVE    = 0.10f;
    const float PUNCH_RECOVERY  = 0.17f;
    const float PUNCH_TOTAL     = PUNCH_STARTUP + PUNCH_ACTIVE + PUNCH_RECOVERY;
}

Fighter::Fighter(float startX, float startY, float bodyWidth, float bodyHeight)
    : x(startX), y(startY), width(bodyWidth), height(bodyHeight),
      standingHeight(bodyHeight), crouchHeight(bodyHeight * CROUCH_HEIGHT_RATIO),
      moveSpeed(250.0f), velocityY(0.0f), moveDirection(0.0f), facingRight(true),
      onGround(false), crouching(false), state(FighterState::IDLE), stateTime(0.0f) {
}

void Fighter::moveHorizontal(float direction, float deltaTime) {
    if (isAttacking()) direction = 0.0f;   // can't walk or turn around mid-attack
    moveDirection = direction;

    float speed = crouching ? moveSpeed * CROUCH_SPEED_RATIO : moveSpeed;
    x += direction * speed * deltaTime;

    if (direction > 0.0f) facingRight = true;
    else if (direction < 0.0f) facingRight = false;

    // keep the body inside the world
    if (x < 0.0f) x = 0.0f;
    if (x + width > Config::WORLD_WIDTH) x = Config::WORLD_WIDTH - width;
}

void Fighter::jump() {
    if (onGround && !isAttacking()) {
        velocityY = Config::JUMP_SPEED;
        onGround = false;
    }
}

void Fighter::setCrouching(bool wantsToCrouch) {
    if (isAttacking()) wantsToCrouch = false;

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
    }
}

void Fighter::applyPhysics(float deltaTime) {
    // gravity changes the speed, and the speed changes the position
    velocityY += Config::GRAVITY * deltaTime;
    y += velocityY * deltaTime;

    // landed on (or sunk into) the ground?
    if (y <= Config::GROUND_HEIGHT) {
        y = Config::GROUND_HEIGHT;
        velocityY = 0.0f;
        onGround = true;
    }
}

void Fighter::updateState(float deltaTime) {
    FighterState newState;

    if (state == FighterState::PUNCH && stateTime < PUNCH_TOTAL) {
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
    }
    return "UNKNOWN";
}

void Fighter::draw(const Renderer& renderer) const {
    Pose pose = Animation::getPose(state, stateTime);

    // The body, scaled around its bottom-centre so the feet stay in place.
    float drawW = width  * pose.scaleX;
    float drawH = height * pose.scaleY;
    float drawX = x + (width - drawW) / 2.0f;
    float drawY = y + pose.offsetY;
    renderer.drawRect(drawX, drawY, drawW, drawH, 1.0f, 1.0f, 1.0f);

    // small dark "eye" on the front so we can see which way we're facing
    const float eyeSize = 10.0f;
    float eyeX = facingRight ? (drawX + drawW - eyeSize - 4.0f) : (drawX + 4.0f);
    float eyeY = drawY + drawH - eyeSize - 8.0f;
    renderer.drawRect(eyeX, eyeY, eyeSize, eyeSize, 0.1f, 0.1f, 0.1f);

    // punching arm: its length and colour show which phase of the attack we're in
    if (state == FighterState::PUNCH) {
        AttackPhase phase = getAttackPhase();

        float armLength = 20.0f;
        float armR = 1.0f, armG = 1.0f, armB = 1.0f;
        if (phase == AttackPhase::STARTUP)  armLength = 12.0f;
        if (phase == AttackPhase::RECOVERY) armLength = 28.0f;
        if (phase == AttackPhase::ACTIVE) {
            armLength = 50.0f;                 // fully extended...
            armG = 0.35f; armB = 0.35f;        // ...and red: this is the dangerous moment
        }

        const float armThickness = 14.0f;
        float armY = drawY + drawH * 0.6f;
        float armX = facingRight ? (drawX + drawW) : (drawX - armLength);
        renderer.drawRect(armX, armY, armLength, armThickness, armR, armG, armB);
    }
}
