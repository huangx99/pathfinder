#include "runtime/V3LocalClient.h"

#include <algorithm>

namespace pf::client {
namespace {
std::string labelForObject(const std::string& key) {
    if (key == "red_berry") return "投放红果";
    if (key == "decayed_red_berry") return "投放腐果";
    if (key == "stone_flake") return "投放石片";
    if (key == "bitter_leaf") return "投放苦叶";
    if (key == "wood") return "投放木头";
    return "投放 " + key;
}

std::string labelForTerrain(const std::string& key) {
    if (key == "grass") return "画草地";
    if (key == "soil") return "画泥土";
    if (key == "water") return "画浅水";
    return "画 " + key;
}
} // namespace

V3LocalClient::V3LocalClient()
    : runtime_(pathfinder::v3_sandbox::V3SandboxRuntime::createDefault(PATHFINDER_REPO_ROOT "/content")) {
    snapshot_ = runtime_.snapshot();
    rebuildTools();
}

void V3LocalClient::selectTool(int index) {
    if (index < 0 || index >= static_cast<int>(tools_.size())) return;
    selected_tool_index_ = index;
}

bool V3LocalClient::applySelectedToolToCell(int x, int y) {
    const auto& tool = selectedTool();
    pathfinder::v3_sandbox::V3SandboxCommand command;
    command.x = x;
    command.y = y;
    if (tool.kind == ToolKind::PaintTerrain) {
        command.kind = pathfinder::v3_sandbox::V3SandboxCommandKind::PaintTerrain;
        command.terrain_key = tool.key;
    } else if (tool.kind == ToolKind::PlaceObject) {
        command.kind = pathfinder::v3_sandbox::V3SandboxCommandKind::PlaceObject;
        command.object_key = tool.key;
    } else if (tool.kind == ToolKind::PlaceAgent) {
        command.kind = pathfinder::v3_sandbox::V3SandboxCommandKind::PlaceAgent;
        command.agent_template_key = "basic_npc";
        command.agent_name = "小人" + std::to_string(next_agent_name_++);
    } else {
        last_error_ = "tool_not_supported_on_map";
        return false;
    }
    return submit(command);
}

bool V3LocalClient::tick(int count) {
    pathfinder::v3_sandbox::V3SandboxCommand command;
    command.kind = pathfinder::v3_sandbox::V3SandboxCommandKind::Tick;
    command.tick_count = count;
    return submit(command);
}

bool V3LocalClient::inspectCell(int x, int y, std::vector<std::string>& lines) {
    pathfinder::v3_sandbox::V3SandboxCommand command;
    command.kind = pathfinder::v3_sandbox::V3SandboxCommandKind::InspectCell;
    command.x = x;
    command.y = y;
    auto result = runtime_.submit(command);
    if (!result.accepted) {
        last_error_ = result.error;
        return false;
    }
    lines = result.events;
    return true;
}

bool V3LocalClient::inspectAgent(const std::string& agent_id, std::vector<std::string>& lines) {
    pathfinder::v3_sandbox::V3SandboxCommand command;
    command.kind = pathfinder::v3_sandbox::V3SandboxCommandKind::InspectAgent;
    command.agent_id = agent_id;
    auto result = runtime_.submit(command);
    if (!result.accepted) {
        last_error_ = result.error;
        return false;
    }
    lines = result.events;
    return true;
}

const pathfinder::v3_sandbox::V3AgentView* V3LocalClient::agentAtCell(int x, int y) const {
    for (const auto& agent : snapshot_.agents) {
        if (agent.x == x && agent.y == y) return &agent;
    }
    return nullptr;
}

const pathfinder::v3_sandbox::V3AgentView* V3LocalClient::findAgent(const std::string& agent_id) const {
    for (const auto& agent : snapshot_.agents) {
        if (agent.agent_id == agent_id) return &agent;
    }
    return nullptr;
}

bool V3LocalClient::submit(pathfinder::v3_sandbox::V3SandboxCommand command) {
    auto result = runtime_.submit(command);
    if (!result.accepted) {
        last_error_ = result.error;
        snapshot_ = runtime_.snapshot();
        return false;
    }
    last_error_.clear();
    snapshot_ = runtime_.snapshot();
    rebuildTools();
    if (selected_tool_index_ >= static_cast<int>(tools_.size())) selected_tool_index_ = 0;
    return true;
}

void V3LocalClient::rebuildTools() {
    tools_.clear();
    for (const auto& terrain : snapshot_.unlocked_terrains) {
        tools_.push_back({ToolKind::PaintTerrain, terrain, labelForTerrain(terrain)});
    }
    for (const auto& object : snapshot_.unlocked_objects) {
        tools_.push_back({ToolKind::PlaceObject, object, labelForObject(object)});
    }
    tools_.push_back({ToolKind::PlaceAgent, "basic_npc", "投放小人"});
}

} // namespace pf::client
