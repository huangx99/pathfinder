#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createAxe(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 22, 13, 5, 30, draw::color(92, 64, 36));
    draw::rect(x, size, 24, 13, 2, 29, draw::color(146, 64, 14));
    draw::rect(x, size, 14, 11, 19, 7, draw::color(71, 85, 105));
    draw::rect(x, size, 10, 16, 25, 9, draw::color(100, 116, 139));
    draw::rect(x, size, 14, 24, 16, 5, draw::color(71, 85, 105));
    draw::rect(x, size, 11, 18, 5, 5, draw::color(203, 213, 225));
    draw::rect(x, size, 17, 13, 12, 3, draw::color(226, 232, 240));
    draw::rect(x, size, 29, 18, 5, 4, draw::color(30, 41, 59));
    draw::rect(x, size, 19, 27, 5, 3, draw::color(30, 41, 59));
    draw::rect(x, size, 19, 38, 11, 4, draw::color(2, 6, 23, 0.18F));
    return x;
}
} // namespace pf::art
