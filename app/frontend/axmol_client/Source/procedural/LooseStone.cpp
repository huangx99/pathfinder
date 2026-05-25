#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createLooseStone(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 14, 36, 20, 4, draw::color(2, 6, 23, 0.16F));
    draw::rect(x, size, 15, 22, 18, 14, draw::color(100, 116, 139));
    draw::rect(x, size, 19, 17, 12, 7, draw::color(148, 163, 184));
    draw::rect(x, size, 13, 27, 5, 7, draw::color(71, 85, 105));
    draw::rect(x, size, 28, 25, 6, 8, draw::color(51, 65, 85));
    draw::rect(x, size, 20, 20, 5, 3, draw::color(226, 232, 240));
    return x;
}
} // namespace pf::art
