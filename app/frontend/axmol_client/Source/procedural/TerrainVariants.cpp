#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createForestTile(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 0, 0, 48, 48, draw::color(34, 139, 75));
    draw::rect(x, size, 4, 36, 10, 4, draw::color(22, 101, 52));
    draw::rect(x, size, 23, 39, 8, 3, draw::color(74, 222, 128));
    draw::rect(x, size, 38, 33, 5, 4, draw::color(21, 128, 61));
    draw::rect(x, size, 10, 34, 6, 8, draw::color(22, 101, 52));
    draw::rect(x, size, 8, 29, 10, 6, draw::color(21, 128, 61));
    draw::rect(x, size, 31, 37, 5, 7, draw::color(20, 83, 45));
    draw::rect(x, size, 29, 32, 9, 5, draw::color(22, 101, 52));
    return x;
}

ax::Node* createRockyTile(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 0, 0, 48, 48, draw::color(120, 113, 108));
    draw::rect(x, size, 5, 37, 14, 4, draw::color(87, 83, 78));
    draw::rect(x, size, 24, 28, 8, 5, draw::color(168, 162, 158));
    draw::rect(x, size, 9, 34, 8, 5, draw::color(100, 116, 139));
    draw::rect(x, size, 24, 27, 10, 6, draw::color(71, 85, 105));
    draw::rect(x, size, 32, 39, 5, 3, draw::color(148, 163, 184));
    return x;
}
} // namespace pf::art
