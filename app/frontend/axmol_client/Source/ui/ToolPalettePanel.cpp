#include "ui/ToolPalettePanel.h"
#include "ui/PixelUI.h"
#include "procedural/ProceduralArt.h"

#include "axmol/ui/UILayout.h"
#include "axmol/ui/UILayoutParameter.h"
#include "axmol/ui/UIScrollView.h"

#include <algorithm>

namespace pf::ui {
namespace {

constexpr float kWidth = 220.0F;
constexpr float kHeight = 500.0F;
constexpr float kPad = 12.0F;
constexpr float kSlotSize = 52.0F;
constexpr float kIconSize = 36.0F;
constexpr float kSlotGap = 4.0F;
constexpr float kToolLabelHeight = 14.0F;
constexpr float kToolRowGap = 18.0F;
constexpr float kCols = 2.0F;
constexpr float kHeaderHeight = 32.0F;
constexpr float kCancelHeight = 30.0F;
constexpr float kScrollTopGap = 14.0F;


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

ax::Node* createToolIcon(const std::string& key, float size) {
    if (key == "grass")  return pf::art::createGrassTile(size);
    if (key == "soil")   return pf::art::createSoilTile(size);
    if (key == "water")  return pf::art::createWaterTile(size);
    if (key == "red_berry")        return pf::art::createRedBerry(size);
    if (key == "decayed_red_berry") return pf::art::createDecayedRedBerry(size);
    if (key == "stone_flake")      return pf::art::createStoneFlake(size);
    if (key == "bitter_leaf")      return pf::art::createBitterLeaf(size);
    if (key == "wood")             return pf::art::createWood(size);
    if (key == "basic_npc")        return pf::art::createPlayer(size);
    return nullptr;
}

} // namespace

ToolPalettePanel* ToolPalettePanel::create(pf::client::V3LocalClient* client,
                                            std::function<void(int)> on_tool_clicked) {
    auto* panel = new ToolPalettePanel();
    if (panel && panel->init(client, std::move(on_tool_clicked))) {
        panel->autorelease();
        return panel;
    }
    delete panel;
    return nullptr;
}

bool ToolPalettePanel::init(pf::client::V3LocalClient* client,
                             std::function<void(int)> on_tool_clicked) {
    if (!Node::init()) return false;
    client_ = client;
    on_tool_clicked_ = std::move(on_tool_clicked);
    setContentSize(ax::Size(kWidth, kHeight));
    buildSkeleton();
    updateContent();
    return true;
}

void ToolPalettePanel::buildSkeleton() {
    bg_ = pixelPanelBackground(getContentSize());
    addChild(bg_, 0);

    auto* root = ax::ui::Layout::create();
    root->setContentSize(ax::Size(kWidth - kPad * 2.0F, kHeight - kPad * 2.0F));
    root->setLayoutType(ax::ui::Layout::Type::VERTICAL);
    root->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    root->setPosition(ax::Vec2(kPad, kHeight - kPad));
    addChild(root, 1);

    auto* title = pixelPanelLabel("工具", 18.0F, ax::Vec2(kWidth - kPad * 2.0F, kHeaderHeight), PixelPalette::TextPrimary);
    auto* title_cell = makeLayoutCell(ax::Size(kWidth - kPad * 2.0F, kHeaderHeight), title);
    title->setPosition(0.0F, kHeaderHeight);
    title_cell->setLayoutParameter(linearParam(0.0F, 6.0F));
    root->addChild(title_cell);

    cancel_button_ = pixelButton(ax::Size(kWidth - kPad * 2.0F, kCancelHeight), "取消选择", 13.0F);
    auto* cancel_cell = makeLayoutCell(ax::Size(kWidth - kPad * 2.0F, kCancelHeight), cancel_button_);
    cancel_button_->setPosition(0.0F, 0.0F);
    cancel_cell->setLayoutParameter(linearParam(0.0F, kScrollTopGap));
    root->addChild(cancel_cell);

    const float scroll_height = kHeight - kPad * 2.0F - kHeaderHeight - 6.0F - kCancelHeight - kScrollTopGap;
    tool_scroll_ = ax::ui::ScrollView::create();
    tool_scroll_->setContentSize(ax::Size(kWidth - kPad * 2.0F, scroll_height));
    tool_scroll_->setDirection(ax::ui::ScrollView::Direction::VERTICAL);
    tool_scroll_->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    tool_scroll_->setSwallowTouches(true);
    tool_scroll_->setLayoutParameter(linearParam(0.0F, 0.0F));
    root->addChild(tool_scroll_);
    root->forceDoLayout();

    auto* cancel_listener = ax::EventListenerTouchOneByOne::create();
    cancel_listener->setSwallowTouches(true);
    cancel_listener->onTouchBegan = [this](ax::Touch* touch, ax::Event*) {
        const auto local = cancel_button_->convertToNodeSpace(touch->getLocation());
        const auto size = cancel_button_->getContentSize();
        if (local.x >= 0 && local.x <= size.width && local.y >= 0 && local.y <= size.height) {
            // 按下效果
            if (auto* bg = cancel_button_->getChildByName("bg")) {
                bg->setColor(ax::Color32(107, 107, 107));
            }
            return true;
        }
        return false;
    };
    cancel_listener->onTouchEnded = [this](ax::Touch*, ax::Event*) {
        if (auto* bg = cancel_button_->getChildByName("bg")) {
            bg->setColor(ax::Color32(139, 139, 139));
        }
        if (on_tool_clicked_) on_tool_clicked_(-1);
    };
    cancel_listener->onTouchCancelled = [this](ax::Touch*, ax::Event*) {
        if (auto* bg = cancel_button_->getChildByName("bg")) {
            bg->setColor(ax::Color32(139, 139, 139));
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(cancel_listener, cancel_button_);
}

void ToolPalettePanel::refresh() {
    updateContent();
}

void ToolPalettePanel::updateContent() {
    // 清除旧的工具格子
    for (auto* slot : tool_slots_) {
        slot->removeFromParentAndCleanup(true);
    }
    tool_slots_.clear();
    tool_labels_.clear();

    const auto& tools = client_->tools();
    selected_index_ = client_->selectedToolIndex();

    if (!tool_scroll_) return;

    const float content_width = tool_scroll_->getContentSize().width;
    const int rows = static_cast<int>((tools.size() + 1) / 2);
    const float row_height = kSlotSize + kToolLabelHeight + kToolRowGap;
    const float content_height = std::max(tool_scroll_->getContentSize().height, rows * row_height + 8.0F);
    tool_scroll_->setInnerContainerSize(ax::Vec2(content_width, content_height));

    const float grid_x = 4.0F;
    float y = content_height - kSlotSize;

    for (int index = 0; index < static_cast<int>(tools.size()); ++index) {
        const int col = index % 2;
        const float x = grid_x + col * (kSlotSize + kSlotGap);

        const bool is_selected = index == selected_index_;

        auto* item_cell = ax::ui::Layout::create();
        item_cell->setContentSize(ax::Size(kSlotSize, kSlotSize + kToolLabelHeight));
        item_cell->setAnchorPoint(ax::Vec2(0.0F, 0.0F));
        item_cell->setPosition(ax::Vec2(x, y - kToolLabelHeight));
        tool_scroll_->addChild(item_cell, 2);
        tool_slots_.push_back(item_cell);

        auto* slot = inventorySlot(kSlotSize, is_selected);
        slot->setPosition(0.0F, kToolLabelHeight);
        item_cell->addChild(slot, 1);

        auto* icon = createToolIcon(tools[index].key, kIconSize);
        if (icon) {
            icon->setPosition(kSlotSize * 0.5F, kToolLabelHeight + kSlotSize * 0.5F);
            item_cell->addChild(icon, 2);
        }

        auto* label = pixelLabel(tools[index].label, 9.0F,
                                  ax::Vec2(kSlotSize, kToolLabelHeight),
                                  is_selected ? PixelPalette::SlotSelected : PixelPalette::TextSecondary);
        label->setAlignment(ax::TextHAlignment::CENTER, ax::TextVAlignment::CENTER);
        label->setPosition(kSlotSize * 0.5F, 4.0F);
        item_cell->addChild(label, 3);
        tool_labels_.push_back(label);

        auto* listener = ax::EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = [item_cell](ax::Touch* touch, ax::Event*) {
            const auto local = item_cell->convertToNodeSpace(touch->getLocation());
            const auto size = item_cell->getContentSize();
            return local.x >= 0 && local.x <= size.width &&
                   local.y >= 0 && local.y <= size.height;
        };
        listener->onTouchEnded = [this, index](ax::Touch*, ax::Event*) {
            if (on_tool_clicked_) on_tool_clicked_(index);
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, item_cell);

        if (col == 1) {
            y -= row_height;
        }
    }
}

} // namespace pf::ui
