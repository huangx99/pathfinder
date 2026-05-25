#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createWaterTile(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 0, 0, 48, 48, draw::color(26, 45, 74));
    draw::rect(x, size, 6, 9, 12, 3, draw::color(37, 99, 235));
    draw::rect(x, size, 24, 21, 15, 3, draw::color(37, 99, 235));
    draw::rect(x, size, 12, 33, 9, 3, draw::color(37, 99, 235));
    draw::rect(x, size, 3, 24, 6, 6, draw::color(30, 64, 175));
    draw::rect(x, size, 36, 12, 6, 6, draw::color(30, 64, 175));
    draw::rect(x, size, 18, 42, 9, 3, draw::color(59, 130, 246));
    draw::rect(x, size, 30, 6, 6, 3, draw::color(59, 130, 246));
    return x;
}
} // namespace pf::art
