#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createReed(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 13, 39, 22, 3, draw::color(2, 6, 23, 0.15F));
    draw::rect(x, size, 17, 14, 3, 25, draw::color(132, 204, 22));
    draw::rect(x, size, 25, 10, 3, 29, draw::color(101, 163, 13));
    draw::rect(x, size, 32, 16, 3, 23, draw::color(77, 124, 15));
    draw::rect(x, size, 23, 8, 7, 5, draw::color(202, 138, 4));
    return x;
}
} // namespace pf::art
