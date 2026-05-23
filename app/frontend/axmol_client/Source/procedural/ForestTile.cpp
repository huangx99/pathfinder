#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createForestTile(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 0, 0, 48, 48, draw::color(15, 46, 28));
    draw::rect(x, size, 6, 6, 6, 6, draw::color(20, 83, 45));
    draw::rect(x, size, 30, 18, 6, 6, draw::color(20, 83, 45));
    draw::rect(x, size, 18, 30, 3, 3, draw::color(6, 78, 59));
    draw::rect(x, size, 39, 9, 3, 3, draw::color(6, 78, 59));
    draw::rect(x, size, 12, 36, 6, 3, draw::color(22, 101, 52));
    draw::rect(x, size, 24, 12, 3, 3, draw::color(22, 101, 52));
    return x;
}
} // namespace pf::art
