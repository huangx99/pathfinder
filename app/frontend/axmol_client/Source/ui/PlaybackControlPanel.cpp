#include "ui/PlaybackControlPanel.h"
#include "ui/PixelUI.h"

namespace pf::ui {
namespace {

constexpr float kWidth = 220.0F;
constexpr float kHeight = 80.0F;

void setupButtonTouch(ax::Node* button, std::function<void()> callback) {
    auto* listener = ax::EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [button](ax::Touch* touch, ax::Event*) {
        const auto local = button->convertToNodeSpace(touch->getLocation());
        const auto size = button->getContentSize();
        if (local.x >= 0 && local.x <= size.width && local.y >= 0 && local.y <= size.height) {
            if (auto* bg = button->getChildByName("bg")) {
                bg->setColor(ax::Color32(107, 107, 107));
            }
            return true;
        }
        return false;
    };
    listener->onTouchEnded = [button, callback](ax::Touch*, ax::Event*) {
        if (auto* bg = button->getChildByName("bg")) {
            bg->setColor(ax::Color32(139, 139, 139));
        }
        if (callback) callback();
    };
    listener->onTouchCancelled = [button](ax::Touch*, ax::Event*) {
        if (auto* bg = button->getChildByName("bg")) {
            bg->setColor(ax::Color32(139, 139, 139));
        }
    };
    button->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, button);
}

} // namespace

PlaybackControlPanel* PlaybackControlPanel::create(std::function<void()> on_toggle,
                                                    std::function<void()> on_step) {
    auto* panel = new PlaybackControlPanel();
    if (panel && panel->init(std::move(on_toggle), std::move(on_step))) {
        panel->autorelease();
        return panel;
    }
    delete panel;
    return nullptr;
}

bool PlaybackControlPanel::init(std::function<void()> on_toggle,
                                 std::function<void()> on_step) {
    if (!Node::init()) return false;
    on_toggle_ = std::move(on_toggle);
    on_step_ = std::move(on_step);
    setContentSize(ax::Size(kWidth, kHeight));

    bg_ = pixelPanelBackground(getContentSize());
    addChild(bg_, 0);

    // 标题
    auto* title = pixelPanelLabel("回合", 12.0F, ax::Vec2(60.0F, 18.0F), PixelPalette::TextSecondary);
    title->setPosition(10.0F, kHeight - 10.0F);
    addChild(title, 1);

    // 回合数
    tick_label_ = pixelLabel("第 0 回合", 14.0F, ax::Vec2(120.0F, 20.0F), PixelPalette::TextPrimary);
    tick_label_->setPosition(10.0F, kHeight - 34.0F);
    addChild(tick_label_, 1);

    // 开始/暂停按钮
    toggle_button_ = pixelButton(ax::Size(78.0F, 28.0F), "开始", 12.0F);
    toggle_button_->setPosition(kWidth - 172.0F, 10.0F);
    setupButtonTouch(toggle_button_, on_toggle_);
    addChild(toggle_button_, 2);

    // 单步按钮
    step_button_ = pixelButton(ax::Size(78.0F, 28.0F), "步进", 12.0F);
    step_button_->setPosition(kWidth - 88.0F, 10.0F);
    setupButtonTouch(step_button_, on_step_);
    addChild(step_button_, 2);

    return true;
}

void PlaybackControlPanel::setState(bool playing, int tick) {
    playing_ = playing;
    tick_ = tick;
    updateContent();
}

void PlaybackControlPanel::updateContent() {
    tick_label_->setString("第 " + std::to_string(tick_) + " 回合");

    if (auto* label = toggle_button_->getChildByName<ax::Label*>("label")) {
        label->setString(playing_ ? "暂停" : "开始");
    }
    if (auto* bg = toggle_button_->getChildByName("bg")) {
        const auto& color = playing_ ? PixelPalette::HealthBorder : PixelPalette::ButtonNormal;
        // Note: DrawNode color can't be easily changed, we'd need to recreate.
        // For simplicity, we just update the text; the button color stays neutral.
    }
}

} // namespace pf::ui
