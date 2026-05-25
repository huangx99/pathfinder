#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createFlint(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 14, 36, 20, 3, draw::color(2, 6, 23, 0.15F));
    draw::rect(x, size, 15, 24, 19, 10, draw::color(51, 65, 85));
    draw::rect(x, size, 20, 18, 13, 8, draw::color(100, 116, 139));
    draw::rect(x, size, 28, 20, 4, 4, draw::color(250, 204, 21));
    draw::rect(x, size, 32, 17, 3, 3, draw::color(251, 146, 60));
    return x;
}
} // namespace pf::art
