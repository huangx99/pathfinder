#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createPoisonMushroom(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 14, 38, 20, 3, draw::color(2, 6, 23, 0.14F));
    draw::rect(x, size, 21, 24, 8, 14, draw::color(220, 252, 231));
    draw::rect(x, size, 15, 18, 20, 9, draw::color(126, 34, 206));
    draw::rect(x, size, 18, 14, 14, 7, draw::color(168, 85, 247));
    draw::rect(x, size, 20, 18, 3, 3, draw::color(134, 239, 172));
    draw::rect(x, size, 28, 19, 3, 3, draw::color(134, 239, 172));
    return x;
}
} // namespace pf::art
