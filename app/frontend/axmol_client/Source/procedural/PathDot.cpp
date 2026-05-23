#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createPathDot(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 19, 19, 10, 10, draw::color(56, 189, 248, 0.55F));
    draw::rect(x, size, 21, 21, 6, 6, draw::color(250, 204, 21, 0.95F));
    draw::rect(x, size, 23, 23, 2, 2, draw::color(224, 242, 254, 0.85F));
    return x;
}
} // namespace pf::art
