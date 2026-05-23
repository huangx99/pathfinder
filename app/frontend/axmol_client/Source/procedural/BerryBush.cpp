#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createBerryBush(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 8, 16, 32, 28, draw::color(20, 83, 45));
    draw::rect(x, size, 12, 8, 24, 10, draw::color(20, 83, 45));
    draw::rect(x, size, 10, 18, 10, 10, draw::color(34, 197, 94));
    draw::rect(x, size, 14, 10, 8, 6, draw::color(34, 197, 94));
    draw::rect(x, size, 28, 28, 10, 14, draw::color(5, 46, 22));
    draw::rect(x, size, 12, 20, 4, 4, draw::color(220, 38, 38));
    draw::rect(x, size, 22, 14, 4, 4, draw::color(220, 38, 38));
    draw::rect(x, size, 30, 24, 4, 4, draw::color(220, 38, 38));
    draw::rect(x, size, 16, 32, 4, 4, draw::color(220, 38, 38));
    draw::rect(x, size, 28, 36, 4, 4, draw::color(220, 38, 38));
    draw::rect(x, size, 20, 26, 4, 4, draw::color(220, 38, 38));
    draw::rect(x, size, 13, 21, 2, 2, draw::color(252, 165, 165));
    draw::rect(x, size, 23, 15, 2, 2, draw::color(252, 165, 165));
    draw::rect(x, size, 31, 25, 2, 2, draw::color(252, 165, 165));
    return x;
}
} // namespace pf::art
