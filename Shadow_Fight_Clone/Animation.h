#ifndef ANIMATION_H
#define ANIMATION_H

#include "Fighter.h"   // for FighterState

struct AttackData;     // forward declaration

// Comprehensive animation pose containing dynamic procedural curves for all limbs
struct Pose {
    float offsetY = 0.0f;   // vertical root translation (bounce/hop/collapse)
    float scaleX  = 1.0f;   // body squash & stretch X
    float scaleY  = 1.0f;   // body squash & stretch Y

    // Stride & Limb cycle angles/offsets for walking
    float strideLegF = 0.0f; // horizontal foot offset for front leg
    float strideLegB = 0.0f; // horizontal foot offset for back leg
    float liftLegF   = 0.0f; // vertical foot lift during swing phase
    float liftLegB   = 0.0f;

    float armSwingF  = 0.0f; // arm sway / swing offset
    float armSwingB  = 0.0f;

    // Breathing / ambient idle sway
    float breathY    = 0.0f;

    // Torso dynamic lean offset
    float torsoLeanX = 0.0f;

    // Normalized progress within current action phase (0.0 to 1.0)
    float actionProgress = 0.0f;

    // Hit impact recoil displacement
    float hitRecoilX = 0.0f;

    // Death collapse factor (0.0 = standing, 1.0 = collapsed flat on ground)
    float collapse = 0.0f;

    // Block snap transition (0.0 = relaxed, 1.0 = fully braced)
    float blockBrace = 0.0f;
};

namespace Animation {
    // Computes the dynamic animation pose based on state, elapsed state time,
    // current attack data (if attacking), and hit stun duration.
    Pose getPose(FighterState state, float stateTime,
                 const AttackData* attack = nullptr,
                 float hitStunDuration = 0.35f);
}

#endif
