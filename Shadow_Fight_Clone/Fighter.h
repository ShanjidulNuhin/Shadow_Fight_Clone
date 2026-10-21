#ifndef FIGHTER_H
#define FIGHTER_H

#include "Collision.h"
#include "AttackData.h"

class Renderer;

enum class FighterState {
    IDLE,
    WALK,
    JUMP,
    FALL,
    CROUCH,
    BLOCK,
    PUNCH,
    KICK,
    SPECIAL,
    HIT,
    DEAD
};

enum class AttackPhase {
    NONE,
    STARTUP,
    ACTIVE,
    RECOVERY
};

class Fighter {
public:
    Fighter(float startX, float startY, float bodyWidth, float bodyHeight);

    void moveHorizontal(float direction, float deltaTime);

    void jump();
    void setCrouching(bool wantsToCrouch);
    void setBlocking(bool wantsToBlock);
    void startPunch();
    void startKick();
    void startSpecial();
    void applyPhysics(float deltaTime);
    void updateState(float deltaTime);

    void setColor(float r, float g, float b);
    void setFacingRight(bool faceRight);
    float pushHorizontally(float dx);

    Rect getHurtbox() const;

    bool getHitbox(Rect& outBox) const;
    const AttackData* getCurrentAttack() const;
    void onAttackConnected();
    void receiveHit(const AttackData& attack, float pushDirection);

    void takeDamage(int amount);
    void restoreHealth();
    int  getHealth() const;
    int  getMaxHealth() const;
    bool isDead() const;

    void  restoreEnergy();
    float getEnergy() const;
    float getMaxEnergy() const;

    bool         isInvulnerable() const;
    bool         canAct() const;
    bool         isAttacking() const;
    AttackPhase  getAttackPhase() const;
    FighterState getState() const;
    const char*  getStateName() const;
    void draw(const Renderer& renderer) const;

private:
    void keepInsideWorld();
    void startAttack(FighterState attackState);

    float x;
    float y;
    float width;
    float height;
    float standingHeight;
    float crouchHeight;
    float moveSpeed;
    float velocityY;
    float knockbackVelocityX;
    float moveDirection;
    bool  facingRight;
    bool  onGround;
    bool  crouching;
    bool  blocking;
    FighterState state;
    float stateTime;
    float hitStunDuration;
    float blockFlashTimer;
    float invulnerableTimer;
    float colorR, colorG, colorB;
    int   health;
    int   maxHealth;
    float energy;
    float maxEnergy;
    bool  attackHasHit;
};

#endif
