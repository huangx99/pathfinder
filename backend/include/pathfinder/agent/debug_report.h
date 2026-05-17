#pragma once

#include "pathfinder/protocol/agent_projection.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::agent {

// --- ID Tag Types ---

struct AgentDebugReportIdTag {};
using AgentDebugReportId = pathfinder::foundation::StrongId<AgentDebugReportIdTag>;

// --- ID Helper ---

// Generate stable AgentDebugReportId: agent_debug_report_<query_id>_<item_count>
AgentDebugReportId makeAgentDebugReportId(
    const std::string& query_id,
    size_t item_count);

// --- Enums ---

enum class AgentDebugReportMode {
    Unknown,
    Public,
    Debug,
    Training
};

std::string toString(AgentDebugReportMode mode);
pathfinder::foundation::Result<AgentDebugReportMode> agentDebugReportModeFromString(const std::string& str);

enum class AgentDebugReportSeverity {
    Unknown,
    Info,
    Warning,
    Error
};

std::string toString(AgentDebugReportSeverity severity);
pathfinder::foundation::Result<AgentDebugReportSeverity> agentDebugReportSeverityFromString(const std::string& str);

// --- Data Contracts ---

struct AgentDebugReportItem {
    std::string record_id;
    std::string agent_id;
    std::string actor_id;
    int64_t tick = 0;
    std::string runtime_status;
    std::string decision_status;
    std::optional<std::string> command_id;
    std::optional<std::string> replay_lock_status;
    std::optional<std::string> training_sample_status;

    AgentDebugReportSeverity severity = AgentDebugReportSeverity::Info;
    std::vector<std::string> summary_keys;
    std::vector<std::string> reason_keys;
    std::vector<std::string> phase_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentDebugReport {
    AgentDebugReportId report_id;
    AgentDebugReportMode mode = AgentDebugReportMode::Unknown;
    size_t total_items = 0;
    size_t replay_locked_count = 0;
    size_t explained_only_count = 0;
    size_t broken_count = 0;
    size_t no_decision_count = 0;
    size_t skipped_count = 0;
    bool truncated = false;
    std::vector<AgentDebugReportItem> items;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- Builder ---

struct AgentDebugReportBuildRequest {
    pathfinder::protocol::AgentHistoryProjection projection;
    AgentDebugReportMode mode = AgentDebugReportMode::Public;
    bool include_trace_keys = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class AgentDebugReportBuilder {
public:
    pathfinder::foundation::Result<AgentDebugReport> build(
        const AgentDebugReportBuildRequest& request) const;
};

// --- Diagnostics ---

enum class AgentDiagnosticsStatus {
    Unknown,
    Clean,
    HasWarnings,
    HasErrors
};

std::string toString(AgentDiagnosticsStatus status);
pathfinder::foundation::Result<AgentDiagnosticsStatus> agentDiagnosticsStatusFromString(const std::string& str);

struct AgentDiagnosticsIssue {
    AgentDebugReportSeverity severity = AgentDebugReportSeverity::Unknown;
    std::string issue_key;
    std::optional<std::string> record_id;
    std::optional<std::string> agent_id;
    std::vector<std::string> detail_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentDiagnosticsSummary {
    AgentDiagnosticsStatus status = AgentDiagnosticsStatus::Unknown;
    size_t item_count = 0;
    size_t warning_count = 0;
    size_t error_count = 0;
    std::vector<AgentDiagnosticsIssue> issues;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class AgentDiagnosticsBuilder {
public:
    pathfinder::foundation::Result<AgentDiagnosticsSummary> build(
        const AgentDebugReport& report) const;
};

} // namespace pathfinder::agent
