#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createClay(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 11, 36, 28, 4, draw::color(2, 6, 23, 0.14F));
    draw::rect(x, size, 13, 25, 24, 11, draw::color(146, 64, 14));
    draw::rect(x, size, 17, 19, 17, 8, draw::color(194, 65, 12));
    draw::rect(x, size, 21, 22, 7, 4, draw::color(251, 146, 60));
    return x;
}
} // namespace pf::art
