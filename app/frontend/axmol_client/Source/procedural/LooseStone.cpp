#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createLooseStone(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 10, 39, 29, 4, draw::color(2, 6, 23, 0.22F));
    draw::rect(x, size, 15, 42, 19, 2, draw::color(2, 6, 23, 0.22F));
    draw::rect(x, size, 8, 31, 15, 8, draw::color(51, 65, 85));
    draw::rect(x, size, 12, 26, 8, 7, draw::color(51, 65, 85));
    draw::rect(x, size, 21, 29, 18, 11, draw::color(51, 65, 85));
    draw::rect(x, size, 27, 24, 8, 7, draw::color(51, 65, 85));
    draw::rect(x, size, 11, 29, 8, 4, draw::color(100, 116, 139));
    draw::rect(x, size, 23, 31, 10, 5, draw::color(100, 116, 139));
    draw::rect(x, size, 28, 26, 5, 3, draw::color(100, 116, 139));
    draw::rect(x, size, 13, 28, 4, 2, draw::color(203, 213, 225));
    draw::rect(x, size, 25, 31, 5, 2, draw::color(203, 213, 225));
    draw::rect(x, size, 29, 25, 3, 1, draw::color(203, 213, 225));
    draw::rect(x, size, 8, 37, 15, 3, draw::color(30, 41, 59));
    draw::rect(x, size, 21, 38, 18, 3, draw::color(30, 41, 59));
    draw::rect(x, size, 35, 32, 4, 6, draw::color(30, 41, 59));
    return x;
}
} // namespace pf::art
