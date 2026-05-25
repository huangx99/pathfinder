#include "pathfinder/camp/camp_runtime.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::camp;

namespace {

const CampTaskGraphNodeView* findNode(const CampTaskGraphView& graph, const std::string& node_id) {
    for (const auto& node : graph.nodes) {
        if (node.node_id == node_id) return &node;
    }
    return nullptr;
}

std::string optionFor(const CampSnapshot& snapshot, const std::string& command_key) {
    for (const auto& option : snapshot.options) {
        if (option.command_key == command_key) return option.option_id;
    }
    return {};
}


const CampInventoryItemView* findInventory(const std::vector<CampInventoryItemView>& items, const std::string& entity_key) {
    for (const auto& item : items) {
        if (item.entity_key == entity_key) return &item;
    }
    return nullptr;
}

const CampKnowledgeCardView* findCard(const CampSnapshot& snapshot, const std::string& card_id) {
    for (const auto& card : snapshot.knowledge_cards) {
        if (card.card_id == card_id) return &card;
    }
    return nullptr;
}

void run_content_pack_projection_test() {
    CampRuntime runtime;
    assert(runtime.content().source_path.find("basic_camp.json") != std::string::npos);
    assert(runtime.content().player_inventory_capacity_slots == 18);
    assert(runtime.content().actor_inventory_capacity_slots == 9);
    runtime.addItem("camp", "axe", 1);
    auto snapshot = runtime.snapshot();
    assert(snapshot.player_inventory_capacity_slots == runtime.content().player_inventory_capacity_slots);
    assert(snapshot.actor_inventory_capacity_slots.at("npc_amu") == runtime.content().actor_inventory_capacity_slots);
    const auto* axe = findInventory(snapshot.player_inventory, "axe");
    assert(axe != nullptr);
    assert(axe->display_name == runtime.content().items.at("axe").display_name);
    assert(axe->numeric_states.at("durability") == runtime.content().items.at("axe").numeric_states.at("durability"));
    const auto* card = findCard(snapshot, "card_chop_wood");
    assert(card != nullptr);
    assert(card->category == runtime.content().knowledge_cards.at("card_chop_wood").category);
}

void run_content_driven_player_command_test() {
    CampRuntime runtime;
    std::string error;
    const auto wood_command = runtime.content().resources.at("nearby_woods").gather_command_key;
    const auto stone_command = runtime.content().resources.at("stone_field").gather_command_key;
    auto snapshot = runtime.snapshot();
    assert(optionFor(snapshot, wood_command) != "");
    assert(runtime.submitOption(optionFor(snapshot, wood_command), error));
    snapshot = runtime.snapshot();
    assert(optionFor(snapshot, stone_command) != "");
    assert(runtime.submitOption(optionFor(snapshot, stone_command), error));
    snapshot = runtime.snapshot();
    const auto craft_axe_command = runtime.content().recipes.at("craft_axe").command_key;
    assert(optionFor(snapshot, craft_axe_command) != "");
    assert(runtime.submitOption(optionFor(snapshot, craft_axe_command), error));
    snapshot = runtime.snapshot();
    assert(findInventory(snapshot.player_inventory, "axe") != nullptr);
    assert(optionFor(snapshot, "camp.camp_repel_beast") == "");
    const auto* threat = runtime.primaryThreatDefinition();
    assert(threat != nullptr);
    assert(optionFor(snapshot, threat->repel_command_key) != "");
}

void run_content_driven_scholar_reward_test() {
    CampRuntime runtime;
    std::string error;
    auto snapshot = runtime.snapshot();
    const auto option_id = optionFor(snapshot, "camp.scholar_inspect");
    assert(!option_id.empty());
    assert(runtime.submitOption(option_id, error));
    snapshot = runtime.snapshot();
    const auto* reward = findCard(snapshot, runtime.content().scholar_reward_card_id);
    assert(reward != nullptr);
    assert(reward->name == runtime.content().knowledge_cards.at(runtime.content().scholar_reward_card_id).name);
}

void run_success_branch_test() {
    CampRuntime runtime;
    std::string error;
    assert(runtime.assignKnowledgeCard("npc_amu", "card_chop_wood", error));
    assert(runtime.assignKnowledgeCard("npc_amu", "card_repel_beast", error));

    auto snapshot = runtime.snapshot();
    const auto option_id = optionFor(snapshot, "camp.npc_gather_wood");
    assert(!option_id.empty());
    assert(runtime.submitOption(option_id, error));

    snapshot = runtime.snapshot();
    auto task = snapshot.actor_tasks.at("npc_amu");
    assert(task.task_graph.graph_mode == "linear");
    assert(findNode(task.task_graph, "risk_encounter") == nullptr);

    for (int i = 0; i < 13; ++i) runtime.advanceOneTick();
    snapshot = runtime.snapshot();
    task = snapshot.actor_tasks.at("npc_amu");
    assert(task.risk_encountered);
    assert(task.task_graph.graph_mode == "branching");
    assert(findNode(task.task_graph, "risk_encounter") != nullptr);
    assert(findNode(task.task_graph, "risk_reason") != nullptr);
    assert(findNode(task.task_graph, "risk_action") != nullptr);
    const auto first_active_node = task.task_graph.active_node_id;

    for (int i = 0; i < 4; ++i) runtime.advanceOneTick();
    snapshot = runtime.snapshot();
    task = snapshot.actor_tasks.at("npc_amu");
    assert(task.task_graph.active_node_id != first_active_node);

    for (int i = 0; i < 12; ++i) runtime.advanceOneTick();
    snapshot = runtime.snapshot();
    task = snapshot.actor_tasks.at("npc_amu");
    assert(task.task_graph.active_node_id.rfind("main_", 0) == 0);
    assert(findNode(task.task_graph, "risk_action")->status == "completed");

    for (int i = 0; i < 20; ++i) runtime.advanceOneTick();
    snapshot = runtime.snapshot();
    task = snapshot.actor_tasks.at("npc_amu");
    assert(!task.active);
    assert(!task.task_graph.nodes.empty());
    assert(snapshot.actor_work_status.at("npc_amu").find("已完成") != std::string::npos);
}

void run_return_home_branch_test() {
    CampRuntime runtime;
    std::string error;
    assert(runtime.assignKnowledgeCard("npc_amu", "card_chop_wood", error));

    auto snapshot = runtime.snapshot();
    const auto option_id = optionFor(snapshot, "camp.npc_gather_wood");
    assert(!option_id.empty());
    assert(runtime.submitOption(option_id, error));

    for (int i = 0; i < 13; ++i) runtime.advanceOneTick();
    snapshot = runtime.snapshot();
    const auto task = snapshot.actor_tasks.at("npc_amu");
    assert(task.risk_encountered);
    assert(task.returning_home);
    assert(findNode(task.task_graph, "risk_action") != nullptr);
    assert(findNode(task.task_graph, "risk_action")->node_type == "fail");
}

} // namespace

int main() {
    run_content_pack_projection_test();
    run_content_driven_player_command_test();
    run_content_driven_scholar_reward_test();
    run_success_branch_test();
    run_return_home_branch_test();
    std::cout << "camp_task_graph_test: passed" << std::endl;
    return 0;
}
