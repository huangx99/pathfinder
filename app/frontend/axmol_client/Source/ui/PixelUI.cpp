#include "ui/PixelUI.h"
#include "ui/UiStyle.h"

namespace pf::ui {

// ── 内部辅助 ──────────────────────────────────────────────

namespace {

void drawPixelBorder(ax::DrawNode* draw, const ax::Vec2& origin, const ax::Size& size,
                     const ax::Color& light, const ax::Color& dark, float thickness) {
    const auto top_left     = origin + ax::Vec2(0.0F, size.height);
    const auto top_right    = origin + ax::Vec2(size.width, size.height);
    const auto bottom_right = origin + ax::Vec2(size.width, 0.0F);

    // 亮色：顶边 + 左边
    draw->drawSolidRect(ax::Vec2(origin.x, top_left.y - thickness),
                        ax::Vec2(top_right.x, top_left.y), light);
    draw->drawSolidRect(ax::Vec2(origin.x, origin.y),
                        ax::Vec2(origin.x + thickness, top_left.y), light);

    // 暗色：底边 + 右边
    draw->drawSolidRect(ax::Vec2(origin.x, origin.y),
                        ax::Vec2(bottom_right.x, origin.y + thickness), dark);
    draw->drawSolidRect(ax::Vec2(bottom_right.x - thickness, origin.y),
                        ax::Vec2(bottom_right.x, top_left.y), dark);
}

std::string fontPath() {
    static const std::array<std::string, 7> candidates{
        "fonts/NotoSansCJK-Regular.ttc",
        "fonts/NotoSansSC-Regular.otf",
        "fonts/SourceHanSansSC-Regular.otf",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/arphic/uming.ttc",
    };
    auto* files = ax::FileUtils::getInstance();
    for (const auto& path : candidates) {
        if (files->isFileExist(path)) return path;
    }
    return {};
}

} // namespace

// ── 面板背景 ──────────────────────────────────────────────

ax::DrawNode* pixelPanelBackground(const ax::Size& size,
                                    const ax::Color& border_light,
                                    const ax::Color& border_dark,
                                    const ax::Color& fill) {
    auto* draw = ax::DrawNode::create();

    // 内部填充（留 2px 给边框）
    draw->drawSolidRect(ax::Vec2(2.0F, 2.0F),
                        ax::Vec2(size.width - 2.0F, size.height - 2.0F), fill);

    // 像素边框（2px 粗）
    drawPixelBorder(draw, ax::Vec2::ZERO, size, border_light, border_dark, 2.0F);

    return draw;
}

// ── 按钮 ──────────────────────────────────────────────────

ax::Node* pixelButton(const ax::Size& size,
                       const std::string& text,
                       float font_size,
                       const ax::Color& bg_normal,
                       const ax::Color& bg_pressed) {
    auto* root = ax::Node::create();
    root->setContentSize(size);

    // 背景
    auto* bg = ax::DrawNode::create();
    bg->drawSolidRect(ax::Vec2::ZERO, ax::Vec2(size.width, size.height), bg_normal);
    drawPixelBorder(bg, ax::Vec2::ZERO, size,
                    pixelColor(220, 220, 220), pixelColor(60, 60, 60), 2.0F);
    bg->setName("bg");
    root->addChild(bg, 0);

    // 文字
    auto* label = pixelLabel(text, font_size,
                              ax::Vec2(size.width - 8.0F, size.height - 4.0F),
                              PixelPalette::TextPrimary);
    label->setAlignment(ax::TextHAlignment::CENTER, ax::TextVAlignment::CENTER);
    label->setPosition(size.width * 0.5F, size.height * 0.5F);
    label->setName("label");
    root->addChild(label, 1);

    return root;
}

// ── 进度条 ────────────────────────────────────────────────

ax::Node* pixelProgressBar(float width, float height,
                            float ratio,
                            const ax::Color& fill_color,
                            const ax::Color& bg_color,
                            const ax::Color& border_color) {
    auto* root = ax::Node::create();
    root->setContentSize(ax::Size(width, height));

    // 背景
    auto* bg = ax::DrawNode::create();
    bg->drawSolidRect(ax::Vec2::ZERO, ax::Vec2(width, height), bg_color);
    root->addChild(bg, 0);

    // 填充
    if (ratio > 0.0F) {
        auto* fill = ax::DrawNode::create();
        const float fill_w = std::max(1.0F, width * std::clamp(ratio, 0.0F, 1.0F));
        fill->drawSolidRect(ax::Vec2(2.0F, 2.0F),
                            ax::Vec2(fill_w - 2.0F, height - 2.0F), fill_color);
        fill->setName("fill");
        root->addChild(fill, 1);
    }

    // 边框
    auto* border = ax::DrawNode::create();
    drawPixelBorder(border, ax::Vec2::ZERO, ax::Size(width, height),
                    border_color, border_color, 2.0F);
    root->addChild(border, 2);

    return root;
}

// ── 背包格子 ──────────────────────────────────────────────

ax::Node* inventorySlot(float size,
                         bool selected,
                         const ax::Color& border,
                         const ax::Color& fill,
                         const ax::Color& selected_border) {
    auto* root = ax::Node::create();
    root->setContentSize(ax::Size(size, size));

    const auto& active_border = selected ? selected_border : border;

    // 填充
    auto* bg = ax::DrawNode::create();
    bg->drawSolidRect(ax::Vec2::ZERO, ax::Vec2(size, size), fill);
    root->addChild(bg, 0);

    // 边框（1px 粗，Minecraft 风格：左上亮，右下暗）
    auto* frame = ax::DrawNode::create();
    const auto light = selected ? pixelColor(255, 235, 100) : pixelColor(180, 180, 180);
    const auto dark  = selected ? pixelColor(180, 150,   0) : pixelColor( 60,  60,  60);
    drawPixelBorder(frame, ax::Vec2::ZERO, ax::Size(size, size), light, dark, 1.0F);
    root->addChild(frame, 1);

    return root;
}

// ── 心形图标 ──────────────────────────────────────────────

ax::Node* createHeartIcon(float size) {
    auto* draw = ax::DrawNode::create();
    const float s = size / 10.0F;

    const auto red   = pixelColor(255,  51,  51);
    const auto dark  = pixelColor(170,   0,   0);
    const auto shine = pixelColor(255, 150, 150);

    // 用 10×10 逻辑网格画像素心形
    //   11  11
    //  1111 1111
    //  1111111111
    //  1111111111
    //   11111111
    //    111111
    //     1111
    //      11
    struct Pixel { int x, y; };
    constexpr Pixel kHeartPixels[] = {
        {2,1},{3,1},{6,1},{7,1},
        {1,2},{2,2},{3,2},{4,2},{5,2},{6,2},{7,2},{8,2},
        {0,3},{1,3},{2,3},{3,3},{4,3},{5,3},{6,3},{7,3},{8,3},{9,3},
        {0,4},{1,4},{2,4},{3,4},{4,4},{5,4},{6,4},{7,4},{8,4},{9,4},
        {1,5},{2,5},{3,5},{4,5},{5,5},{6,5},{7,5},{8,5},
        {2,6},{3,6},{4,6},{5,6},{6,6},{7,6},
        {3,7},{4,7},{5,7},{6,7},
        {4,8},{5,8},
    };
    constexpr Pixel kShinePixels[] = {
        {2,3},{3,3},{1,4},{2,4},
    };

    for (const auto& p : kHeartPixels) {
        draw->drawSolidRect(ax::Vec2(p.x * s, (9 - p.y) * s),
                            ax::Vec2((p.x + 1) * s, (10 - p.y) * s), red);
    }
    for (const auto& p : kShinePixels) {
        draw->drawSolidRect(ax::Vec2(p.x * s, (9 - p.y) * s),
                            ax::Vec2((p.x + 1) * s, (10 - p.y) * s), shine);
    }
    // 外轮廓 1px 深色
    draw->drawRect(ax::Vec2(0.0F, 0.0F), ax::Vec2(size, size), dark);

    return draw;
}

// ── 鸡腿/饥饿图标 ─────────────────────────────────────────

ax::Node* createHungerIcon(float size) {
    auto* draw = ax::DrawNode::create();
    const float s = size / 10.0F;

    const auto meat  = pixelColor(210, 105,  30);
    const auto bone  = pixelColor(240, 240, 230);
    const auto dark  = pixelColor(139,  69,  19);

    struct P2 { int x, y; };
    const P2 kMeatPixels[] = {
        {2,2},{3,2},{4,2},{5,2},{6,2},
        {1,3},{2,3},{3,3},{4,3},{5,3},{6,3},
        {1,4},{2,4},{3,4},{4,4},{5,4},{6,4},
        {2,5},{3,5},{4,5},{5,5},
        {3,6},{4,6},
    };
    const P2 kBonePixels[] = {
        {7,3},{8,3},{9,3},
        {7,4},{8,4},{9,4},
    };

    for (const auto& p : kMeatPixels) {
        draw->drawSolidRect(ax::Vec2(p.x * s, (9 - p.y) * s),
                            ax::Vec2((p.x + 1) * s, (10 - p.y) * s), meat);
    }
    for (const auto& p : kBonePixels) {
        draw->drawSolidRect(ax::Vec2(p.x * s, (9 - p.y) * s),
                            ax::Vec2((p.x + 1) * s, (10 - p.y) * s), bone);
    }
    draw->drawRect(ax::Vec2(0.0F, 0.0F), ax::Vec2(size, size), dark);

    return draw;
}

// ── 悬浮提示 ──────────────────────────────────────────────

ax::Node* createTooltip(const ax::Size& size,
                         const std::string& title,
                         const std::vector<std::string>& lines,
                         float font_size) {
    auto* root = ax::Node::create();
    root->setContentSize(size);

    // 背景 + 边框
    auto* bg = pixelPanelBackground(size, PixelPalette::TooltipBorder,
                                     pixelColor(40, 0, 120), PixelPalette::TooltipBg);
    root->addChild(bg, 0);

    // 标题
    auto* title_label = pixelLabel(title, font_size + 2.0F,
                                    ax::Vec2(size.width - 12.0F, 20.0F),
                                    pixelColor(255, 215, 0));
    title_label->setPosition(6.0F, size.height - 6.0F);
    root->addChild(title_label, 1);

    // 内容行
    float y = size.height - 28.0F;
    for (const auto& line : lines) {
        auto* line_label = pixelLabel(line, font_size,
                                       ax::Vec2(size.width - 12.0F, 18.0F),
                                       PixelPalette::TextPrimary);
        line_label->setPosition(6.0F, y);
        root->addChild(line_label, 1);
        y -= 18.0F;
        if (y < 6.0F) break;
    }

    return root;
}

// ── 文字标签 ──────────────────────────────────────────────

ax::Label* pixelLabel(const std::string& text, float size,
                       const ax::Vec2& dimensions,
                       const ax::Color& color) {
    const auto path = fontPath();
    ax::Label* result = nullptr;
    if (!path.empty()) {
        result = ax::Label::createWithTTF(text, path, size, dimensions,
                                          ax::TextHAlignment::LEFT, ax::TextVAlignment::TOP);
    }
    if (!result) {
        result = ax::Label::createWithSystemFont(text, "Noto Sans CJK SC", size, dimensions,
                                                  ax::TextHAlignment::LEFT, ax::TextVAlignment::TOP);
    }
    if (result) {
        result->setTextColor(ax::Color32(
            static_cast<uint8_t>(color.r * 255),
            static_cast<uint8_t>(color.g * 255),
            static_cast<uint8_t>(color.b * 255),
            static_cast<uint8_t>(color.a * 255)));
    }
    return result;
}

ax::Label* pixelPanelLabel(const std::string& text, float size,
                            const ax::Vec2& dimensions,
                            const ax::Color& color) {
    auto* result = pixelLabel(text, size, dimensions, color);
    if (result) result->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    return result;
}

} // namespace pf::ui
