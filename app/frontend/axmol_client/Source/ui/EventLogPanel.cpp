#include "ui/EventLogPanel.h"
#include "ui/UiStyle.h"

#include "axmol/ui/UIScrollView.h"

namespace pf::ui {
namespace {
constexpr float kWidth = 980.0F;
constexpr float kHeight = 118.0F;
constexpr float kInnerHeight = 360.0F;
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

    auto* scroll = ax::ui::ScrollView::create();
    scroll->setContentSize(ax::Size(kWidth - 24.0F, kHeight - 40.0F));
    scroll->setInnerContainerSize(ax::Size(kWidth - 24.0F, kInnerHeight));
    scroll->setDirection(ax::ui::ScrollView::Direction::VERTICAL);
    scroll->setBounceEnabled(true);
    scroll->setScrollBarEnabled(true);
    scroll->setScrollBarWidth(4.0F);
    scroll->setScrollBarColor(ax::Color32(148, 163, 184, 255));
    scroll->setScrollBarOpacity(170);
    scroll->setPosition(ax::Vec2(12.0F, 8.0F));
    addChild(scroll, 2);

    float y = kInnerHeight - 8.0F;
    for (auto it = events.rbegin(); it != events.rend(); ++it) {
        auto* line = panelLabel(shorten(humanizeDebugText(*it), 62), 14.0F, ax::Vec2(kWidth - 36.0F, 22.0F));
        line->setPosition(0.0F, y);
        scroll->addChild(line, 2);
        y -= 24.0F;
        if (y < 12.0F) break;
    }
}

} // namespace pf::ui
