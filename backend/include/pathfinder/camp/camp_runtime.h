#pragma once

#include "pathfinder/camp/camp_content.h"
#include "pathfinder/world_command/world_command_types.h"
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::camp {

struct CampCellView {
    std::string cell_id;
    int x{0};
    int y{0};
    std::string terrain_key{"camp_grass"};
    bool blocks_movement{false};
};

struct CampEntityView {
    std::string entity_id;
    std::string entity_key;
    std::string display_name;
    std::string actor_key;
    int x{0};
    int y{0};
    bool is_resource_node{false};
    int health{0};
    int max_health{0};
    bool alive{true};
};

struct CampInventoryItemView {
    std::string entry_id;
    std::string entity_key;
    std::string display_name;
    int quantity{1};
    std::map<std::string, double> numeric_states;
};

struct CampKnowledgeView {
    std::string knowledge_id;
    std::string subject_id;
    std::string action_key;
    std::string effect_key;
    std::string status;
    std::string subject_name;
    std::string action_name;
    std::string effect_name;
    std::string summary_text;
    std::string recipe_text;
    std::string actor_key;
};

struct CampKnowledgeCardView {
    std::string card_id;
    std::string knowledge_key;
    std::string name;
    std::string summary;
    std::string status;
    std::string category;
    std::string assigned_actor_key;
    std::string assigned_actor_name;
    bool recoverable{true};
    bool locked{false};
};

struct CampTaskGraphNodeView {
    std::string node_id;
    std::string label;
    std::string node_type;
    std::string status;
    float x{0.0F};
    float y{0.0F};
};

struct CampTaskGraphEdgeView {
    std::string edge_id;
    std::string from_node_id;
    std::string to_node_id;
    std::string edge_type;
    std::string status;
};

struct CampTaskGraphView {
    std::vector<CampTaskGraphNodeView> nodes;
    std::vector<CampTaskGraphEdgeView> edges;
    std::string active_node_id;
    std::string graph_mode;
};

struct CampActorTaskView {
    bool active{false};
    std::string task_key;
    std::string label;
    std::vector<std::string> nodes;
    int node_index{0};
    int progress{0};
    int duration{1};
    int percent{0};
    bool outside_safe_zone{false};
    bool handling_threat{false};
    bool returning_home{false};
    bool risk_encountered{false};
    bool risk_resolved{false};
    std::string required_card_key;
    std::string blocked_reason;
    std::string risk_text;
    std::string reasoning_text;
    CampTaskGraphView task_graph;
};

struct CampCommandOptionView {
    std::string option_id;
    std::string command_key;
    std::string label_text;
    pathfinder::world_command::WorldCommandKind command_kind{pathfinder::world_command::WorldCommandKind::Unknown};
    bool enabled{true};
    std::optional<int> target_x;
    std::optional<int> target_y;
    std::string target_entity_id;
    std::string target_actor_key;
    std::string knowledge_key;
};

struct CampEventView {
    std::string event_kind;
    std::string title_text;
    std::string body_text;
    std::string actor_key;
    std::string target_entity_id;
};

struct CampSnapshot {
    uint64_t projection_version{0};
    std::string status_text;
    std::vector<CampCellView> cells;
    std::vector<CampEntityView> entities;
    std::vector<CampCommandOptionView> options;
    std::vector<CampInventoryItemView> player_inventory;
    std::map<std::string, std::vector<CampInventoryItemView>> actor_inventory;
    std::map<std::string, int> actor_inventory_capacity_slots;
    std::vector<CampKnowledgeView> knowledge;
    std::vector<CampKnowledgeCardView> knowledge_cards;
    std::map<std::string, CampActorTaskView> actor_tasks;
    std::map<std::string, std::string> actor_work_status;
    std::vector<std::string> event_feed;
    int player_inventory_capacity_slots{18};
};

struct CampResourcePool {
    std::string key;
    std::string name;
    int current{0};
    int max{0};
    int reserved{0};
    int regen_per_day{0};
};

struct CampKnowledgeCard {
    std::string card_id;
    std::string knowledge_key;
    std::string name;
    std::string summary;
    std::string category{"通用"};
    std::string status{"active"};
    std::string assigned_actor_key;
    bool recoverable{true};
    bool locked{false};
};

struct CampNpcTask {
    bool active{false};
    std::string task_key;
    std::string label;
    std::vector<std::string> nodes;
    int node_index{0};
    int progress{0};
    int duration{1};
    bool outside_safe_zone{false};
    bool handling_threat{false};
    bool returning_home{false};
    bool risk_encountered{false};
    bool risk_resolved{false};
    std::string required_card_key;
    std::string resource_key;
    std::string recipe_key;
    std::string threat_key;
    std::string blocked_reason;
    std::string risk_text;
    std::string reasoning_text;
    std::string risk_event_label;
    std::string risk_success_reason_label;
    std::string risk_failure_reason_label;
    std::string risk_success_action_label;
    std::string risk_failure_action_label;
    std::string risk_resolved_node_label;
    std::string risk_return_node_label;
    int resume_progress{0};
    int resume_duration{1};
};

struct CampActorState {
    std::string actor_key;
    std::string name;
    int x{0};
    int y{0};
    int health{10};
    int max_health{10};
    std::map<std::string, int> inventory;
    CampNpcTask task;
};

struct CampRuntimeState {
    uint64_t tick{0};
    uint64_t version{1};
    int day_tick{0};
    int threat_level{1};
    std::map<std::string, CampResourcePool> resources;
    std::map<std::string, int> player_inventory;
    std::map<std::string, CampActorState> actors;
    std::vector<CampKnowledgeCard> cards;
    std::vector<std::string> events;
    std::string status_text;
};

class CampRuntime;

class CampRule {
public:
    virtual ~CampRule() = default;
    virtual std::string ruleKey() const = 0;
    virtual void buildOptions(const CampRuntime& runtime, std::vector<CampCommandOptionView>& options) const = 0;
    virtual bool canHandle(const CampCommandOptionView& option) const = 0;
    virtual bool execute(CampRuntime& runtime, const CampCommandOptionView& option, std::string& error) const = 0;
};

class CampRuntime final {
public:
    CampRuntime();

    void reset();
    CampSnapshot snapshot();
    bool submitOption(const std::string& option_id, std::string& error);
    bool assignKnowledgeCard(const std::string& actor_key, const std::string& card_id, std::string& error);
    bool recoverKnowledgeCard(const std::string& actor_key, const std::string& card_id, std::string& error);

    CampRuntimeState& state() { return state_; }
    const CampRuntimeState& state() const { return state_; }
    const CampContentPack& content() const { return content_; }
    const CampThreatDefinition* primaryThreatDefinition() const { return primaryThreat(); }

    void registerRule(std::unique_ptr<CampRule> rule);
    void markChanged(const std::string& status);
    void addEvent(const std::string& text, const std::string& actor_key = {}, const std::string& kind = "CampEvent");
    bool hasCard(const std::string& actor_key, const std::string& knowledge_key) const;
    bool hasItem(const std::string& actor_key, const std::string& item_key) const;
    int itemCount(const std::string& actor_key, const std::string& item_key) const;
    void addItem(const std::string& actor_key, const std::string& item_key, int amount);
    bool consumeItem(const std::string& actor_key, const std::string& item_key, int amount);
    bool hasItems(const std::string& actor_key, const std::vector<CampItemStackDefinition>& stacks) const;
    bool consumeItems(const std::string& actor_key, const std::vector<CampItemStackDefinition>& stacks);
    void addItems(const std::string& actor_key, const std::vector<CampItemStackDefinition>& stacks);
    void startNpcTask(const std::string& actor_key, CampNpcTask task);
    void advanceOneTick();
    std::string actorName(const std::string& actor_key) const;
    std::string cardOwnerLabel(const CampKnowledgeCard& card) const;

private:
    void seedInitialState();
    std::vector<CampCommandOptionView> buildOptions() const;
    CampSnapshot buildSnapshot(const std::vector<CampCommandOptionView>& options) const;
    CampInventoryItemView makeItem(const std::string& owner, const std::string& key, int quantity) const;
    const CampThreatDefinition* findThreat(const std::string& key) const;
    const CampThreatDefinition* primaryThreat() const;
    void advanceNpcTask(CampActorState& actor);
    void completeNpcTask(CampActorState& actor);
    CampRuntimeState state_;
    CampContentPack content_;
    std::vector<std::unique_ptr<CampRule>> rules_;
    std::map<std::string, CampCommandOptionView> last_options_;
};

void registerCampCoreRules(CampRuntime& runtime);

} // namespace pathfinder::camp
