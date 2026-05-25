#include "ui/UiStyle.h"
#include <array>

namespace pf::ui {

ax::Color color(int r, int g, int b, float a) {
    return ax::Color(r / 255.0F, g / 255.0F, b / 255.0F, a);
}

namespace {
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

} // namespace pf::ui
