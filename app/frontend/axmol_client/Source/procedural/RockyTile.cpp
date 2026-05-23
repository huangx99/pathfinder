#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createRockyTile(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 0, 0, 48, 48, draw::color(45, 49, 57));
    draw::rect(x, size, 3, 3, 42, 42, draw::color(76, 82, 94));
    draw::rect(x, size, 9, 12, 15, 12, draw::color(107, 114, 128));
    draw::rect(x, size, 27, 30, 12, 9, draw::color(107, 114, 128));
    draw::rect(x, size, 6, 33, 9, 6, draw::color(82, 82, 91));
    draw::rect(x, size, 33, 9, 6, 6, draw::color(82, 82, 91));
    return x;
}
} // namespace pf::art
