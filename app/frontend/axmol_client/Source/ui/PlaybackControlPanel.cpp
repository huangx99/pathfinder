#include "ui/PlaybackControlPanel.h"
#include "ui/PixelUI.h"

#include "axmol/ui/UILayout.h"
#include "axmol/ui/UILayoutParameter.h"

namespace pf::ui {
namespace {

constexpr float kWidth = 220.0F;
constexpr float kHeight = 104.0F;
constexpr float kPad = 10.0F;
constexpr float kGap = 8.0F;


ax::ui::LinearLayoutParameter* linearParam(float top, float bottom,
                                           ax::ui::LinearLayoutParameter::LinearGravity gravity =
                                               ax::ui::LinearLayoutParameter::LinearGravity::LEFT,
                                           float left = 0.0F, float right = 0.0F) {
    auto* param = ax::ui::LinearLayoutParameter::create();
    param->setGravity(gravity);
    param->setMargin(ax::ui::Margin(left, top, right, bottom));
    return param;
}

ax::ui::Layout* makeLayoutCell(const ax::Size& size, ax::Node* child) {
    auto* cell = ax::ui::Layout::create();
    cell->setContentSize(size);
    cell->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    if (child) cell->addChild(child, 1);
    return cell;
}

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

    auto* root = ax::ui::Layout::create();
    root->setContentSize(ax::Size(kWidth - kPad * 2.0F, kHeight - kPad * 2.0F));
    root->setLayoutType(ax::ui::Layout::Type::VERTICAL);
    root->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    root->setPosition(ax::Vec2(kPad, kHeight - kPad));
    addChild(root, 1);

    auto* title = pixelPanelLabel("回合", 12.0F, ax::Vec2(kWidth - kPad * 2.0F, 18.0F), PixelPalette::TextSecondary);
    auto* title_cell = makeLayoutCell(ax::Size(kWidth - kPad * 2.0F, 18.0F), title);
    title->setPosition(0.0F, 18.0F);
    title_cell->setLayoutParameter(linearParam(0.0F, 2.0F));
    root->addChild(title_cell);

    tick_label_ = pixelPanelLabel("第 0 回合", 14.0F, ax::Vec2(kWidth - kPad * 2.0F, 22.0F), PixelPalette::TextPrimary);
    auto* tick_cell = makeLayoutCell(ax::Size(kWidth - kPad * 2.0F, 22.0F), tick_label_);
    tick_label_->setPosition(0.0F, 22.0F);
    tick_cell->setLayoutParameter(linearParam(0.0F, kGap));
    root->addChild(tick_cell);

    auto* button_row = ax::ui::Layout::create();
    button_row->setContentSize(ax::Size(kWidth - kPad * 2.0F, 28.0F));
    button_row->setLayoutType(ax::ui::Layout::Type::HORIZONTAL);
    button_row->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    button_row->setLayoutParameter(linearParam(0.0F, 0.0F));
    root->addChild(button_row);

    toggle_button_ = pixelButton(ax::Size(78.0F, 28.0F), "开始", 12.0F);
    auto* toggle_cell = makeLayoutCell(ax::Size(78.0F, 28.0F), toggle_button_);
    toggle_button_->setPosition(0.0F, 0.0F);
    toggle_cell->setLayoutParameter(linearParam(0.0F, 0.0F, ax::ui::LinearLayoutParameter::LinearGravity::TOP, 0.0F, kGap));
    setupButtonTouch(toggle_button_, on_toggle_);
    button_row->addChild(toggle_cell);

    step_button_ = pixelButton(ax::Size(78.0F, 28.0F), "步进", 12.0F);
    auto* step_cell = makeLayoutCell(ax::Size(78.0F, 28.0F), step_button_);
    step_button_->setPosition(0.0F, 0.0F);
    step_cell->setLayoutParameter(linearParam(0.0F, 0.0F));
    setupButtonTouch(step_button_, on_step_);
    button_row->addChild(step_cell);

    root->forceDoLayout();
    button_row->forceDoLayout();

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
