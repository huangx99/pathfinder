#include "ui/PlaybackControlPanel.h"
#include "ui/UiStyle.h"

namespace pf::ui {
namespace {
constexpr float kWidth = 360.0F;
constexpr float kHeight = 70.0F;

ax::MenuItemLabel* menuButton(const std::string& text, const ax::Color& fill, const std::function<void()>& callback) {
    auto* bg = ax::DrawNode::create();
    bg->drawSolidRect(ax::Vec2::ZERO, ax::Vec2(128.0F, 34.0F), fill);
    bg->drawRect(ax::Vec2::ZERO, ax::Vec2(128.0F, 34.0F), color(148, 163, 184, 0.75F));

    auto* text_label = panelLabel(text, 16.0F, ax::Vec2(128.0F, 34.0F));
    text_label->setAlignment(ax::TextHAlignment::CENTER, ax::TextVAlignment::CENTER);
    text_label->setTextColor(ax::Color32(248, 250, 252, 255));
    text_label->setPosition(0.0F, 34.0F);

    auto* container = ax::Node::create();
    container->setContentSize(ax::Size(128.0F, 34.0F));
    container->addChild(bg, 0);
    container->addChild(text_label, 1);

    return ax::MenuItemLabel::create(container, [callback](ax::Object*) {
        if (callback) callback();
    });
}
} // namespace

PlaybackControlPanel* PlaybackControlPanel::create(std::function<void()> on_toggle, std::function<void()> on_step) {
    auto* panel = new PlaybackControlPanel();
    if (panel && panel->init(std::move(on_toggle), std::move(on_step))) {
        panel->autorelease();
        return panel;
    }
    delete panel;
    return nullptr;
}

bool PlaybackControlPanel::init(std::function<void()> on_toggle, std::function<void()> on_step) {
    if (!Node::init()) return false;
    on_toggle_ = std::move(on_toggle);
    on_step_ = std::move(on_step);
    setContentSize(ax::Size(kWidth, kHeight));
    render();
    return true;
}

void PlaybackControlPanel::setState(bool playing, int tick) {
    playing_ = playing;
    tick_ = tick;
    render();
}

void PlaybackControlPanel::render() {
    removeAllChildren();
    addChild(panelBackground(getContentSize(), color(10, 18, 32, 0.94F)), 0);

    auto* title = panelLabel("时间", 14.0F, ax::Vec2(82.0F, 22.0F));
    title->setTextColor(ax::Color32(147, 197, 253, 255));
    title->setPosition(14.0F, kHeight - 12.0F);
    addChild(title, 1);

    auto* tick_label = panelLabel("第 " + std::to_string(tick_) + " 回合", 16.0F, ax::Vec2(112.0F, 28.0F));
    tick_label->setPosition(14.0F, 36.0F);
    addChild(tick_label, 1);

    auto* toggle = menuButton(playing_ ? "暂停" : "开始", playing_ ? color(185, 28, 28, 0.95F) : color(21, 128, 61, 0.95F), on_toggle_);
    auto* step = menuButton("单步", color(30, 64, 175, 0.95F), on_step_);
    auto* menu = ax::Menu::create(toggle, step, nullptr);
    menu->setPosition(ax::Vec2::ZERO);
    toggle->setPosition(154.0F, 24.0F);
    step->setPosition(292.0F, 24.0F);
    addChild(menu, 2);
}

} // namespace pf::ui
