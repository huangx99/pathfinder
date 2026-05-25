#pragma once

#include <map>
#include <string>
#include <vector>

namespace pathfinder::camp {

struct CampItemStackDefinition {
    std::string item_key;
    int quantity{0};
};

struct CampItemDefinition {
    std::string key;
    std::string display_name;
    std::map<std::string, double> numeric_states;
};

struct CampResourceDefinition {
    std::string key;
    std::string name;
    int current{0};
    int max{0};
    int regen_per_day{0};
    std::string entity_id;
    std::string entity_key;
    int x{0};
    int y{0};
    std::string gather_command_key;
    std::string gather_label;
    std::vector<CampItemStackDefinition> output_items;
    std::vector<CampItemStackDefinition> npc_output_items;
    std::string depleted_text;
    std::string completion_event;
};

struct CampActorDefinition {
    std::string actor_key;
    std::string entity_key;
    std::string name;
    int x{0};
    int y{0};
    int health{10};
    int max_health{10};
};

struct CampKnowledgeCardDefinition {
    std::string card_id;
    std::string knowledge_key;
    std::string name;
    std::string summary;
    std::string category{"通用"};
    std::string status{"active"};
    bool initial{true};
    bool recoverable{true};
    bool locked{false};
};

struct CampStaticEntityDefinition {
    std::string entity_id;
    std::string entity_key;
    std::string display_name;
    int x{0};
    int y{0};
    bool is_resource_node{false};
    int health{0};
    int max_health{0};
    bool alive{true};
};

struct CampThreatDefinition {
    std::string key;
    std::string name;
    std::string entity_id;
    std::string entity_key;
    int x{0};
    int y{0};
    int health{0};
    int max_health{0};
    int initial_level{0};
    int max_level{0};
    int trigger_level{1};
    int increase_interval_ticks{6};
    int repel_amount{1};
    int encounter_percent{55};
    int handle_duration{12};
    int return_duration{9};
    std::string repel_command_key;
    std::string repel_label;
    std::string counter_knowledge_key;
    std::string counter_item_key;
    int counter_item_quantity{1};
    std::string encounter_label;
    std::string success_reason_label;
    std::string failure_reason_without_item_label;
    std::string failure_reason_without_knowledge_label;
    std::string success_action_label;
    std::string failure_action_label;
    std::string resolved_node_label;
    std::string return_node_label;
    std::string risk_text;
    std::string resolved_text;
    std::string missing_item_blocked_reason;
    std::string missing_knowledge_blocked_reason;
    std::string success_reasoning_text;
    std::string missing_item_reasoning_text;
    std::string missing_knowledge_reasoning_text;
};

struct CampRecipeDefinition {
    std::string key;
    std::string command_key;
    std::string label;
    std::string work_label;
    std::string target_entity_id;
    std::string required_card_key;
    int duration{1};
    std::vector<CampItemStackDefinition> inputs;
    std::vector<CampItemStackDefinition> outputs;
    std::string missing_material_text;
    std::string completion_event;
};

struct CampTaskDefinition {
    std::string key;
    std::string command_key;
    std::string label;
    int duration{1};
    bool outside_safe_zone{false};
    std::string required_card_key;
    std::string resource_key;
    std::string recipe_key;
    std::string threat_key;
    std::vector<std::string> nodes;
};

struct CampContentPack {
    std::string schema_version;
    std::string source_path;
    std::string status_text;
    int player_inventory_capacity_slots{18};
    int actor_inventory_capacity_slots{9};
    std::vector<CampItemStackDefinition> initial_inventory;
    std::vector<std::string> initial_events;
    std::vector<CampStaticEntityDefinition> static_entities;
    std::map<std::string, CampItemDefinition> items;
    std::map<std::string, CampResourceDefinition> resources;
    std::map<std::string, CampActorDefinition> actors;
    std::map<std::string, CampKnowledgeCardDefinition> knowledge_cards;
    std::map<std::string, CampThreatDefinition> threats;
    std::map<std::string, CampRecipeDefinition> recipes;
    std::map<std::string, CampTaskDefinition> tasks;
    std::string primary_threat_key;
    std::string scholar_entity_id;
    std::string scholar_reward_card_id;
    std::string scholar_repeat_event;
};

CampContentPack loadDefaultCampContentPack();

} // namespace pathfinder::camp
