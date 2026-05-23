#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createWolf(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 10, 40, 28, 4, draw::color(0, 0, 0, 0.20F));
    draw::rect(x, size, 12, 22, 24, 14, draw::color(71, 85, 105));
    draw::rect(x, size, 8, 18, 14, 12, draw::color(71, 85, 105));
    draw::rect(x, size, 10, 20, 8, 10, draw::color(148, 163, 184));
    draw::rect(x, size, 14, 34, 5, 8, draw::color(51, 65, 85));
    draw::rect(x, size, 22, 34, 5, 8, draw::color(51, 65, 85));
    draw::rect(x, size, 29, 34, 5, 8, draw::color(51, 65, 85));
    draw::rect(x, size, 6, 10, 14, 12, draw::color(71, 85, 105));
    draw::rect(x, size, 4, 12, 6, 8, draw::color(71, 85, 105));
    draw::rect(x, size, 2, 14, 6, 6, draw::color(203, 213, 225));
    draw::rect(x, size, 1, 15, 3, 3, draw::color(15, 23, 42));
    draw::rect(x, size, 8, 14, 3, 3, draw::color(251, 191, 36));
    draw::rect(x, size, 9, 15, 1, 1, draw::color(15, 23, 42));
    draw::rect(x, size, 6, 6, 4, 6, draw::color(51, 65, 85));
    draw::rect(x, size, 12, 6, 4, 6, draw::color(51, 65, 85));
    draw::rect(x, size, 34, 16, 8, 4, draw::color(71, 85, 105));
    draw::rect(x, size, 40, 12, 4, 6, draw::color(71, 85, 105));
    return x;
}
} // namespace pf::art
