#include "ui/ToolPalettePanel.h"
#include "ui/UiStyle.h"

#include "axmol/ui/UIScrollView.h"

namespace pf::ui {
namespace {
constexpr float kWidth = 220.0F;
constexpr float kHeight = 640.0F;
constexpr float kButtonHeight = 34.0F;
constexpr float kListHeight = 430.0F;
constexpr float kListInnerHeight = 760.0F;
}

ToolPalettePanel* ToolPalettePanel::create(pf::client::V3LocalClient* client, std::function<void(int)> on_tool_clicked) {
    auto* panel = new ToolPalettePanel();
    if (panel && panel->init(client, std::move(on_tool_clicked))) {
        panel->autorelease();
        return panel;
    }
    delete panel;
    return nullptr;
}

bool ToolPalettePanel::init(pf::client::V3LocalClient* client, std::function<void(int)> on_tool_clicked) {
    if (!Node::init()) return false;
    client_ = client;
    on_tool_clicked_ = std::move(on_tool_clicked);
    setContentSize(ax::Size(kWidth, kHeight));
    refresh();
    return true;
}

void ToolPalettePanel::refresh() {
    removeAllChildren();
    addChild(panelBackground(getContentSize()), 0);
    auto* title = panelLabel("工具", 24.0F, ax::Vec2(kWidth - 24.0F, 32.0F));
    title->setPosition(12.0F, kHeight - 16.0F);
    addChild(title, 2);

    const auto* selected_tool = client_->selectedTool();
    auto* selected = panelLabel(selected_tool ? ("当前：" + selected_tool->label) : "当前：未选择", 15.0F, ax::Vec2(kWidth - 24.0F, 42.0F));
    selected->setTextColor(ax::Color32(250, 204, 21, 255));
    selected->setPosition(12.0F, kHeight - 56.0F);
    addChild(selected, 2);

    auto* cancel_label = label("取消选择", 16.0F, ax::Vec2(kWidth - 36.0F, 28.0F));
    cancel_label->setTextColor(client_->hasSelectedTool() ? ax::Color32(226, 232, 240, 255) : ax::Color32(15, 23, 42, 255));
    auto* cancel_item = ax::MenuItemLabel::create(cancel_label, [this](ax::Object*) {
        if (on_tool_clicked_) on_tool_clicked_(-1);
    });
    cancel_item->setAnchorPoint(ax::Vec2(0.0F, 0.5F));
    cancel_item->setPosition(16.0F, kHeight - 100.0F);
    auto* cancel_menu = ax::Menu::create(cancel_item, nullptr);
    cancel_menu->setPosition(ax::Vec2::ZERO);
    addChild(cancel_menu, 3);
    auto* cancel_bg = ax::DrawNode::create();
    cancel_bg->drawSolidRect(ax::Vec2(10.0F, kHeight - 100.0F - kButtonHeight * 0.5F), ax::Vec2(kWidth - 10.0F, kHeight - 100.0F + kButtonHeight * 0.5F),
                             client_->hasSelectedTool() ? color(30, 41, 59, 0.86F) : color(250, 204, 21, 0.92F));
    addChild(cancel_bg, 1);

    auto* scroll = ax::ui::ScrollView::create();
    scroll->setContentSize(ax::Size(kWidth - 20.0F, kListHeight));
    scroll->setInnerContainerSize(ax::Size(kWidth - 20.0F, kListInnerHeight));
    scroll->setDirection(ax::ui::ScrollView::Direction::VERTICAL);
    scroll->setBounceEnabled(true);
    scroll->setScrollBarEnabled(true);
    scroll->setScrollBarWidth(4.0F);
    scroll->setScrollBarColor(ax::Color32(148, 163, 184, 255));
    scroll->setScrollBarOpacity(180);
    scroll->setScrollBarPositionFromCorner(ax::Vec2(2.0F, 2.0F));
    scroll->setPosition(ax::Vec2(10.0F, 24.0F));
    addChild(scroll, 3);

    auto* menu = ax::Menu::create();
    menu->setPosition(ax::Vec2::ZERO);
    scroll->addChild(menu, 3);

    float y = kListInnerHeight - 18.0F;
    std::string current_category;
    const auto& tools = client_->tools();
    for (int index = 0; index < static_cast<int>(tools.size()); ++index) {
        if (tools[index].category != current_category) {
            current_category = tools[index].category;
            auto* section = panelLabel(current_category, 14.0F, ax::Vec2(kWidth - 24.0F, 22.0F));
            section->setTextColor(ax::Color32(147, 197, 253, 255));
            section->setPosition(2.0F, y + 10.0F);
            scroll->addChild(section, 2);
            y -= 24.0F;
        }
        const bool is_selected = index == client_->selectedToolIndex();
        auto* item_label = label(tools[index].label, 16.0F, ax::Vec2(kWidth - 36.0F, 28.0F));
        item_label->setTextColor(is_selected ? ax::Color32(15, 23, 42, 255) : ax::Color32(226, 232, 240, 255));
        auto* item = ax::MenuItemLabel::create(item_label, [this, index](ax::Object*) {
            if (on_tool_clicked_) on_tool_clicked_(index);
        });
        item->setAnchorPoint(ax::Vec2(0.0F, 0.5F));
        item->setPosition(8.0F, y);
        menu->addChild(item, 2);

        auto* bg = ax::DrawNode::create();
        bg->drawSolidRect(ax::Vec2(0.0F, y - kButtonHeight * 0.5F), ax::Vec2(kWidth - 20.0F, y + kButtonHeight * 0.5F),
                          is_selected ? color(250, 204, 21, 0.92F) : color(30, 41, 59, 0.86F));
        scroll->addChild(bg, 1);
        y -= kButtonHeight + 7.0F;
    }
}

} // namespace pf::ui
