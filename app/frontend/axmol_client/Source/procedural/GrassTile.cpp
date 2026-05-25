#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createGrassTile(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 0, 0, 48, 48, draw::color(30, 58, 30));
    draw::rect(x, size, 3, 12, 6, 6, draw::color(20, 83, 45));
    draw::rect(x, size, 24, 36, 6, 6, draw::color(20, 83, 45));
    draw::rect(x, size, 36, 9, 3, 3, draw::color(6, 78, 59));
    draw::rect(x, size, 15, 27, 3, 3, draw::color(6, 78, 59));
    return x;
}
} // namespace pf::art
