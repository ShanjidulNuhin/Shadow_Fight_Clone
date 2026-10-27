#include "Animation.h"
#include "AttackData.h"
#include <cmath>
#include <algorithm>

namespace Animation {

Pose getPose(FighterState state, float stateTime,
             const AttackData* attack,
             float hitStunDuration) {
    Pose pose;

    switch (state) {
        case FighterState::IDLE: {
            // Gentle rhythmic breathing: chest and arms rise and fall
            float breath = std::sin(stateTime * 2.8f);
            pose.scaleY = 1.0f + 0.025f * breath;
            pose.scaleX = 1.0f - 0.012f * breath;
            pose.breathY = breath * 1.5f;
            pose.armSwingF = breath * 1.2f;
            pose.armSwingB = breath * 0.8f;
            break;
        }

        case FighterState::WALK: {
            // Natural martial arts stride cycle (legs swing & lift, arms counterbalance)
            const float walkSpeed = 9.5f;
            float phase = stateTime * walkSpeed;
            float stride = std::sin(phase);

            // Subtle vertical hip bob
            pose.offsetY = std::fabs(std::sin(phase)) * 4.0f;
            pose.torsoLeanX = 2.5f; // athletic forward lean

            // Leg strides (opposite phases)
            pose.strideLegF = stride * 13.0f;
            pose.liftLegF   = std::max(0.0f, std::sin(phase)) * 7.0f;

            pose.strideLegB = -stride * 13.0f;
            pose.liftLegB   = std::max(0.0f, -std::sin(phase)) * 7.0f;

            // Arm counter-swing
            pose.armSwingF = -stride * 7.5f;
            pose.armSwingB =  stride * 6.5f;

            // Foot plant squash & stretch
            pose.scaleY = 1.0f - 0.025f * std::fabs(stride);
            pose.scaleX = 1.0f + 0.015f * std::fabs(stride);
            break;
        }

        case FighterState::JUMP: {
            pose.scaleX = 0.90f;
            pose.scaleY = 1.10f;
            pose.offsetY = 3.0f;
            break;
        }

        case FighterState::FALL: {
            pose.scaleX = 0.93f;
            pose.scaleY = 1.07f;
            pose.offsetY = 1.0f;
            break;
        }

        case FighterState::CROUCH: {
            float t = std::min(1.0f, stateTime / 0.08f);
            pose.scaleX = 1.0f + 0.08f * t;
            pose.scaleY = 1.0f - 0.08f * t;
            break;
        }

        case FighterState::BLOCK: {
            float t = std::min(1.0f, stateTime / 0.09f);
            pose.blockBrace = t * t * (3.0f - 2.0f * t); // smoothstep
            pose.torsoLeanX = -2.5f * pose.blockBrace;
            pose.scaleX = 0.96f;
            pose.scaleY = 0.98f;
            break;
        }

        case FighterState::PUNCH: {
            if (attack != nullptr) {
                if (stateTime < attack->startup) {
                    float t = stateTime / attack->startup;
                    pose.actionProgress = t;
                    pose.torsoLeanX = -2.5f * t; // coil back
                } else if (stateTime < attack->startup + attack->active) {
                    float t = (stateTime - attack->startup) / attack->active;
                    pose.actionProgress = t;
                    pose.torsoLeanX = 5.5f * std::sin(t * 1.570796f); // explosive drive
                } else {
                    float t = (stateTime - attack->startup - attack->active) / attack->recovery;
                    pose.actionProgress = std::min(1.0f, t);
                    pose.torsoLeanX = 5.5f * (1.0f - pose.actionProgress);
                }
            }
            break;
        }

        case FighterState::KICK: {
            if (attack != nullptr) {
                if (stateTime < attack->startup) {
                    float t = stateTime / attack->startup;
                    pose.actionProgress = t;
                    pose.torsoLeanX = -7.0f * t; // counterbalance lean
                } else if (stateTime < attack->startup + attack->active) {
                    float t = (stateTime - attack->startup) / attack->active;
                    pose.actionProgress = t;
                    pose.torsoLeanX = -11.0f;
                } else {
                    float t = (stateTime - attack->startup - attack->active) / attack->recovery;
                    pose.actionProgress = std::min(1.0f, t);
                    pose.torsoLeanX = -11.0f * (1.0f - pose.actionProgress);
                }
            }
            break;
        }

        case FighterState::SPECIAL: {
            if (attack != nullptr) {
                if (stateTime < attack->startup) {
                    float t = stateTime / attack->startup;
                    pose.actionProgress = t;
                    pose.scaleX = 1.0f + 0.05f * std::sin(stateTime * 45.0f);
                    pose.scaleY = 1.0f - 0.03f * std::sin(stateTime * 45.0f);
                    pose.torsoLeanX = -4.0f * t;
                } else if (stateTime < attack->startup + attack->active) {
                    float t = (stateTime - attack->startup) / attack->active;
                    pose.actionProgress = t;
                    pose.torsoLeanX = 11.0f * t; // lunging thrust
                } else {
                    float t = (stateTime - attack->startup - attack->active) / attack->recovery;
                    pose.actionProgress = std::min(1.0f, t);
                    pose.torsoLeanX = 11.0f * (1.0f - pose.actionProgress);
                }
            }
            break;
        }

        case FighterState::HIT: {
            float duration = (hitStunDuration > 0.001f) ? hitStunDuration : 0.35f;
            float t = std::min(1.0f, stateTime / duration);
            // Quick snap-back followed by damped harmonic oscillation
            float impact = (t < 0.22f) ? (t / 0.22f) : std::exp(-4.0f * (t - 0.22f)) * std::cos((t - 0.22f) * 12.0f);
            pose.hitRecoilX = -15.0f * impact;
            pose.torsoLeanX = -12.0f * impact;
            pose.scaleX = 1.0f + 0.10f * impact;
            pose.scaleY = 1.0f - 0.10f * impact;
            break;
        }

        case FighterState::DEAD: {
            // Smooth dynamic ragdoll-style collapse onto the arena floor
            float t = std::min(1.0f, stateTime / 0.55f);
            float ease = t * t * (3.0f - 2.0f * t); // smoothstep
            pose.collapse = ease;
            pose.offsetY = -6.0f * ease;
            pose.scaleX = 1.0f + 0.35f * ease;
            pose.scaleY = 1.0f - 0.30f * ease;
            break;
        }
    }

    return pose;
}

} // namespace Animation
