#include "ui/ToolPalettePanel.h"
#include "ui/UiStyle.h"

namespace pf::ui {
namespace {
constexpr float kWidth = 220.0F;
constexpr float kHeight = 640.0F;
constexpr float kButtonHeight = 34.0F;
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
    auto* title = label("工具", 24.0F, ax::Vec2(kWidth - 24.0F, 32.0F));
    title->setPosition(12.0F, kHeight - 16.0F);
    addChild(title, 2);

    auto* selected = label("当前：" + client_->selectedTool().label, 15.0F, ax::Vec2(kWidth - 24.0F, 42.0F));
    selected->setTextColor(ax::Color32(250, 204, 21, 255));
    selected->setPosition(12.0F, kHeight - 56.0F);
    addChild(selected, 2);

    auto* menu = ax::Menu::create();
    menu->setPosition(ax::Vec2::ZERO);
    addChild(menu, 3);

    float y = kHeight - 104.0F;
    const auto& tools = client_->tools();
    for (int index = 0; index < static_cast<int>(tools.size()); ++index) {
        const bool is_selected = index == client_->selectedToolIndex();
        auto* item_label = label(tools[index].label, 16.0F, ax::Vec2(kWidth - 36.0F, 28.0F));
        item_label->setTextColor(is_selected ? ax::Color32(15, 23, 42, 255) : ax::Color32(226, 232, 240, 255));
        auto* item = ax::MenuItemLabel::create(item_label, [this, index](ax::Object*) {
            if (on_tool_clicked_) on_tool_clicked_(index);
        });
        item->setAnchorPoint(ax::Vec2(0.0F, 0.5F));
        item->setPosition(16.0F, y);
        menu->addChild(item, 2);

        auto* bg = ax::DrawNode::create();
        bg->drawSolidRect(ax::Vec2(10.0F, y - kButtonHeight * 0.5F), ax::Vec2(kWidth - 10.0F, y + kButtonHeight * 0.5F),
                          is_selected ? color(250, 204, 21, 0.92F) : color(30, 41, 59, 0.86F));
        addChild(bg, 1);
        y -= kButtonHeight + 7.0F;
    }
}

} // namespace pf::ui
