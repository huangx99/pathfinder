#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createTree(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 21, 25, 7, 18, draw::color(92, 45, 15));
    draw::rect(x, size, 15, 10, 18, 14, draw::color(22, 101, 52));
    draw::rect(x, size, 11, 18, 26, 12, draw::color(21, 128, 61));
    draw::rect(x, size, 18, 7, 12, 7, draw::color(34, 197, 94));
    return x;
}

ax::Node* createBerryBush(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 12, 24, 24, 14, draw::color(22, 101, 52));
    draw::rect(x, size, 9, 29, 30, 8, draw::color(21, 128, 61));
    draw::rect(x, size, 17, 25, 4, 4, draw::color(220, 38, 38));
    draw::rect(x, size, 27, 29, 4, 4, draw::color(239, 68, 68));
    return x;
}

ax::Node* createStonePile(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 10, 36, 28, 4, draw::color(2, 6, 23, 0.15F));
    draw::rect(x, size, 12, 27, 11, 9, draw::color(100, 116, 139));
    draw::rect(x, size, 22, 23, 13, 13, draw::color(71, 85, 105));
    draw::rect(x, size, 28, 18, 8, 7, draw::color(148, 163, 184));
    return x;
}
} // namespace pf::art
