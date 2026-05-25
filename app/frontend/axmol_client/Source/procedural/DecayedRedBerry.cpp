#include "procedural/ProceduralArt.h"
#include "procedural/ProceduralDrawing.h"

namespace pf::art {
ax::Node* createDecayedRedBerry(float size) {
    auto* x = draw::canvas();
    draw::rect(x, size, 16, 21, 16, 17, draw::color(127, 29, 29));
    draw::rect(x, size, 13, 26, 22, 9, draw::color(153, 27, 27));
    draw::rect(x, size, 18, 18, 12, 6, draw::color(88, 28, 25));
    draw::rect(x, size, 20, 22, 5, 4, draw::color(74, 222, 128));
    draw::rect(x, size, 28, 28, 4, 3, draw::color(63, 98, 18));
    draw::rect(x, size, 23, 13, 4, 6, draw::color(63, 98, 18));
    draw::rect(x, size, 17, 38, 16, 3, draw::color(2, 6, 23, 0.18F));
    return x;
}
} // namespace pf::art
