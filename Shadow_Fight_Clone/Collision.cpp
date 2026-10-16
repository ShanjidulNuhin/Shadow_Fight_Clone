#include "Collision.h"

namespace Collision {

bool overlaps(const Rect& a, const Rect& b) {
    // They overlap only if they overlap on the x axis AND on the y axis.
    bool overlapX = a.x < b.x + b.width  && a.x + a.width  > b.x;
    bool overlapY = a.y < b.y + b.height && a.y + a.height > b.y;
    return overlapX && overlapY;
}

}   // namespace Collision

