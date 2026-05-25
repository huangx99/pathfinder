#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createRedBerry(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 16, 20, 16, 18, draw::color(185, 28, 28));
    draw::rect(x, size, 13, 25, 22, 10, draw::color(220, 38, 38));
    draw::rect(x, size, 18, 17, 12, 7, draw::color(153, 27, 27));
    draw::rect(x, size, 20, 18, 5, 4, draw::color(254, 202, 202));
    draw::rect(x, size, 23, 12, 4, 7, draw::color(22, 101, 52));
    draw::rect(x, size, 18, 13, 6, 3, draw::color(34, 197, 94));
    draw::rect(x, size, 25, 13, 6, 3, draw::color(34, 197, 94));
    draw::rect(x, size, 17, 38, 16, 3, draw::color(2, 6, 23, 0.18F));
    return x;
}
} // namespace pf::art
