#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createWood(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 10, 39, 28, 4, draw::color(2, 6, 23, 0.18F));
    draw::rect(x, size, 9, 30, 28, 7, draw::color(69, 26, 3));
    draw::rect(x, size, 13, 25, 25, 7, draw::color(124, 45, 18));
    draw::rect(x, size, 7, 32, 8, 7, draw::color(202, 138, 4));
    draw::rect(x, size, 12, 27, 8, 7, draw::color(234, 179, 8));
    draw::rect(x, size, 16, 31, 18, 2, draw::color(146, 64, 14));
    draw::rect(x, size, 21, 28, 13, 2, draw::color(146, 64, 14));
    return x;
}
} // namespace pf::art
