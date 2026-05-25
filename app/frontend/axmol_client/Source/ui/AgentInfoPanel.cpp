#include "ui/AgentInfoPanel.h"
#include "ui/PixelUI.h"
#include "procedural/ProceduralArt.h"

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
constexpr float kGridX = (kWidth - kGridWidth) * 0.5F;
constexpr float kIconSize = 32.0F;

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
    // 背景
    bg_ = pixelPanelBackground(getContentSize());
    addChild(bg_, 0);

    // 名字
    name_label_ = pixelPanelLabel("", 18.0F, ax::Vec2(kWidth - 24.0F, 26.0F), PixelPalette::TextPrimary);
    name_label_->setPosition(12.0F, kHeight - 10.0F);
    addChild(name_label_, 2);

    // 心形图标 + 血条
    auto* heart = createHeartIcon(20.0F);
    heart->setPosition(12.0F, kHeight - 42.0F);
    addChild(heart, 2);

    health_bar_ = pixelProgressBar(kWidth - 48.0F, 14.0F, 1.0F,
                                    PixelPalette::HealthFill,
                                    PixelPalette::PanelFill,
                                    PixelPalette::HealthBorder);
    health_bar_->setPosition(40.0F, kHeight - 52.0F);
    addChild(health_bar_, 2);

    health_text_ = pixelLabel("100%", 11.0F, ax::Vec2(40.0F, 14.0F), PixelPalette::TextPrimary);
    health_text_->setAlignment(ax::TextHAlignment::CENTER, ax::TextVAlignment::CENTER);
    health_text_->setPosition(kWidth - 28.0F, kHeight - 45.0F);
    addChild(health_text_, 3);

    // 鸡腿图标 + 饥饿条
    auto* hunger = createHungerIcon(20.0F);
    hunger->setPosition(12.0F, kHeight - 70.0F);
    addChild(hunger, 2);

    hunger_bar_ = pixelProgressBar(kWidth - 48.0F, 14.0F, 1.0F,
                                    PixelPalette::HungerFill,
                                    PixelPalette::PanelFill,
                                    PixelPalette::HungerBorder);
    hunger_bar_->setPosition(40.0F, kHeight - 80.0F);
    addChild(hunger_bar_, 2);

    hunger_text_ = pixelLabel("100%", 11.0F, ax::Vec2(40.0F, 14.0F), PixelPalette::TextPrimary);
    hunger_text_->setAlignment(ax::TextHAlignment::CENTER, ax::TextVAlignment::CENTER);
    hunger_text_->setPosition(kWidth - 28.0F, kHeight - 73.0F);
    addChild(hunger_text_, 3);

    // 背包标题
    auto* inv_title = pixelPanelLabel("背包", 14.0F, ax::Vec2(60.0F, 20.0F), PixelPalette::TextSecondary);
    inv_title->setPosition(12.0F, kHeight - 96.0F);
    addChild(inv_title, 2);

    // 背包网格
    inventory_container_ = ax::Node::create();
    inventory_container_->setPosition(kGridX, kHeight - 118.0F - kGridHeight);
    addChild(inventory_container_, 2);

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

        // 数量角标
        auto* qty = pixelLabel("", 10.0F, ax::Vec2(20.0F, 12.0F), pixelColor(255, 255, 255));
        qty->setAlignment(ax::TextHAlignment::RIGHT, ax::TextVAlignment::BOTTOM);
        qty->setPosition(x + kSlotSize - 2.0F, y + 2.0F);
        qty->setName("qty_" + std::to_string(i));
        inventory_container_->addChild(qty, 3);
        slot_quantity_labels_[i] = qty;

        // Touch 监听（用于选中 + 长按 tooltip）
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

    // 选中物品信息
    selected_item_label_ = pixelPanelLabel("", 12.0F, ax::Vec2(kWidth - 24.0F, 18.0F), PixelPalette::TextSecondary);
    selected_item_label_->setPosition(12.0F, kHeight - 118.0F - kGridHeight - 18.0F);
    addChild(selected_item_label_, 2);

    // 知识标题
    knowledge_title_ = pixelPanelLabel("知识", 14.0F, ax::Vec2(60.0F, 20.0F), PixelPalette::TextSecondary);
    knowledge_title_->setPosition(12.0F, kHeight - 118.0F - kGridHeight - 40.0F);
    addChild(knowledge_title_, 2);

    // 知识容器
    knowledge_container_ = ax::Node::create();
    knowledge_container_->setPosition(12.0F, kHeight - 118.0F - kGridHeight - 44.0F);
    addChild(knowledge_container_, 2);
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
    const float slot_x = kGridX + col * (kSlotSize + kSlotGap);
    const float slot_y = getContentSize().height - 118.0F - kGridHeight + (3 - row) * (kSlotSize + kSlotGap);
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
    const size_t max_lines = 5;
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
    const float line_h = 16.0F;
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
            line_label = pixelLabel(text, 11.0F,
                                     ax::Vec2(kWidth - 28.0F, line_h),
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
