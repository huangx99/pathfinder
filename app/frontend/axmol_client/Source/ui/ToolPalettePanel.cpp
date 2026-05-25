#include "ui/ToolPalettePanel.h"
#include "ui/PixelUI.h"
#include "procedural/ProceduralArt.h"

namespace pf::ui {
namespace {

constexpr float kWidth = 220.0F;
constexpr float kHeight = 640.0F;
constexpr float kSlotSize = 52.0F;
constexpr float kIconSize = 36.0F;
constexpr float kSlotGap = 4.0F;
constexpr float kCols = 2.0F;

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

    // 标题
    auto* title = pixelPanelLabel("工具", 18.0F, ax::Vec2(kWidth - 24.0F, 26.0F), PixelPalette::TextPrimary);
    title->setPosition(12.0F, kHeight - 10.0F);
    addChild(title, 2);

    // 取消选择按钮
    cancel_button_ = pixelButton(ax::Size(kWidth - 24.0F, 30.0F), "取消选择", 13.0F);
    cancel_button_->setPosition(12.0F, kHeight - 48.0F);

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
    addChild(cancel_button_, 3);
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
    for (auto* label : tool_labels_) {
        label->removeFromParentAndCleanup(true);
    }
    tool_labels_.clear();

    const auto& tools = client_->tools();
    selected_index_ = client_->selectedToolIndex();

    const float grid_x = 14.0F;
    float y = kHeight - 90.0F;

    for (int index = 0; index < static_cast<int>(tools.size()); ++index) {
        const int col = index % 2;
        const float x = grid_x + col * (kSlotSize + kSlotGap);

        const bool is_selected = index == selected_index_;

        // 格子背景
        auto* slot = inventorySlot(kSlotSize, is_selected);
        slot->setPosition(x, y);
        addChild(slot, 2);
        tool_slots_.push_back(slot);

        // 图标
        auto* icon = createToolIcon(tools[index].key, kIconSize);
        if (icon) {
            icon->setPosition(x + kSlotSize * 0.5F, y + kSlotSize * 0.5F + 4.0F);
            addChild(icon, 3);
            tool_slots_.push_back(icon); // 方便清理
        }

        // 文字标签
        auto* label = pixelLabel(tools[index].label, 9.0F,
                                  ax::Vec2(kSlotSize, 12.0F),
                                  is_selected ? PixelPalette::SlotSelected : PixelPalette::TextSecondary);
        label->setAlignment(ax::TextHAlignment::CENTER, ax::TextVAlignment::CENTER);
        label->setPosition(x + kSlotSize * 0.5F, y - 2.0F);
        addChild(label, 3);
        tool_labels_.push_back(label);

        // Touch 监听
        auto* listener = ax::EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = [slot](ax::Touch* touch, ax::Event*) {
            const auto local = slot->convertToNodeSpace(touch->getLocation());
            const auto size = slot->getContentSize();
            return local.x >= 0 && local.x <= size.width &&
                   local.y >= 0 && local.y <= size.height;
        };
        listener->onTouchEnded = [this, index](ax::Touch*, ax::Event*) {
            if (on_tool_clicked_) on_tool_clicked_(index);
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, slot);

        if (col == 1) {
            y -= kSlotSize + kSlotGap + 12.0F;
        }
    }
}

} // namespace pf::ui
