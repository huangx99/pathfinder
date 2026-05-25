#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createBitterLeaf(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 14, 39, 20, 3, draw::color(2, 6, 23, 0.18F));
    draw::rect(x, size, 22, 12, 5, 25, draw::color(21, 128, 61));
    draw::rect(x, size, 16, 18, 12, 15, draw::color(22, 163, 74));
    draw::rect(x, size, 25, 19, 9, 14, draw::color(20, 83, 45));
    draw::rect(x, size, 21, 16, 3, 18, draw::color(187, 247, 208));
    draw::rect(x, size, 28, 23, 3, 3, draw::color(132, 204, 22));
    return x;
}
} // namespace pf::art
