#include "ui/InspectorPanel.h"
#include "ui/UiStyle.h"

namespace pf::ui {
namespace {
constexpr float kWidth = 280.0F;
constexpr float kHeight = 420.0F;
}

InspectorPanel* InspectorPanel::create() {
    auto* panel = new InspectorPanel();
    if (panel && panel->init()) {
        panel->autorelease();
        return panel;
    }
    delete panel;
    return nullptr;
}

bool InspectorPanel::init() {
    if (!Node::init()) return false;
    setContentSize(ax::Size(kWidth, kHeight));
    setLines("观察", {"点击地图格子投放当前工具。", "点击小人可查看知识。"});
    return true;
}

void InspectorPanel::setLines(const std::string& title_text, const std::vector<std::string>& lines) {
    removeAllChildren();
    addChild(panelBackground(getContentSize()), 0);
    auto* title = label(title_text, 22.0F, ax::Vec2(kWidth - 24.0F, 30.0F));
    title->setPosition(12.0F, kHeight - 16.0F);
    addChild(title, 2);

    float y = kHeight - 62.0F;
    for (const auto& line : lines) {
        auto* text = label(shorten(line, 40), 15.0F, ax::Vec2(kWidth - 24.0F, 42.0F));
        text->setPosition(12.0F, y);
        addChild(text, 2);
        y -= 42.0F;
        if (y < 16.0F) break;
    }
}

} // namespace pf::ui
