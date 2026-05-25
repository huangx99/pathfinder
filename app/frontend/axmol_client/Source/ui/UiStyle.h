#pragma once

#include "axmol/axmol.h"
#include <string>

namespace pf::ui {

ax::Color color(int r, int g, int b, float a = 1.0F);
ax::Label* label(const std::string& text, float size, const ax::Vec2& dimensions = ax::Vec2::ZERO);
ax::DrawNode* panelBackground(const ax::Size& size, const ax::Color& fill = color(12, 24, 38, 0.92F));
std::string shorten(const std::string& text, size_t max_chars);

} // namespace pf::ui
