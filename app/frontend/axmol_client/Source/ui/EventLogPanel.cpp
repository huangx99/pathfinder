#include "ui/EventLogPanel.h"
#include "ui/PixelUI.h"
#include "ui/UiStyle.h"

namespace pf::ui {
namespace {

constexpr float kWidth = 940.0F;
constexpr float kHeight = 64.0F;

std::string eventPrefix(const std::string& text) {
    if (text.find("阻止") == 0 || text.find("超出") == 0 ||
        text.find("不能") == 0 || text.find("尚未") == 0) {
        return "⚠️ ";
    }
    return "💬 ";
}

} // namespace

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

    bg_ = pixelPanelBackground(getContentSize());
    addChild(bg_, 0);

    line1_ = pixelPanelLabel("", 12.0F, ax::Vec2(kWidth - 20.0F, 20.0F), PixelPalette::TextSecondary);
    line1_->setPosition(10.0F, kHeight - 12.0F);
    addChild(line1_, 1);

    line2_ = pixelPanelLabel("", 12.0F, ax::Vec2(kWidth - 20.0F, 20.0F), PixelPalette::TextSecondary);
    line2_->setPosition(10.0F, kHeight - 36.0F);
    addChild(line2_, 1);

    return true;
}

void EventLogPanel::setEvents(const std::vector<std::string>& events, const std::string& error) {
    if (!error.empty()) {
        auto text = humanizeDebugText(error);
        line1_->setString("⚠️ " + text);
        line1_->setTextColor(ax::Color32(248, 113, 113));
        line2_->setString("");
        return;
    }

    line1_->setTextColor(ax::Color32(226, 232, 240));
    line2_->setTextColor(ax::Color32(170, 170, 170));

    if (events.empty()) {
        line1_->setString("");
        line2_->setString("");
        return;
    }

    auto it = events.rbegin();
    line1_->setString(eventPrefix(humanizeDebugText(*it)) + humanizeDebugText(*it));
    ++it;
    if (it != events.rend()) {
        line2_->setString(eventPrefix(humanizeDebugText(*it)) + humanizeDebugText(*it));
    } else {
        line2_->setString("");
    }
}

} // namespace pf::ui
