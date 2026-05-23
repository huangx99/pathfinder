#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createTorch(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 21, 24, 6, 20, draw::color(69, 26, 3));
    draw::rect(x, size, 23, 24, 2, 20, draw::color(124, 45, 18));
    draw::rect(x, size, 18, 21, 12, 5, draw::color(120, 53, 15));
    draw::rect(x, size, 16, 15, 16, 10, draw::color(234, 88, 12));
    draw::rect(x, size, 18, 9, 12, 10, draw::color(250, 204, 21));
    draw::rect(x, size, 20, 5, 8, 8, draw::color(254, 240, 138));
    draw::rect(x, size, 22, 2, 4, 6, draw::color(254, 240, 138));
    return x;
}
} // namespace pf::art
