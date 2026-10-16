#ifndef COLLISION_H
#define COLLISION_H

// An axis-aligned rectangle (its sides are parallel to the screen edges).
// x, y is the BOTTOM-LEFT corner.
struct Rect {
    float x;
    float y;
    float width;
    float height;
};

namespace Collision {
    // Do the two rectangles overlap at all?
    bool overlaps(const Rect& a, const Rect& b);
}

#endif
