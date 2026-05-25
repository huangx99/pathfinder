#include "ui/EventLogPanel.h"
#include "ui/UiStyle.h"

namespace pf::ui {
namespace {
constexpr float kWidth = 980.0F;
constexpr float kHeight = 118.0F;
}

EventLogPanel* EventLogPanel::create() {
    auto* panel = new EventLogPanel();
    if (panel && panel->init()) {
        panel->autorelease();
        return panel;
    }
    delete panel;
    return nullptr;
}

bool EventLogPanel::init() {
    if (!Node::init()) return false;
    setContentSize(ax::Size(kWidth, kHeight));
    setEvents({}, "");
    return true;
}

void EventLogPanel::setEvents(const std::vector<std::string>& events, const std::string& error) {
    removeAllChildren();
    addChild(panelBackground(getContentSize(), color(8, 13, 26, 0.92F)), 0);
    auto* title = panelLabel(error.empty() ? "事件" : ("阻止：" + humanizeDebugText(error)), 16.0F, ax::Vec2(kWidth - 24.0F, 22.0F));
    title->setTextColor(error.empty() ? ax::Color32(147, 197, 253, 255) : ax::Color32(248, 113, 113, 255));
    title->setPosition(12.0F, kHeight - 12.0F);
    addChild(title, 2);

    int shown = 0;
    float y = kHeight - 38.0F;
    for (auto it = events.rbegin(); it != events.rend() && shown < 3; ++it, ++shown) {
        auto* line = panelLabel(shorten(humanizeDebugText(*it), 58), 14.0F, ax::Vec2(kWidth - 24.0F, 20.0F));
        line->setPosition(12.0F, y);
        addChild(line, 2);
        y -= 24.0F;
    }
}

} // namespace pf::ui
