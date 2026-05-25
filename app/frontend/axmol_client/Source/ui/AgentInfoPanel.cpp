#include "ui/AgentInfoPanel.h"
#include "ui/PixelUI.h"
#include "procedural/ProceduralArt.h"

#include "axmol/ui/UILayout.h"
#include "axmol/ui/UILayoutParameter.h"

#include <algorithm>
#include <cmath>

namespace pf::ui {
namespace {

constexpr float kWidth = 280.0F;
constexpr float kHeight = 540.0F;
constexpr float kSlotSize = 44.0F;
constexpr float kSlotGap = 2.0F;
constexpr float kGridCols = 4;
constexpr float kGridRows = 4;
constexpr float kGridWidth  = kGridCols * kSlotSize + (kGridCols - 1) * kSlotGap;
constexpr float kGridHeight = kGridRows * kSlotSize + (kGridRows - 1) * kSlotGap;
constexpr float kIconSize = 32.0F;
constexpr float kPanelPadX = 12.0F;
constexpr float kContentWidth = kWidth - kPanelPadX * 2.0F;
constexpr float kNameHeight = 30.0F;
constexpr float kStatsHeight = 62.0F;
constexpr float kSectionTitleHeight = 22.0F;
constexpr float kSelectedItemHeight = 22.0F;
constexpr float kKnowledgeLineHeight = 20.0F;
constexpr float kInventoryGridX = (kContentWidth - kGridWidth) * 0.5F;
constexpr float kInventoryGridBottom = kHeight - 10.0F - kNameHeight - 4.0F - kStatsHeight - 10.0F - kSectionTitleHeight - 6.0F - kGridHeight;


ax::ui::LinearLayoutParameter* linearParam(float top, float bottom,
                                           ax::ui::LinearLayoutParameter::LinearGravity gravity =
                                               ax::ui::LinearLayoutParameter::LinearGravity::LEFT) {
    auto* param = ax::ui::LinearLayoutParameter::create();
    param->setGravity(gravity);
    param->setMargin(ax::ui::Margin(0.0F, top, 0.0F, bottom));
    return param;
}

ax::ui::Layout* makeLayoutCell(const ax::Size& size, ax::Node* child = nullptr) {
    auto* cell = ax::ui::Layout::create();
    cell->setContentSize(size);
    cell->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    if (child) cell->addChild(child, 1);
    return cell;
}

std::string actionName(const std::string& key) {
    if (key == "eat") return "吃";
    if (key == "use") return "使用";
    return key;
}

std::string effectName(const std::string& key) {
    if (key == "restore_hunger") return "缓解饥饿";
    if (key == "poison") return "中毒";
    if (key == "use_hint") return "发现用途";
    if (key == "no_visible_effect") return "无明显效果";
    return key;
}

std::string statusIcon(const std::string& status) {
    if (status == "active") return "✓";
    if (status == "hypothesis") return "?";
    return "~";
}

ax::Color32 statusColor(const std::string& status) {
    if (status == "active") return ax::Color32(74, 222, 128);
    if (status == "hypothesis") return ax::Color32(250, 204, 21);
    return ax::Color32(148, 163, 184);
}

ax::Node* createItemIconNode(const std::string& key, float size) {
    if (key == "red_berry") return pf::art::createRedBerry(size);
    if (key == "decayed_red_berry") return pf::art::createDecayedRedBerry(size);
    if (key == "stone_flake") return pf::art::createStoneFlake(size);
    if (key == "bitter_leaf") return pf::art::createBitterLeaf(size);
    if (key == "wood") return pf::art::createWood(size);
    return nullptr;
}

} // namespace

AgentInfoPanel* AgentInfoPanel::create() {
    auto* panel = new AgentInfoPanel();
    if (panel && panel->init()) {
        panel->autorelease();
        return panel;
    }
    delete panel;
    return nullptr;
}

bool AgentInfoPanel::init() {
    if (!Node::init()) return false;
    setContentSize(ax::Size(kWidth, kHeight));
    buildSkeleton();
    return true;
}

void AgentInfoPanel::buildSkeleton() {
    bg_ = pixelPanelBackground(getContentSize());
    addChild(bg_, 0);

    auto* root = ax::ui::Layout::create();
    root->setContentSize(ax::Size(kContentWidth, kHeight - 20.0F));
    root->setLayoutType(ax::ui::Layout::Type::VERTICAL);
    root->setAnchorPoint(ax::Vec2(0.0F, 1.0F));
    root->setPosition(ax::Vec2(kPanelPadX, kHeight - 10.0F));
    addChild(root, 1);

    name_label_ = pixelPanelLabel("", 18.0F, ax::Vec2(kContentWidth, kNameHeight), PixelPalette::TextPrimary);
    auto* name_cell = makeLayoutCell(ax::Size(kContentWidth, kNameHeight), name_label_);
    name_label_->setPosition(0.0F, kNameHeight);
    name_cell->setLayoutParameter(linearParam(0.0F, 4.0F));
    root->addChild(name_cell);

    auto* stats_cell = makeLayoutCell(ax::Size(kContentWidth, kStatsHeight));
    stats_cell->setLayoutParameter(linearParam(0.0F, 10.0F));
    root->addChild(stats_cell);

    auto* heart = createHeartIcon(20.0F);
    heart->setPosition(0.0F, 38.0F);
    stats_cell->addChild(heart, 2);

    health_bar_ = pixelProgressBar(kContentWidth - 28.0F, 14.0F, 1.0F,
                                    PixelPalette::HealthFill,
                                    PixelPalette::PanelFill,
                                    PixelPalette::HealthBorder);
    health_bar_->setPosition(28.0F, 41.0F);
    stats_cell->addChild(health_bar_, 2);

    health_text_ = pixelLabel("100%", 11.0F, ax::Vec2(40.0F, 14.0F), PixelPalette::TextPrimary);
    health_text_->setAlignment(ax::TextHAlignment::CENTER, ax::TextVAlignment::CENTER);
    health_text_->setPosition(kContentWidth - 12.0F, 48.0F);
    stats_cell->addChild(health_text_, 3);

    auto* hunger = createHungerIcon(20.0F);
    hunger->setPosition(0.0F, 10.0F);
    stats_cell->addChild(hunger, 2);

    hunger_bar_ = pixelProgressBar(kContentWidth - 28.0F, 14.0F, 1.0F,
                                    PixelPalette::HungerFill,
                                    PixelPalette::PanelFill,
                                    PixelPalette::HungerBorder);
    hunger_bar_->setPosition(28.0F, 13.0F);
    stats_cell->addChild(hunger_bar_, 2);

    hunger_text_ = pixelLabel("100%", 11.0F, ax::Vec2(40.0F, 14.0F), PixelPalette::TextPrimary);
    hunger_text_->setAlignment(ax::TextHAlignment::CENTER, ax::TextVAlignment::CENTER);
    hunger_text_->setPosition(kContentWidth - 12.0F, 20.0F);
    stats_cell->addChild(hunger_text_, 3);

    auto* inv_title = pixelPanelLabel("背包", 14.0F, ax::Vec2(kContentWidth, kSectionTitleHeight), PixelPalette::TextSecondary);
    auto* inv_title_cell = makeLayoutCell(ax::Size(kContentWidth, kSectionTitleHeight), inv_title);
    inv_title->setPosition(0.0F, kSectionTitleHeight);
    inv_title_cell->setLayoutParameter(linearParam(0.0F, 6.0F));
    root->addChild(inv_title_cell);

    auto* grid_cell = makeLayoutCell(ax::Size(kContentWidth, kGridHeight));
    grid_cell->setLayoutParameter(linearParam(0.0F, 12.0F));
    root->addChild(grid_cell);

    inventory_container_ = ax::Node::create();
    inventory_container_->setPosition(kInventoryGridX, 0.0F);
    grid_cell->addChild(inventory_container_, 2);

    slot_nodes_.resize(16);
    slot_quantity_labels_.resize(16);
    for (int i = 0; i < 16; ++i) {
        const int col = i % 4;
        const int row = i / 4;
        const float x = col * (kSlotSize + kSlotGap);
        const float y = (3 - row) * (kSlotSize + kSlotGap);

        auto* slot = inventorySlot(kSlotSize);
        slot->setPosition(x, y);
        slot->setName("slot_" + std::to_string(i));
        inventory_container_->addChild(slot, 0);
        slot_nodes_[i] = slot;

        auto* qty = pixelLabel("", 10.0F, ax::Vec2(20.0F, 12.0F), pixelColor(255, 255, 255));
        qty->setAlignment(ax::TextHAlignment::RIGHT, ax::TextVAlignment::BOTTOM);
        qty->setPosition(x + kSlotSize - 2.0F, y + 2.0F);
        qty->setName("qty_" + std::to_string(i));
        inventory_container_->addChild(qty, 3);
        slot_quantity_labels_[i] = qty;

        auto* listener = ax::EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(false);
        listener->onTouchBegan = [this, i](ax::Touch* touch, ax::Event*) {
            const auto local = inventory_container_->convertToNodeSpace(touch->getLocation());
            const int col = i % 4;
            const int row = i / 4;
            const float x = col * (kSlotSize + kSlotGap);
            const float y = (3 - row) * (kSlotSize + kSlotGap);
            if (local.x >= x && local.x <= x + kSlotSize &&
                local.y >= y && local.y <= y + kSlotSize) {
                hovered_slot_ = i;
                scheduleOnce([this, i](float) { showTooltip(i); }, 0.3F, "tooltip_" + std::to_string(i));
                return true;
            }
            return false;
        };
        listener->onTouchEnded = [this, i](ax::Touch*, ax::Event*) {
            unschedule("tooltip_" + std::to_string(i));
            hideTooltip();
            if (hovered_slot_ == i) {
                onSlotTouched(i);
            }
            hovered_slot_ = -1;
        };
        listener->onTouchCancelled = [this, i](ax::Touch*, ax::Event*) {
            unschedule("tooltip_" + std::to_string(i));
            hideTooltip();
            hovered_slot_ = -1;
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, slot);
    }

    selected_item_label_ = pixelPanelLabel("", 12.0F, ax::Vec2(kContentWidth, kSelectedItemHeight), PixelPalette::TextSecondary);
    auto* selected_cell = makeLayoutCell(ax::Size(kContentWidth, kSelectedItemHeight), selected_item_label_);
    selected_item_label_->setPosition(0.0F, kSelectedItemHeight);
    selected_cell->setLayoutParameter(linearParam(0.0F, 8.0F));
    root->addChild(selected_cell);

    knowledge_title_ = pixelPanelLabel("知识", 14.0F, ax::Vec2(kContentWidth, kSectionTitleHeight), PixelPalette::TextSecondary);
    auto* knowledge_title_cell = makeLayoutCell(ax::Size(kContentWidth, kSectionTitleHeight), knowledge_title_);
    knowledge_title_->setPosition(0.0F, kSectionTitleHeight);
    knowledge_title_cell->setLayoutParameter(linearParam(0.0F, 6.0F));
    root->addChild(knowledge_title_cell);

    knowledge_container_ = ax::Node::create();
    auto* knowledge_cell = makeLayoutCell(ax::Size(kContentWidth, 140.0F), knowledge_container_);
    knowledge_container_->setPosition(0.0F, 140.0F);
    knowledge_cell->setLayoutParameter(linearParam(0.0F, 0.0F));
    root->addChild(knowledge_cell);

    root->forceDoLayout();
}

void AgentInfoPanel::setAgent(const pathfinder::v3_sandbox::V3AgentView* agent) {
    if (!agent) {
        has_agent_ = false;
        current_agent_id_.clear();
        selected_item_key_.clear();
        current_inventory_.clear();

        name_label_->setString("点击地图上的小人");
        updateHealth(0.0);
        updateHunger(0.0);
        updateInventory({});
        selected_item_label_->setString("");
        knowledge_title_->setString("知识");
        for (auto* line : knowledge_lines_) {
            line->setVisible(false);
        }
        return;
    }

    const bool changed = current_agent_id_ != agent->agent_id;
    has_agent_ = true;
    current_agent_id_ = agent->agent_id;
    current_agent_data_ = *agent;

    if (changed) {
        selected_item_key_.clear();
    }

    name_label_->setString(agent->name);
    updateHealth(agent->health);
    updateHunger(agent->hunger);
    updateInventory(agent->inventory);
    updateKnowledge(agent);
}

void AgentInfoPanel::updateHealth(double value) {
    const float ratio = static_cast<float>(std::clamp(value, 0.0, 100.0) / 100.0);
    // 重新创建进度条（因为 axmol DrawNode 不支持动态修改已有绘制）
    if (health_bar_) {
        health_bar_->removeFromParent();
    }
    health_bar_ = pixelProgressBar(kWidth - 48.0F, 14.0F, ratio,
                                    PixelPalette::HealthFill,
                                    PixelPalette::PanelFill,
                                    PixelPalette::HealthBorder);
    health_bar_->setPosition(40.0F, getContentSize().height - 52.0F);
    addChild(health_bar_, 2);

    health_text_->setString(std::to_string(static_cast<int>(std::round(std::clamp(value, 0.0, 100.0)))) + "%");
}

void AgentInfoPanel::updateHunger(double value) {
    const float ratio = static_cast<float>(std::clamp(value, 0.0, 100.0) / 100.0);
    if (hunger_bar_) {
        hunger_bar_->removeFromParent();
    }
    hunger_bar_ = pixelProgressBar(kWidth - 48.0F, 14.0F, ratio,
                                    PixelPalette::HungerFill,
                                    PixelPalette::PanelFill,
                                    PixelPalette::HungerBorder);
    hunger_bar_->setPosition(40.0F, getContentSize().height - 80.0F);
    addChild(hunger_bar_, 2);

    hunger_text_->setString(std::to_string(static_cast<int>(std::round(std::clamp(value, 0.0, 100.0)))) + "%");
}

void AgentInfoPanel::updateInventory(const std::vector<pathfinder::v3_sandbox::V3InventoryItemView>& inventory) {
    current_inventory_ = inventory;

    // 清除旧的图标子节点
    for (int i = 0; i < 16; ++i) {
        auto* slot = slot_nodes_[i];
        // 移除旧的图标（保留背景和边框）
        auto* old_icon = slot->getChildByName("icon");
        if (old_icon) old_icon->removeFromParent();

        if (i < static_cast<int>(inventory.size())) {
            const auto& item = inventory[i];
            auto* icon = createItemIconNode(item.object_key, kIconSize);
            if (icon) {
                icon->setPosition(kSlotSize * 0.5F, kSlotSize * 0.5F);
                icon->setName("icon");
                slot->addChild(icon, 1);
            }
            slot_quantity_labels_[i]->setString("×" + std::to_string(item.quantity));
            slot_quantity_labels_[i]->setVisible(true);
        } else {
            slot_quantity_labels_[i]->setString("");
            slot_quantity_labels_[i]->setVisible(false);
        }

        // 更新选中状态边框
        auto* frame = slot->getChildByName("frame");
        if (frame) frame->removeFromParent();
        bool selected = false;
        if (i < static_cast<int>(inventory.size())) {
            selected = inventory[i].object_key == selected_item_key_;
        }
        auto* new_slot = inventorySlot(kSlotSize, selected);
        // 只取边框部分
        if (auto* border = new_slot->getChildByName("frame")) {
            border->retain();
            border->removeFromParent();
            border->setName("frame");
            slot->addChild(border, 2);
            border->release();
        }
        new_slot->removeFromParentAndCleanup(true);
    }

    // 更新选中物品信息
    if (!selected_item_key_.empty()) {
        for (const auto& item : inventory) {
            if (item.object_key == selected_item_key_) {
                selected_item_label_->setString(item.display_name + " ×" + std::to_string(item.quantity));
                break;
            }
        }
    } else {
        selected_item_label_->setString("");
    }
}

void AgentInfoPanel::onSlotTouched(int slot_index) {
    if (slot_index < 0 || slot_index >= static_cast<int>(current_inventory_.size())) {
        selected_item_key_.clear();
    } else {
        const auto& key = current_inventory_[slot_index].object_key;
        selected_item_key_ = (selected_item_key_ == key) ? std::string{} : key;
    }
    updateInventory(current_inventory_);
    if (has_agent_) {
        updateKnowledge(&current_agent_data_);
    }
}

void AgentInfoPanel::showTooltip(int slot_index) {
    if (slot_index < 0 || slot_index >= static_cast<int>(current_inventory_.size())) return;
    const auto& item = current_inventory_[slot_index];

    hideTooltip();

    // 收集该物品的知识
    std::vector<std::string> lines;
    for (const auto& claim : current_agent_data_.knowledge) {
        if (claim.subject_object_key == item.object_key) {
            lines.push_back(statusIcon(claim.status) + " " + actionName(claim.action_key) +
                           " → " + effectName(claim.effect_key));
        }
    }
    if (lines.empty()) {
        lines.push_back("暂无知识");
    }

    const float tooltip_w = 180.0F;
    const float tooltip_h = 40.0F + lines.size() * 16.0F;
    tooltip_ = createTooltip(ax::Size(tooltip_w, tooltip_h),
                              item.display_name + " ×" + std::to_string(item.quantity),
                              lines, 11.0F);

    // 位置：格子上方
    const int col = slot_index % 4;
    const int row = slot_index / 4;
    const float slot_x = kPanelPadX + kInventoryGridX + col * (kSlotSize + kSlotGap);
    const float slot_y = kInventoryGridBottom + (3 - row) * (kSlotSize + kSlotGap);
    float tx = slot_x + kSlotSize * 0.5F - tooltip_w * 0.5F;
    float ty = slot_y + kSlotSize + 4.0F;
    // 避免超出面板右边界
    if (tx + tooltip_w > kWidth) tx = kWidth - tooltip_w - 4.0F;
    if (tx < 4.0F) tx = 4.0F;
    if (ty + tooltip_h > getContentSize().height) ty = slot_y - tooltip_h - 4.0F;

    tooltip_->setPosition(tx, ty);
    addChild(tooltip_, 10);
}

void AgentInfoPanel::hideTooltip() {
    if (tooltip_) {
        tooltip_->removeFromParent();
        tooltip_ = nullptr;
    }
}

void AgentInfoPanel::updateKnowledge(const pathfinder::v3_sandbox::V3AgentView* agent) {
    // 过滤知识
    std::vector<pathfinder::v3_sandbox::V3KnowledgeClaimView> filtered;
    for (const auto& claim : agent->knowledge) {
        if (!selected_item_key_.empty()) {
            if (claim.subject_object_key == selected_item_key_) {
                filtered.push_back(claim);
            }
        } else {
            if (claim.status == "active") {
                filtered.push_back(claim);
            }
        }
    }

    // 限制显示数量
    const size_t max_lines = 6;
    if (filtered.size() > max_lines) filtered.resize(max_lines);

    // 更新标题
    const std::string title = selected_item_key_.empty()
        ? "知识 (" + std::to_string(filtered.size()) + ")"
        : "关于此物 (" + std::to_string(filtered.size()) + ")";
    knowledge_title_->setString(title);

    // 隐藏旧的行
    for (auto* line : knowledge_lines_) {
        line->setVisible(false);
    }

    // 显示新行（复用或创建）
    const float line_h = kKnowledgeLineHeight;
    float y = 0.0F;
    for (size_t i = 0; i < filtered.size(); ++i) {
        const auto& claim = filtered[i];
        const std::string text = statusIcon(claim.status) + " " +
                                 actionName(claim.action_key) + " → " +
                                 effectName(claim.effect_key);

        ax::Label* line_label = nullptr;
        if (i < knowledge_lines_.size()) {
            line_label = knowledge_lines_[i];
            line_label->setString(text);
            line_label->setVisible(true);
        } else {
            line_label = pixelPanelLabel(text, 11.0F,
                                     ax::Vec2(kContentWidth, line_h - 2.0F),
                                     statusColor(claim.status));
            line_label->setPosition(0.0F, y);
            knowledge_container_->addChild(line_label, 1);
            knowledge_lines_.push_back(line_label);
        }
        line_label->setTextColor(statusColor(claim.status));
        line_label->setPosition(0.0F, y);
        y -= line_h;
    }
}

} // namespace pf::ui
