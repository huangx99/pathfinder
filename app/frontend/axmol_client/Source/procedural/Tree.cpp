#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createTree(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 20, 28, 8, 18, draw::color(63, 46, 24));
    draw::rect(x, size, 21, 28, 2, 18, draw::color(92, 64, 36));
    draw::rect(x, size, 16, 42, 6, 3, draw::color(63, 46, 24));
    draw::rect(x, size, 26, 42, 6, 3, draw::color(63, 46, 24));
    draw::rect(x, size, 8, 22, 32, 10, draw::color(6, 78, 59));
    draw::rect(x, size, 10, 14, 28, 10, draw::color(6, 78, 59));
    draw::rect(x, size, 14, 6, 20, 10, draw::color(6, 78, 59));
    draw::rect(x, size, 10, 24, 8, 6, draw::color(4, 120, 87));
    draw::rect(x, size, 12, 16, 8, 6, draw::color(4, 120, 87));
    draw::rect(x, size, 16, 8, 6, 6, draw::color(4, 120, 87));
    draw::rect(x, size, 28, 24, 8, 6, draw::color(2, 44, 34));
    draw::rect(x, size, 26, 16, 8, 6, draw::color(2, 44, 34));
    return x;
}
} // namespace pf::art
