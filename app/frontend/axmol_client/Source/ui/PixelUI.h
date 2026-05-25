#pragma once

#include "axmol/axmol.h"
#include <functional>
#include <string>
#include <vector>

namespace pf::ui {

// ── 颜色常量 ──────────────────────────────────────────────

inline ax::Color pixelColor(int r, int g, int b, float a = 1.0F) {
    return ax::Color(r / 255.0F, g / 255.0F, b / 255.0F, a);
}

// 像素风标准色
namespace PixelPalette {
inline const ax::Color PanelBorderLight   = pixelColor(198, 198, 198);  // #C6C6C6
inline const ax::Color PanelBorderDark    = pixelColor( 85,  85,  85);  // #555555
inline const ax::Color PanelFill          = pixelColor( 43,  43,  43);  // #2B2B2B
inline const ax::Color ButtonNormal       = pixelColor(139, 139, 139);  // #8B8B8B
inline const ax::Color ButtonPressed      = pixelColor(107, 107, 107);  // #6B6B6B
inline const ax::Color SlotBorder         = pixelColor(139, 139, 139);  // #8B8B8B
inline const ax::Color SlotFill           = pixelColor( 59,  59,  59);  // #3B3B3B
inline const ax::Color SlotSelected       = pixelColor(255, 215,   0);  // #FFD700
inline const ax::Color HealthFill         = pixelColor(255,  51,  51);  // #FF3333
inline const ax::Color HealthBorder       = pixelColor(170,   0,   0);  // #AA0000
inline const ax::Color HungerFill         = pixelColor(255, 170,   0);  // #FFAA00
inline const ax::Color HungerBorder       = pixelColor(170, 102,   0);  // #AA6600
inline const ax::Color KnowledgeActive    = pixelColor( 74, 222, 128);  // #4ADE80
inline const ax::Color KnowledgeHypo      = pixelColor(250, 204,  21);  // #FACC15
inline const ax::Color KnowledgeCandidate = pixelColor(148, 163, 184);  // #94A3B8
inline const ax::Color TextPrimary        = pixelColor(255, 255, 255);  // #FFFFFF
inline const ax::Color TextSecondary      = pixelColor(170, 170, 170);  // #AAAAAA
inline const ax::Color TooltipBg          = pixelColor( 16,   0,  16, 0.94F);
inline const ax::Color TooltipBorder      = pixelColor( 80,   0, 255);
} // namespace PixelPalette

// ── 面板 ──────────────────────────────────────────────────

ax::DrawNode* pixelPanelBackground(const ax::Size& size,
                                    const ax::Color& border_light = PixelPalette::PanelBorderLight,
                                    const ax::Color& border_dark  = PixelPalette::PanelBorderDark,
                                    const ax::Color& fill         = PixelPalette::PanelFill);

// ── 按钮 ──────────────────────────────────────────────────

// 创建一个像素风按钮节点。返回的根节点包含背景和文字。
// 调用方需要自行包装成 MenuItem 或直接添加触摸监听。
ax::Node* pixelButton(const ax::Size& size,
                       const std::string& text,
                       float font_size = 14.0F,
                       const ax::Color& bg_normal  = PixelPalette::ButtonNormal,
                       const ax::Color& bg_pressed = PixelPalette::ButtonPressed);

// ── 进度条 ────────────────────────────────────────────────

ax::Node* pixelProgressBar(float width, float height,
                            float ratio,
                            const ax::Color& fill_color  = PixelPalette::HealthFill,
                            const ax::Color& bg_color    = PixelPalette::PanelFill,
                            const ax::Color& border_color = PixelPalette::PanelBorderDark);

// ── 背包格子 ──────────────────────────────────────────────

ax::Node* inventorySlot(float size,
                         bool selected = false,
                         const ax::Color& border = PixelPalette::SlotBorder,
                         const ax::Color& fill   = PixelPalette::SlotFill,
                         const ax::Color& selected_border = PixelPalette::SlotSelected);

// ── 图标 ──────────────────────────────────────────────────

ax::Node* createHeartIcon(float size);
ax::Node* createHungerIcon(float size);

// ── 悬浮提示 ──────────────────────────────────────────────

ax::Node* createTooltip(const ax::Size& size,
                         const std::string& title,
                         const std::vector<std::string>& lines,
                         float font_size = 12.0F);

// ── 辅助文字 ──────────────────────────────────────────────

ax::Label* pixelLabel(const std::string& text, float size,
                       const ax::Vec2& dimensions = ax::Vec2::ZERO,
                       const ax::Color& color = PixelPalette::TextPrimary);

ax::Label* pixelPanelLabel(const std::string& text, float size,
                            const ax::Vec2& dimensions = ax::Vec2::ZERO,
                            const ax::Color& color = PixelPalette::TextPrimary);

} // namespace pf::ui
