#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createMushroom(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 14, 38, 20, 3, draw::color(2, 6, 23, 0.14F));
    draw::rect(x, size, 21, 24, 8, 14, draw::color(254, 243, 199));
    draw::rect(x, size, 15, 18, 20, 9, draw::color(239, 68, 68));
    draw::rect(x, size, 18, 14, 14, 7, draw::color(248, 113, 113));
    draw::rect(x, size, 20, 18, 3, 3, draw::color(254, 242, 242));
    draw::rect(x, size, 28, 19, 3, 3, draw::color(254, 242, 242));
    return x;
}
} // namespace pf::art
