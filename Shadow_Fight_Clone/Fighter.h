#ifndef FIGHTER_H
#define FIGHTER_H

class Renderer;   // forward declaration: we only need the name here

// What a fighter is doing right now. Only one at a time.
enum class FighterState {
    IDLE,
    WALK,
    JUMP,     // in the air, moving up
    FALL,     // in the air, moving down
    CROUCH,
    PUNCH
};

// Every attack goes through these three phases, in order.
enum class AttackPhase {
    NONE,       // not attacking
    STARTUP,    // winding up: the attack can't hurt anyone yet
    ACTIVE,     // the dangerous moment
    RECOVERY    // pulling back: can't hurt anyone, and can't act yet either
};

// Anything that fights: has a position, a size, and can move and draw itself.
// (Position is the BOTTOM-LEFT corner of the body rectangle.)
class Fighter {
public:
    Fighter(float startX, float startY, float bodyWidth, float bodyHeight);

    // direction: -1 = left, 0 = stand still, +1 = right
    void moveHorizontal(float direction, float deltaTime);

    void jump();                           // only works while standing on the ground
    void setCrouching(bool wantsToCrouch); // call every frame; only works on the ground
    void startPunch();                     // only works while standing or walking
    void applyPhysics(float deltaTime);    // gravity + landing; call once per frame
    void updateState(float deltaTime);     // decide the current state; call last each frame

    bool         isAttacking() const;
    AttackPhase  getAttackPhase() const;
    FighterState getState() const;
    const char*  getStateName() const;     // e.g. "WALK", handy for debugging

    void draw(const Renderer& renderer) const;

private:
    float x;
    float y;
    float width;
    float height;          // CURRENT height (changes when crouching)
    float standingHeight;
    float crouchHeight;
    float moveSpeed;       // pixels per second
    float velocityY;       // vertical speed in pixels per second (+ = up)
    float moveDirection;   // -1, 0 or +1: what the fighter tried to do this frame
    bool  facingRight;
    bool  onGround;
    bool  crouching;
    FighterState state;
    float stateTime;       // seconds spent in the current state
};

#endif
