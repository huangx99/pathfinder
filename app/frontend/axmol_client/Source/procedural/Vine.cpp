#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createVine(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 10, 38, 28, 3, draw::color(2, 6, 23, 0.14F));
    draw::rect(x, size, 14, 29, 6, 6, draw::color(21, 128, 61));
    draw::rect(x, size, 20, 23, 6, 6, draw::color(22, 163, 74));
    draw::rect(x, size, 26, 17, 6, 6, draw::color(21, 128, 61));
    draw::rect(x, size, 18, 18, 16, 4, draw::color(22, 101, 52));
    draw::rect(x, size, 12, 31, 16, 4, draw::color(22, 101, 52));
    return x;
}
} // namespace pf::art
