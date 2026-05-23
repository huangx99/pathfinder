#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createCompanion(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 12, 42, 24, 4, draw::color(0, 0, 0, 0.20F));
    draw::rect(x, size, 14, 18, 20, 22, draw::color(124, 45, 18));
    draw::rect(x, size, 12, 20, 4, 18, draw::color(124, 45, 18));
    draw::rect(x, size, 32, 20, 4, 18, draw::color(124, 45, 18));
    draw::rect(x, size, 14, 6, 20, 14, draw::color(154, 52, 18));
    draw::rect(x, size, 12, 10, 4, 10, draw::color(154, 52, 18));
    draw::rect(x, size, 32, 10, 4, 10, draw::color(154, 52, 18));
    draw::rect(x, size, 18, 12, 12, 10, draw::color(253, 186, 116));
    draw::rect(x, size, 20, 15, 3, 3, draw::color(30, 41, 59));
    draw::rect(x, size, 27, 15, 3, 3, draw::color(30, 41, 59));
    draw::rect(x, size, 14, 28, 20, 3, draw::color(251, 191, 36));
    draw::rect(x, size, 10, 24, 4, 4, draw::color(253, 186, 116));
    draw::rect(x, size, 34, 24, 4, 4, draw::color(253, 186, 116));
    return x;
}
} // namespace pf::art
