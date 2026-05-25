#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createCompanion(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 11, 41, 26, 4, draw::color(2, 6, 23, 0.20F));
    draw::rect(x, size, 16, 29, 7, 14, draw::color(71, 85, 105));
    draw::rect(x, size, 26, 29, 7, 14, draw::color(71, 85, 105));
    draw::rect(x, size, 13, 17, 23, 17, draw::color(22, 101, 52));
    draw::rect(x, size, 16, 19, 17, 12, draw::color(34, 197, 94));
    draw::rect(x, size, 14, 5, 20, 16, draw::color(231, 154, 106));
    draw::rect(x, size, 14, 4, 20, 6, draw::color(63, 32, 12));
    draw::rect(x, size, 19, 12, 3, 3, draw::color(15, 23, 42));
    draw::rect(x, size, 28, 12, 3, 3, draw::color(15, 23, 42));
    return x;
}

ax::Node* createWolf(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 9, 38, 30, 4, draw::color(2, 6, 23, 0.22F));
    draw::rect(x, size, 12, 22, 24, 12, draw::color(51, 65, 85));
    draw::rect(x, size, 30, 17, 10, 9, draw::color(71, 85, 105));
    draw::rect(x, size, 33, 12, 4, 6, draw::color(51, 65, 85));
    draw::rect(x, size, 17, 33, 4, 8, draw::color(30, 41, 59));
    draw::rect(x, size, 29, 33, 4, 8, draw::color(30, 41, 59));
    draw::rect(x, size, 35, 20, 2, 2, draw::color(239, 68, 68));
    return x;
}
} // namespace pf::art
