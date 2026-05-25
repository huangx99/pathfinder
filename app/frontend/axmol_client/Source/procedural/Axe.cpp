#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createAxe(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 22, 12, 4, 30, draw::color(92, 45, 15));
    draw::rect(x, size, 19, 10, 10, 5, draw::color(120, 53, 15));
    draw::rect(x, size, 25, 13, 12, 10, draw::color(148, 163, 184));
    draw::rect(x, size, 31, 17, 6, 5, draw::color(226, 232, 240));
    draw::rect(x, size, 18, 13, 5, 8, draw::color(71, 85, 105));
    return x;
}
} // namespace pf::art
