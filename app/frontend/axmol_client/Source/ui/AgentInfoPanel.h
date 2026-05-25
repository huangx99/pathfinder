#pragma once

#include "axmol/axmol.h"
#include "pathfinder/v3_sandbox/v3_sandbox_types.h"

#include <string>
#include <vector>

namespace pf::ui {

class AgentInfoPanel final : public ax::Node {
public:
    static AgentInfoPanel* create();
    bool init() override;
    void setAgent(const pathfinder::v3_sandbox::V3AgentView* agent);

private:
    void buildSkeleton();
    void updateContent(const pathfinder::v3_sandbox::V3AgentView* agent);
    void updateHealth(double value);
    void updateHunger(double value);
    void updateInventory(const std::vector<pathfinder::v3_sandbox::V3InventoryItemView>& inventory);
    void updateKnowledge(const pathfinder::v3_sandbox::V3AgentView* agent);
    void onSlotTouched(int slot_index);
    void showTooltip(int slot_index);
    void hideTooltip();

    ax::Node* createInventorySlot(int index, float size);
    ax::Node* createItemIcon(const std::string& key, float size);

    // 骨架节点（init 创建后不再删除）
    ax::Node* bg_{nullptr};
    ax::Node* health_bar_{nullptr};
    ax::Node* hunger_bar_{nullptr};
    ax::Node* inventory_container_{nullptr};
    ax::Node* knowledge_container_{nullptr};
    ax::Node* tooltip_{nullptr};

    // 动态内容节点
    ax::Label* name_label_{nullptr};
    ax::Label* health_text_{nullptr};
    ax::Label* hunger_text_{nullptr};
    ax::Label* selected_item_label_{nullptr};
    ax::Label* knowledge_title_{nullptr};
    std::vector<ax::Node*> slot_nodes_;
    std::vector<ax::Label*> slot_quantity_labels_;
    std::vector<ax::Label*> knowledge_lines_;

    // 状态
    std::string current_agent_id_;
    std::string selected_item_key_;
    std::vector<pathfinder::v3_sandbox::V3InventoryItemView> current_inventory_;
    pathfinder::v3_sandbox::V3AgentView current_agent_data_;
    bool has_agent_{false};
    int hovered_slot_{-1};
};

} // namespace pf::ui
