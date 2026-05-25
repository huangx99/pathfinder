#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createSoilTile(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 0, 0, 48, 48, draw::color(92, 57, 33));
    draw::rect(x, size, 6, 10, 8, 5, draw::color(120, 76, 42));
    draw::rect(x, size, 25, 30, 10, 4, draw::color(68, 44, 30));
    draw::rect(x, size, 38, 16, 4, 4, draw::color(146, 92, 48));
    return x;
}
} // namespace pf::art
