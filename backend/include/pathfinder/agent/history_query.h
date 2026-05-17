#pragma once

#include "pathfinder/agent/record_types.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/result.h"
#include <vector>
#include <string>
#include <optional>

namespace pathfinder::agent {

// --- ID Tag Types ---

struct AgentHistoryQueryIdTag {};
using AgentHistoryQueryId = pathfinder::foundation::StrongId<AgentHistoryQueryIdTag>;

// --- ID Helper ---

// Generate stable AgentHistoryQueryId: agent_history_query_<agent_id_or_all>_<tick_from>_<tick_to>_<limit>
AgentHistoryQueryId makeAgentHistoryQueryId(
    const std::optional<AgentId>& agent_id,
    const std::optional<pathfinder::foundation::Tick>& tick_from,
    const std::optional<pathfinder::foundation::Tick>& tick_to,
    size_t limit);

// --- Enums ---

enum class AgentHistorySortOrder {
    Unknown,
    TickAscending,
    TickDescending
};

std::string toString(AgentHistorySortOrder order);
pathfinder::foundation::Result<AgentHistorySortOrder> agentHistorySortOrderFromString(const std::string& str);

enum class AgentHistoryProjectionMode {
    Unknown,
    Public,
    Debug,
    Training
};

std::string toString(AgentHistoryProjectionMode mode);
pathfinder::foundation::Result<AgentHistoryProjectionMode> agentHistoryProjectionModeFromString(const std::string& str);

// --- Data Contracts ---

struct AgentHistoryQueryOptions {
    AgentHistoryQueryId query_id;
    std::optional<AgentId> agent_id;
    std::optional<pathfinder::foundation::Tick> tick_from;
    std::optional<pathfinder::foundation::Tick> tick_to;
    AgentHistorySortOrder sort_order = AgentHistorySortOrder::TickAscending;
    AgentHistoryProjectionMode projection_mode = AgentHistoryProjectionMode::Public;
    size_t limit = 100;
    bool include_replay_lock = true;
    bool include_training_sample = true;
    bool include_debug_trace = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentHistoryQueryResult {
    AgentHistoryQueryId query_id;
    std::vector<AgentTickRecord> records;
    size_t total_matched = 0;
    bool truncated = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- Query Service ---

class AgentHistoryQueryService {
public:
    pathfinder::foundation::Result<AgentHistoryQueryResult> query(
        const AgentTickLog& log,
        const AgentHistoryQueryOptions& options) const;
};

} // namespace pathfinder::agent
