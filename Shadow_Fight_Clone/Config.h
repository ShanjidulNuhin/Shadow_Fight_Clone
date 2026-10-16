#ifndef CONFIG_H
#define CONFIG_H

// Game-wide constants shared by several files.
namespace Config {
    const int   WORLD_WIDTH   = 800;     // world units == window pixels for now
    const int   WORLD_HEIGHT  = 600;
    const float GROUND_HEIGHT = 100.0f;  // the floor covers y = 0 .. GROUND_HEIGHT

    // Physics (units: pixels and seconds; +y is UP, so gravity is negative)
    const float GRAVITY    = -2000.0f;   // pixels per second, per second
    const float JUMP_SPEED = 750.0f;     // upward speed at the moment of the jump
}

#endif
