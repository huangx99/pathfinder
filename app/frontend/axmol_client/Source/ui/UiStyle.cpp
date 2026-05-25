#include "ui/UiStyle.h"
#include <array>

namespace pf::ui {

ax::Color color(int r, int g, int b, float a) {
    return ax::Color(r / 255.0F, g / 255.0F, b / 255.0F, a);
}

namespace {
void replaceAll(std::string& text, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    size_t start = 0;
    while ((start = text.find(from, start)) != std::string::npos) {
        text.replace(start, from.size(), to);
        start += to.size();
    }
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

ax::Label* label(const std::string& text, float size, const ax::Vec2& dimensions) {
    const auto path = fontPath();
    ax::Label* result = nullptr;
    if (!path.empty()) {
        result = ax::Label::createWithTTF(text, path, size, dimensions, ax::TextHAlignment::LEFT, ax::TextVAlignment::TOP);
    }
    if (!result) {
        result = ax::Label::createWithSystemFont(text, "Noto Sans CJK SC", size, dimensions, ax::TextHAlignment::LEFT, ax::TextVAlignment::TOP);
    }
    result->setTextColor(ax::Color32(226, 232, 240, 255));
    return result;
}

ax::Label* panelLabel(const std::string& text, float size, const ax::Vec2& dimensions) {
    auto* result = label(text, size, dimensions);
    result->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    return result;
}

ax::DrawNode* panelBackground(const ax::Size& size, const ax::Color& fill) {
    auto* draw = ax::DrawNode::create();
    draw->drawSolidRect(ax::Vec2::ZERO, ax::Vec2(size.width, size.height), fill);
    draw->drawRect(ax::Vec2::ZERO, ax::Vec2(size.width, size.height), color(71, 85, 105, 0.85F));
    return draw;
}

std::string shorten(const std::string& text, size_t max_chars) {
    size_t bytes = 0;
    size_t chars = 0;
    while (bytes < text.size() && chars < max_chars) {
        const unsigned char lead = static_cast<unsigned char>(text[bytes]);
        size_t step = 1;
        if ((lead & 0x80U) == 0U) step = 1;
        else if ((lead & 0xE0U) == 0xC0U) step = 2;
        else if ((lead & 0xF0U) == 0xE0U) step = 3;
        else if ((lead & 0xF8U) == 0xF0U) step = 4;
        if (bytes + step > text.size()) break;
        bytes += step;
        ++chars;
    }
    if (bytes >= text.size()) return text;
    return text.substr(0, bytes) + "...";
}

std::string humanizeDebugText(std::string text) {
    replaceAll(text, "decayed_red_berry", "腐烂红果");
    replaceAll(text, "red_berry", "红果");
    replaceAll(text, "bitter_leaf", "苦叶");
    replaceAll(text, "stone_flake", "石片");
    replaceAll(text, "no_visible_effect", "无明显效果");
    replaceAll(text, "restore_hunger", "缓解饥饿");
    replaceAll(text, "use_hint", "发现用途");
    replaceAll(text, "poison", "中毒");
    replaceAll(text, "candidate", "候选");
    replaceAll(text, "active", "已确认");
    replaceAll(text, "tool_not_supported_on_map", "当前工具不能用于地图");
    replaceAll(text, "out_of_bounds", "超出地图范围");
    replaceAll(text, "cell_not_walkable", "这里不能投放");
    replaceAll(text, "object_locked", "对象尚未解锁");
    replaceAll(text, "terrain_locked", "地形尚未解锁");
    replaceAll(text, "尝试use", "尝试使用");
    replaceAll(text, "尝试eat", "尝试吃");
    replaceAll(text, "执行 use", "执行 使用");
    replaceAll(text, "执行 eat", "执行 吃");
    replaceAll(text, " + use", " + 使用");
    replaceAll(text, " + eat", " + 吃");
    replaceAll(text, " -> ", " → ");
    replaceAll(text, " + ", " → ");
    return text;
}

} // namespace pf::ui
