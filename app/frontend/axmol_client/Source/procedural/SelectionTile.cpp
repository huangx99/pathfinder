#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createSelectionTile(float size) {
    auto* x = draw::canvas();
    const auto c = draw::color(56, 189, 248, 0.32F);
    draw::rect(x, size, 5, 5, 38, 3, c);
    draw::rect(x, size, 5, 40, 38, 3, c);
    draw::rect(x, size, 5, 5, 3, 38, c);
    draw::rect(x, size, 40, 5, 3, 38, c);
    return x;
}
} // namespace pf::art
