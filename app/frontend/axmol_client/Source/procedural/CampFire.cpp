#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createCampFire(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 10, 34, 28, 5, draw::color(92, 64, 51));
    draw::rect(x, size, 14, 38, 20, 4, draw::color(120, 81, 54));
    draw::rect(x, size, 17, 29, 14, 7, draw::color(124, 45, 18));
    draw::rect(x, size, 15, 21, 18, 12, draw::color(234, 88, 12));
    draw::rect(x, size, 18, 14, 12, 13, draw::color(250, 204, 21));
    draw::rect(x, size, 21, 9, 6, 12, draw::color(254, 240, 138));
    draw::rect(x, size, 12, 40, 6, 4, draw::color(71, 85, 105));
    draw::rect(x, size, 30, 39, 6, 5, draw::color(71, 85, 105));
    return x;
}
} // namespace pf::art
