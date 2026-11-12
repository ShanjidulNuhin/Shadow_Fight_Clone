#ifndef FIGHTER_H
#define FIGHTER_H

#include "Collision.h"
#include "AttackData.h"
#include "CharacterData.h"
#include <vector>

class Renderer;

enum class FighterRole {
    NINJA,
    ARMORED_SPEARMAN
};

enum class FighterState {
    IDLE,
    WALK,
    JUMP,
    FALL,
    CROUCH,
    BLOCK,
    ROLL,
    PUNCH,
    COMBO2,
    COMBO3,
    KICK,
    JUMP_KICK,
    CHAIN_GRAB,
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

enum class ProjectileKind {
    WAR_ARROW,
    CHINESE_FAN,
    SKY_SPEAR,
    THROWN_SPEAR
};

struct ProjectileSpawn {
    ProjectileKind kind;
    float x;
    float y;
    float vx;
    float vy;
    float width;
    float height;
    int   damage;
    float knockback;
    float hitStun;
    float r, g, b;
};

class Fighter {
public:
    Fighter(float startX, float startY, float bodyWidth, float bodyHeight);

    void moveHorizontal(float direction, float deltaTime);

    void jump();
    void setCrouching(bool wantsToCrouch);
    void setBlocking(bool wantsToBlock);
    void setRunning(bool wantsToRun);
    bool isRunning() const;
    void startRoll(float rollDir);
    void startPunch();       // Also queues Combo 2 / Combo 3 if tapped during combo
    void startKick();        // Ground kick OR Flying Jump Kick if airborne (Ch 1, 3, 4)
    void startChainGrab();   // Long-range Spiked Chain pull + 2s stun (Ch 2)
    void startSpecial();     // Signature Critical / Special attack (L)

    void applyPhysics(float deltaTime);
    void updateState(float deltaTime);

    void setRole(FighterRole r);
    FighterRole getRole() const;

    void setCharacter(CharacterId id);
    CharacterId getCharacterId() const;
    const CharacterProfile& getProfile() const;
    float getSpecialEnergyCost() const;
    bool canChainGrab() const;

    void setDamageMultiplier(float mult);
    float getDamageMultiplier() const;

    void setColor(float r, float g, float b);
    void setAccentColor(float r, float g, float b);
    void setFacingRight(bool faceRight);
    float pushHorizontally(float dx);

    Rect getHurtbox() const;
    float getCenterX() const;
    float getDirection() const;
    bool isRolling() const;
    bool isElectrified() const;

    bool getHitbox(Rect& outBox) const;
    const AttackData* getCurrentAttack() const;
    void onAttackConnected();
    void receiveHit(const AttackData& attack, float pushDirection, int customDamage = -1,
                    float customHitStun = -1.0f, float knockbackScale = 1.0f);
    void applyElectrocution(float duration);

    // Multi-stage Special hooks driven from Game
    void setTargetPositionHint(float targetCenterX, float targetGroundY);
    std::vector<ProjectileSpawn> drainProjectileSpawns();
    void onFanProjectileHit();
    int  getFanSpecialStage() const;
    void advanceFanSpecialStage(int stage, float newVelocityY = 0.0f);
    bool consumeKneeImpactReady();
    void setActiveSkySpearsCount(int count);

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
    void setShowcasePose(FighterState s, float time) {
        state = s;
        stateTime = time;
    }
    void draw(const Renderer& renderer) const;

private:
    void keepInsideWorld();
    void startAttack(FighterState attackState);
    const AttackData* attackDataFor(FighterState s) const;

    CharacterId  characterId;
    FighterRole  role;
    float        damageMultiplier;

    float x;
    float y;
    float width;
    float height;
    float standingHeight;
    float crouchHeight;
    float moveSpeed;
    float jumpSpeed;
    float energyRegen;
    float velocityY;
    float airMomentumX;
    float knockbackVelocityX;
    float rollVelocityX;
    float rollTimer;
    float moveDirection;
    bool  facingRight;
    bool  onGround;
    bool  crouching;
    bool  blocking;
    bool  running;
    FighterState state;
    float stateTime;
    float hitStunDuration;
    float blockFlashTimer;
    float invulnerableTimer;
    float electrocuteTimer;
    float chainGrabCooldown;
    bool  comboQueued;

    // Special move sub-state (Archer backroll, Fan Dancer rush-knee, Spear Warrior 10-spears)
    bool  specialShotFired;
    int   fanSpecialStage; // 0 = throwing fans, 1 = sprinting to enemy, 2 = leap + head-grab knee
    float specialSubTimer;
    bool  fan1Fired;
    bool  fan2Fired;
    bool  kneeImpactPending;
    bool  kneeImpactDone;
    int   skySpearsSpawned;
    int   activeSkySpears;
    bool  finalSpearThrown;
    float targetHintX;
    float targetHintY;

    std::vector<ProjectileSpawn> pendingSpawns;

    float colorR, colorG, colorB;
    float accentR, accentG, accentB;
    int   health;
    int   maxHealth;
    float energy;
    float maxEnergy;
    bool  attackHasHit;
};

#endif
