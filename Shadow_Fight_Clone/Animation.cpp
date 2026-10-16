#include "Animation.h"
#include <cmath>

namespace Animation {

Pose getPose(FighterState state, float stateTime) {
    Pose pose;   // starts as "no change"

    switch (state) {
        case FighterState::IDLE: {
            // gentle breathing: a little taller, a little thinner, and back
            float breath = std::sin(stateTime * 3.0f);
            pose.scaleY = 1.0f + 0.03f * breath;
            pose.scaleX = 1.0f - 0.015f * breath;
            break;
        }
        case FighterState::WALK:
            // little hops while walking
            pose.offsetY = std::fabs(std::sin(stateTime * 10.0f)) * 5.0f;
            break;

        case FighterState::JUMP:
            pose.scaleX = 0.90f;
            pose.scaleY = 1.10f;
            break;

        case FighterState::FALL:
            pose.scaleX = 0.92f;
            pose.scaleY = 1.08f;
            break;

        case FighterState::CROUCH:
            pose.scaleX = 1.10f;
            break;

        case FighterState::PUNCH:
            break;   // the punching arm is drawn by Fighter::draw; the body stays normal
    }

    return pose;
}

}   // namespace Animation
