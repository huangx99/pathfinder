#pragma once

#include "axmol/axmol.h"

namespace pf::art::draw {

inline ax::Color color(int r, int g, int b, float a = 1.0F) {
    return ax::Color(r / 255.0F, g / 255.0F, b / 255.0F, a);
}

inline float px(float size, float unit) {
    return size * unit / 48.0F;
}

inline ax::Vec2 point(float size, float x, float y) {
    return ax::Vec2(px(size, x - 24.0F), px(size, 24.0F - y));
}

inline void rect(ax::DrawNode* draw, float size, float x, float y, float w, float h, const ax::Color& fill) {
    draw->drawSolidRect(point(size, x, y + h), point(size, x + w, y), fill);
}

inline ax::DrawNode* canvas() {
    return ax::DrawNode::create();
}

} // namespace pf::art::draw
