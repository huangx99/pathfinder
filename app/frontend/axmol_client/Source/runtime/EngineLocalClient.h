#pragma once

#include "pathfinder/client_runtime_host/client_runtime_host_factory.h"
#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/world_command/world_command_types.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pf::client {

struct EngineCellView {
    int x{0};
    int y{0};
    std::string cell_id;
    std::string terrain_key{"plain"};
    bool blocks_movement{false};
    std::vector<std::string> entity_ids;
};

struct EngineEntityView {
    std::string entity_id;
    std::string entity_key;
    std::string display_name;
    int x{0};
    int y{0};
    bool blocks_movement{false};
    bool resource_node{false};
    int remaining_charges{0};
    std::map<std::string, double> numeric_states;
};

struct EngineInventoryItemView {
    std::string entry_id;
    std::string entity_id;
    std::string object_key;
    std::string display_name;
    int quantity{0};
    std::map<std::string, double> numeric_states;
};

struct EngineKnowledgeClaimView {
    std::string subject_object_key;
    std::string action_key;
    std::string effect_key;
    std::string status{"已记录"};
    int evidence_count{1};
    double utility_delta{0.0};
    double risk_delta{0.0};
};

struct EngineAgentView {
    std::string agent_id;
    std::string name;
    int x{0};
    int y{0};
    double health{10.0};
    double max_health{10.0};
    double hunger{0.0};
    std::string current_intent{"等待指令"};
    std::vector<EngineInventoryItemView> inventory;
    std::vector<EngineKnowledgeClaimView> knowledge;
    std::vector<std::string> recent_memory;
};

struct EngineSnapshot {
    uint64_t tick{0};
    uint64_t projection_version{0};
    int width{0};
    int height{0};
    int min_x{0};
    int min_y{0};
    std::vector<EngineCellView> cells;
    std::vector<EngineEntityView> entities;
    std::vector<EngineAgentView> agents;
    std::vector<std::string> events;
    std::vector<pathfinder::world_command::WorldCommandOptionDto> available_commands;
};

enum class ToolKind {
    CommandOption
};

struct ToolDefinition {
    ToolKind kind{ToolKind::CommandOption};
    std::string key;
    std::string label;
    std::string category;
    pathfinder::world_command::WorldCommandKind command_kind{pathfinder::world_command::WorldCommandKind::Unknown};
    std::string target_item_key;
};

class EngineLocalClient {
public:
    EngineLocalClient();

    const EngineSnapshot& snapshot() const { return snapshot_; }
    const std::vector<ToolDefinition>& tools() const { return tools_; }
    bool hasSelectedTool() const { return selected_tool_index_ >= 0 && selected_tool_index_ < static_cast<int>(tools_.size()); }
    const ToolDefinition* selectedTool() const { return hasSelectedTool() ? &tools_[selected_tool_index_] : nullptr; }
    int selectedToolIndex() const { return selected_tool_index_; }
    const std::string& lastError() const { return last_error_; }

    void selectTool(int index);
    void clearToolSelection();
    bool applySelectedToolToCell(int x, int y);
    bool tick(int count = 1);
    bool inspectCell(int x, int y, std::vector<std::string>& lines);
    bool inspectAgent(const std::string& agent_id, std::vector<std::string>& lines);
    const EngineAgentView* agentAtCell(int x, int y) const;
    const EngineAgentView* findAgent(const std::string& agent_id) const;

private:
    bool bootstrap();
    bool refresh();
    bool submitOption(const std::string& option_id);
    void applyBootstrap(const pathfinder::client_protocol::ClientBootstrapResponse& response);
    void applyRefresh(const pathfinder::client_protocol::ClientRefreshResponse& response);
    void applyCommand(const pathfinder::client_protocol::ClientCommandResponse& response);
    void rebuildSnapshot(const pathfinder::client_protocol::ClientWorldProjection& projection);
    void rebuildTools();
    std::optional<pathfinder::world_command::WorldCommandOptionDto> findSelectedToolOptionForCell(int x, int y) const;
    std::optional<pathfinder::world_command::WorldCommandOptionDto> findOptionForCell(int x, int y) const;
    std::optional<pathfinder::world_command::WorldCommandOptionDto> findOptionByKind(pathfinder::world_command::WorldCommandKind kind) const;

    pathfinder::client_runtime_host::ClientRuntimeHostFactory host_;
    EngineSnapshot snapshot_;
    std::vector<ToolDefinition> tools_;
    int selected_tool_index_{-1};
    uint64_t client_sequence_{0};
    std::string client_id_{"axmol_local"};
    std::string session_id_{"axmol_session"};
    std::string actor_key_{"player"};
    std::string layer_key_{"surface"};
    std::string last_error_;
};

} // namespace pf::client
