#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace pathfinder::v3_sandbox {

struct V3Coord {
    int x{0};
    int y{0};

    bool operator==(const V3Coord& other) const { return x == other.x && y == other.y; }
};

struct V3TerrainDefinition {
    std::string terrain_key;
    std::string display_name;
    bool walkable{true};
};

struct V3UnlockRuleDefinition {
    std::string unlock_key;
    std::string when_any_effect_known;
    std::string unlock_object;
    std::string event_text;
};

struct V3SandboxConfig {
    int width{16};
    int height{12};
    std::string default_terrain{"grass"};
    std::map<std::string, V3TerrainDefinition> terrains;
    std::set<std::string> initial_terrain_unlocks;
    std::set<std::string> initial_object_unlocks;
    std::set<std::string> initial_agent_unlocks;
    std::vector<V3UnlockRuleDefinition> unlock_rules;
    int perception_radius{1};
    double hunger_per_tick{8.0};
    int curiosity_chance_percent{65};
    double critical_hunger{60.0};
    double health_damage_scale{20.0};
};

enum class V3SandboxCommandKind {
    PaintTerrain,
    PlaceObject,
    PlaceAgent,
    Tick,
    InspectAgent,
    InspectCell
};

struct V3SandboxCommand {
    V3SandboxCommandKind kind{V3SandboxCommandKind::Tick};
    int x{0};
    int y{0};
    std::string terrain_key;
    std::string object_key;
    std::string agent_template_key{"basic_npc"};
    std::string agent_name;
    std::string agent_id;
    int tick_count{1};
};

struct V3SandboxCommandResult {
    bool accepted{false};
    std::string error;
    std::vector<std::string> events;
};

struct V3SandboxCellView {
    int x{0};
    int y{0};
    std::string terrain_key;
    std::string terrain_name;
    bool walkable{true};
    std::vector<std::string> object_instance_ids;
    std::vector<std::string> agent_ids;
};

struct V3ObjectInstanceView {
    std::string instance_id;
    std::string object_key;
    std::string display_name;
    int quantity{1};
    int x{0};
    int y{0};
};

struct V3KnowledgeClaimView {
    std::string subject_object_key;
    std::string action_key;
    std::string effect_key;
    std::string status;
    int evidence_count{0};
    double utility_delta{0.0};
    double risk_delta{0.0};
};

struct V3AgentView {
    std::string agent_id;
    std::string name;
    int x{0};
    int y{0};
    double hunger{0.0};
    double health{100.0};
    std::string current_intent;
    std::vector<V3KnowledgeClaimView> knowledge;
    std::vector<std::string> recent_memory;
};

struct V3SandboxSnapshot {
    int tick{0};
    int width{0};
    int height{0};
    std::vector<V3SandboxCellView> cells;
    std::vector<V3ObjectInstanceView> objects;
    std::vector<V3AgentView> agents;
    std::vector<std::string> unlocked_terrains;
    std::vector<std::string> unlocked_objects;
    std::vector<std::string> events;
};

} // namespace pathfinder::v3_sandbox
