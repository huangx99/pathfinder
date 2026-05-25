#pragma once

#include "pathfinder/v3_sandbox/v3_sandbox_runtime.h"

#include <string>
#include <vector>

namespace pf::client {

enum class ToolKind {
    PaintTerrain,
    PlaceObject,
    PlaceAgent
};

struct ToolDefinition {
    ToolKind kind{ToolKind::PaintTerrain};
    std::string key;
    std::string label;
    std::string category;
};

class V3LocalClient {
public:
    V3LocalClient();

    const pathfinder::v3_sandbox::V3SandboxSnapshot& snapshot() const { return snapshot_; }
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
    const pathfinder::v3_sandbox::V3AgentView* agentAtCell(int x, int y) const;
    const pathfinder::v3_sandbox::V3AgentView* findAgent(const std::string& agent_id) const;

private:
    bool submit(pathfinder::v3_sandbox::V3SandboxCommand command);
    void rebuildTools();

    pathfinder::v3_sandbox::V3SandboxRuntime runtime_;
    pathfinder::v3_sandbox::V3SandboxSnapshot snapshot_;
    std::vector<ToolDefinition> tools_;
    int selected_tool_index_{-1};
    int next_agent_name_{1};
    std::string last_error_;
};

} // namespace pf::client
