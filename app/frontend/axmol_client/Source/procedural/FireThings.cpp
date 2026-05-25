#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createTorch(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 23, 20, 4, 23, draw::color(92, 45, 15));
    draw::rect(x, size, 20, 11, 10, 10, draw::color(220, 38, 38));
    draw::rect(x, size, 22, 7, 6, 12, draw::color(249, 115, 22));
    draw::rect(x, size, 24, 10, 3, 8, draw::color(250, 204, 21));
    return x;
}

ax::Node* createCampFire(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 10, 36, 28, 5, draw::color(69, 26, 3));
    draw::rect(x, size, 16, 27, 16, 12, draw::color(220, 38, 38));
    draw::rect(x, size, 19, 18, 10, 18, draw::color(249, 115, 22));
    draw::rect(x, size, 22, 23, 5, 11, draw::color(250, 204, 21));
    return x;
}
} // namespace pf::art
