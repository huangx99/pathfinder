#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createStonePile(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 7, 40, 34, 4, draw::color(2, 6, 23, 0.24F));
    draw::rect(x, size, 10, 34, 28, 7, draw::color(30, 41, 59));
    draw::rect(x, size, 7, 27, 17, 11, draw::color(71, 85, 105));
    draw::rect(x, size, 22, 24, 18, 14, draw::color(51, 65, 85));
    draw::rect(x, size, 15, 17, 18, 13, draw::color(100, 116, 139));
    draw::rect(x, size, 18, 19, 8, 4, draw::color(203, 213, 225));
    draw::rect(x, size, 25, 27, 9, 4, draw::color(148, 163, 184));
    draw::rect(x, size, 10, 29, 7, 3, draw::color(148, 163, 184));
    draw::rect(x, size, 31, 34, 6, 3, draw::color(15, 23, 42));
    draw::rect(x, size, 11, 36, 10, 3, draw::color(15, 23, 42));
    return x;
}
} // namespace pf::art
