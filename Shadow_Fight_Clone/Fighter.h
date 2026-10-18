#ifndef FIGHTER_H
#define FIGHTER_H

#include "Collision.h"    // for Rect
#include "AttackData.h"   // for AttackData

class Renderer;   // forward declaration: we only need the name here

// What a fighter is doing right now. Only one at a time.
enum class FighterState {
    IDLE,
    WALK,
    JUMP,     // in the air, moving up
    FALL,     // in the air, moving down
    CROUCH,
    BLOCK,    // guarding
    PUNCH,
    KICK,
    HIT       // stunned after being hit
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
    void setBlocking(bool wantsToBlock);   // call every frame; only works while standing
    void startPunch();                     // only works while standing or walking
    void startKick();                      // only works while standing or walking
    void applyPhysics(float deltaTime);    // gravity, knockback, landing; call once per frame
    void updateState(float deltaTime);     // decide the current state; call last each frame

    void setColor(float r, float g, float b);
    void setFacingRight(bool faceRight);

    // Hurtbox: the area where this fighter can be hit (its body).
    Rect getHurtbox() const;

    // Hitbox: the area where this fighter's attack can hit someone.
    // Returns false (and leaves outBox alone) unless an attack is in its ACTIVE phase
    // and hasn't already hit someone.
    bool getHitbox(Rect& outBox) const;

    // --- combat ---
    // The attack being performed right now, or nullptr if not attacking.
    const AttackData* getCurrentAttack() const;
    void onAttackConnected();              // call when this fighter's attack has landed

    // Called when this fighter gets hit. If it is blocking AND facing the attacker,
    // the hit is mostly absorbed; otherwise: full damage + stun + knockback.
    // pushDirection: +1 = pushed to the right, -1 = pushed to the left.
    void receiveHit(const AttackData& attack, float pushDirection);

    void takeDamage(int amount);
    void restoreHealth();
    int  getHealth() const;
    int  getMaxHealth() const;
    bool isDead() const;

    bool         canAct() const;           // false while attacking or stunned
    bool         isAttacking() const;
    AttackPhase  getAttackPhase() const;
    FighterState getState() const;
    const char*  getStateName() const;     // e.g. "WALK", handy for debugging

    void draw(const Renderer& renderer) const;

private:
    void keepInsideWorld();
    void startAttack(FighterState attackState);

    float x;
    float y;
    float width;
    float height;          // CURRENT height (changes when crouching)
    float standingHeight;
    float crouchHeight;
    float moveSpeed;       // pixels per second
    float velocityY;       // vertical speed in pixels per second (+ = up)
    float knockbackVelocityX;   // sideways push from a hit (+ = right), fades to 0
    float moveDirection;   // -1, 0 or +1: what the fighter tried to do this frame
    bool  facingRight;
    bool  onGround;
    bool  crouching;
    bool  blocking;
    FighterState state;
    float stateTime;       // seconds spent in the current state
    float hitStunDuration; // how long the current HIT state lasts
    float blockFlashTimer; // > 0 right after a successful block (for the visual flash)
    float colorR, colorG, colorB;   // body colour
    int   health;
    int   maxHealth;
    bool  attackHasHit;    // has the current swing already hit someone?
};

#endif
