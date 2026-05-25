#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createStoneFlake(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 15, 40, 20, 3, draw::color(2, 6, 23, 0.20F));
    draw::rect(x, size, 23, 12, 5, 25, draw::color(71, 85, 105));
    draw::rect(x, size, 18, 18, 13, 19, draw::color(71, 85, 105));
    draw::rect(x, size, 14, 25, 17, 12, draw::color(71, 85, 105));
    draw::rect(x, size, 25, 14, 3, 20, draw::color(148, 163, 184));
    draw::rect(x, size, 20, 20, 5, 12, draw::color(100, 116, 139));
    draw::rect(x, size, 15, 28, 7, 6, draw::color(51, 65, 85));
    draw::rect(x, size, 28, 22, 3, 10, draw::color(30, 41, 59));
    draw::rect(x, size, 25, 15, 2, 7, draw::color(226, 232, 240));
    draw::rect(x, size, 21, 21, 2, 5, draw::color(203, 213, 225));
    return x;
}
} // namespace pf::art
