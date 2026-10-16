#ifndef ANIMATION_H
#define ANIMATION_H

#include "Fighter.h"   // for FighterState

// How the body should look RIGHT NOW. This is purely visual: it never
// changes the fighter's real position or size used for physics.
struct Pose {
    float offsetY = 0.0f;   // lift the body up (pixels)
    float scaleX  = 1.0f;   // 1.0 = normal width
    float scaleY  = 1.0f;   // 1.0 = normal height
};

namespace Animation {
    // stateTime = seconds since the fighter entered its current state
    Pose getPose(FighterState state, float stateTime);
}

#endif
