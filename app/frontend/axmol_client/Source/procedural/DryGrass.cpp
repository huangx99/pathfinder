#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createDryGrass(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 12, 39, 24, 3, draw::color(2, 6, 23, 0.15F));
    draw::rect(x, size, 15, 20, 4, 18, draw::color(180, 83, 9));
    draw::rect(x, size, 22, 14, 4, 24, draw::color(202, 138, 4));
    draw::rect(x, size, 30, 18, 4, 20, draw::color(161, 98, 7));
    draw::rect(x, size, 18, 24, 16, 4, draw::color(234, 179, 8));
    return x;
}
} // namespace pf::art
