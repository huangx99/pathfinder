#include "ui/EventLogPanel.h"
#include "ui/PixelUI.h"
#include "ui/UiStyle.h"

#include "axmol/ui/UILayout.h"
#include "axmol/ui/UILayoutParameter.h"

namespace pf::ui {
namespace {

constexpr float kWidth = 940.0F;
constexpr float kHeight = 64.0F;


ax::ui::LinearLayoutParameter* linearParam(float top, float bottom) {
    auto* param = ax::ui::LinearLayoutParameter::create();
    param->setGravity(ax::ui::LinearLayoutParameter::LinearGravity::LEFT);
    param->setMargin(ax::ui::Margin(0.0F, top, 0.0F, bottom));
    return param;
}

ax::ui::Layout* makeLayoutCell(const ax::Size& size, ax::Node* child) {
    auto* cell = ax::ui::Layout::create();
    cell->setContentSize(size);
    cell->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    if (child) cell->addChild(child, 1);
    return cell;
}

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

    auto* root = ax::ui::Layout::create();
    root->setContentSize(ax::Size(kWidth - 20.0F, kHeight - 16.0F));
    root->setLayoutType(ax::ui::Layout::Type::VERTICAL);
    root->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    root->setPosition(ax::Vec2(10.0F, kHeight - 8.0F));
    addChild(root, 1);

    line1_ = pixelPanelLabel("", 12.0F, ax::Vec2(kWidth - 20.0F, 20.0F), PixelPalette::TextSecondary);
    auto* line1_cell = makeLayoutCell(ax::Size(kWidth - 20.0F, 20.0F), line1_);
    line1_->setPosition(0.0F, 20.0F);
    line1_cell->setLayoutParameter(linearParam(0.0F, 4.0F));
    root->addChild(line1_cell);

    line2_ = pixelPanelLabel("", 12.0F, ax::Vec2(kWidth - 20.0F, 20.0F), PixelPalette::TextSecondary);
    auto* line2_cell = makeLayoutCell(ax::Size(kWidth - 20.0F, 20.0F), line2_);
    line2_->setPosition(0.0F, 20.0F);
    line2_cell->setLayoutParameter(linearParam(0.0F, 0.0F));
    root->addChild(line2_cell);

    root->forceDoLayout();

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
