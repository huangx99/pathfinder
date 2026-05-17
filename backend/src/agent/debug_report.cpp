#include "pathfinder/agent/debug_report.h"
#include <sstream>

namespace pathfinder::agent {

// --- ID Helper ---

AgentDebugReportId makeAgentDebugReportId(
    const std::string& query_id,
    size_t item_count) {
    std::ostringstream oss;
    oss << "agent_debug_report_" << query_id << "_" << item_count;
    return AgentDebugReportId(oss.str());
}

// --- Enum toString / fromString ---

std::string toString(AgentDebugReportMode mode) {
    switch (mode) {
        case AgentDebugReportMode::Unknown: return "Unknown";
        case AgentDebugReportMode::Public: return "Public";
        case AgentDebugReportMode::Debug: return "Debug";
        case AgentDebugReportMode::Training: return "Training";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugReportMode> agentDebugReportModeFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDebugReportMode>::ok(AgentDebugReportMode::Unknown);
    if (str == "Public") return pathfinder::foundation::Result<AgentDebugReportMode>::ok(AgentDebugReportMode::Public);
    if (str == "Debug") return pathfinder::foundation::Result<AgentDebugReportMode>::ok(AgentDebugReportMode::Debug);
    if (str == "Training") return pathfinder::foundation::Result<AgentDebugReportMode>::ok(AgentDebugReportMode::Training);
    return pathfinder::foundation::Result<AgentDebugReportMode>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent_debug_report_mode_unknown",
            "Unknown AgentDebugReportMode: " + str));
}

std::string toString(AgentDebugReportSeverity severity) {
    switch (severity) {
        case AgentDebugReportSeverity::Unknown: return "Unknown";
        case AgentDebugReportSeverity::Info: return "Info";
        case AgentDebugReportSeverity::Warning: return "Warning";
        case AgentDebugReportSeverity::Error: return "Error";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDebugReportSeverity> agentDebugReportSeverityFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDebugReportSeverity>::ok(AgentDebugReportSeverity::Unknown);
    if (str == "Info") return pathfinder::foundation::Result<AgentDebugReportSeverity>::ok(AgentDebugReportSeverity::Info);
    if (str == "Warning") return pathfinder::foundation::Result<AgentDebugReportSeverity>::ok(AgentDebugReportSeverity::Warning);
    if (str == "Error") return pathfinder::foundation::Result<AgentDebugReportSeverity>::ok(AgentDebugReportSeverity::Error);
    return pathfinder::foundation::Result<AgentDebugReportSeverity>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent_debug_report_severity_unknown",
            "Unknown AgentDebugReportSeverity: " + str));
}

std::string toString(AgentDiagnosticsStatus status) {
    switch (status) {
        case AgentDiagnosticsStatus::Unknown: return "Unknown";
        case AgentDiagnosticsStatus::Clean: return "Clean";
        case AgentDiagnosticsStatus::HasWarnings: return "HasWarnings";
        case AgentDiagnosticsStatus::HasErrors: return "HasErrors";
        default: return "Unknown";
    }
}

pathfinder::foundation::Result<AgentDiagnosticsStatus> agentDiagnosticsStatusFromString(const std::string& str) {
    if (str == "Unknown") return pathfinder::foundation::Result<AgentDiagnosticsStatus>::ok(AgentDiagnosticsStatus::Unknown);
    if (str == "Clean") return pathfinder::foundation::Result<AgentDiagnosticsStatus>::ok(AgentDiagnosticsStatus::Clean);
    if (str == "HasWarnings") return pathfinder::foundation::Result<AgentDiagnosticsStatus>::ok(AgentDiagnosticsStatus::HasWarnings);
    if (str == "HasErrors") return pathfinder::foundation::Result<AgentDiagnosticsStatus>::ok(AgentDiagnosticsStatus::HasErrors);
    return pathfinder::foundation::Result<AgentDiagnosticsStatus>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "agent_diagnostics_status_unknown",
            "Unknown AgentDiagnosticsStatus: " + str));
}

// --- AgentDebugReportItem::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugReportItem::validateBasic() const {
    if (record_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "debug_report_item_missing_record_id"));
    }
    if (agent_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "debug_report_item_missing_agent_id"));
    }
    if (actor_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "debug_report_item_missing_actor_id"));
    }
    if (runtime_status.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "debug_report_item_missing_runtime_status"));
    }
    if (decision_status.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "debug_report_item_missing_decision_status"));
    }
    if (severity == AgentDebugReportSeverity::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "debug_report_item_severity_unknown"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugReport::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugReport::validateBasic() const {
    if (report_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "debug_report_missing_report_id"));
    }
    if (mode == AgentDebugReportMode::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "debug_report_mode_unknown"));
    }
    if (items.size() > total_items) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "debug_report_items_exceed_total"));
    }
    if (replay_locked_count > total_items) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "debug_report_replay_locked_count_exceeds_total"));
    }
    if (explained_only_count > total_items) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "debug_report_explained_only_count_exceeds_total"));
    }
    if (broken_count > total_items) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "debug_report_broken_count_exceeds_total"));
    }
    if (no_decision_count > total_items) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "debug_report_no_decision_count_exceeds_total"));
    }
    if (skipped_count > total_items) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "debug_report_skipped_count_exceeds_total"));
    }
    for (const auto& item : items) {
        auto vr = item.validateBasic();
        if (vr.is_error()) return vr;
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugReportBuildRequest::validateBasic ---

pathfinder::foundation::Result<void> AgentDebugReportBuildRequest::validateBasic() const {
    auto vr = projection.validateBasic();
    if (vr.is_error()) return vr;
    if (mode == AgentDebugReportMode::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "debug_report_build_request_mode_unknown"));
    }
    if (mode == AgentDebugReportMode::Public && include_trace_keys) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "debug_report_public_mode_trace_keys_not_allowed"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDebugReportBuilder::build ---

pathfinder::foundation::Result<AgentDebugReport> AgentDebugReportBuilder::build(
    const AgentDebugReportBuildRequest& request) const {
    auto vr = request.validateBasic();
    if (vr.is_error()) return pathfinder::foundation::Result<AgentDebugReport>::fail(vr.errors());

    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId(
        request.projection.query_id,
        request.projection.items.size());
    report.mode = request.mode;
    report.total_items = request.projection.total_matched;
    report.truncated = request.projection.truncated;

    for (const auto& proj_item : request.projection.items) {
        AgentDebugReportItem item;
        item.record_id = proj_item.record_id;
        item.agent_id = proj_item.agent_id;
        item.actor_id = proj_item.actor_id;
        item.tick = proj_item.tick;
        item.runtime_status = proj_item.runtime_status;
        item.decision_status = proj_item.decision_status;
        item.command_id = proj_item.command_id;
        item.replay_lock_status = proj_item.replay_lock_status;
        item.training_sample_status = proj_item.training_sample_status;

        // Determine severity
        if (proj_item.replay_lock_status && *proj_item.replay_lock_status == "Broken") {
            item.severity = AgentDebugReportSeverity::Error;
            report.broken_count++;
        } else if (proj_item.decision_status == "NoDecision") {
            item.severity = AgentDebugReportSeverity::Info;
            report.no_decision_count++;
        } else if (proj_item.replay_lock_status && *proj_item.replay_lock_status == "Locked") {
            item.severity = AgentDebugReportSeverity::Info;
            report.replay_locked_count++;
        } else if (proj_item.replay_lock_status && *proj_item.replay_lock_status == "ExplainedOnly") {
            item.severity = AgentDebugReportSeverity::Info;
            report.explained_only_count++;
        } else if (proj_item.training_sample_status && *proj_item.training_sample_status == "Skipped") {
            item.severity = AgentDebugReportSeverity::Info;
            report.skipped_count++;
        } else {
            item.severity = AgentDebugReportSeverity::Info;
        }

        // Copy trace keys based on mode
        if (request.include_trace_keys && request.mode != AgentDebugReportMode::Public) {
            item.reason_keys = proj_item.reason_keys;
            item.phase_keys = proj_item.phase_keys;
            item.warning_keys = proj_item.warning_keys;
        }

        // Build summary keys
        if (!proj_item.runtime_status.empty()) {
            item.summary_keys.push_back("runtime:" + proj_item.runtime_status);
        }
        if (!proj_item.decision_status.empty()) {
            item.summary_keys.push_back("decision:" + proj_item.decision_status);
        }
        if (proj_item.replay_lock_status) {
            item.summary_keys.push_back("replay_lock:" + *proj_item.replay_lock_status);
        }
        if (proj_item.training_sample_status) {
            item.summary_keys.push_back("training_sample:" + *proj_item.training_sample_status);
        }

        report.items.push_back(std::move(item));
    }

    auto report_vr = report.validateBasic();
    if (report_vr.is_error()) return pathfinder::foundation::Result<AgentDebugReport>::fail(report_vr.errors());

    return pathfinder::foundation::Result<AgentDebugReport>::ok(std::move(report));
}

// --- AgentDiagnosticsIssue::validateBasic ---

pathfinder::foundation::Result<void> AgentDiagnosticsIssue::validateBasic() const {
    if (severity == AgentDebugReportSeverity::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "diagnostics_issue_severity_unknown"));
    }
    if (issue_key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "diagnostics_issue_missing_issue_key"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDiagnosticsSummary::validateBasic ---

pathfinder::foundation::Result<void> AgentDiagnosticsSummary::validateBasic() const {
    if (status == AgentDiagnosticsStatus::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "diagnostics_summary_status_unknown"));
    }
    for (const auto& issue : issues) {
        auto vr = issue.validateBasic();
        if (vr.is_error()) return vr;
    }
    return pathfinder::foundation::Result<void>::ok();
}

// --- AgentDiagnosticsBuilder::build ---

pathfinder::foundation::Result<AgentDiagnosticsSummary> AgentDiagnosticsBuilder::build(
    const AgentDebugReport& report) const {
    auto vr = report.validateBasic();
    if (vr.is_error()) return pathfinder::foundation::Result<AgentDiagnosticsSummary>::fail(vr.errors());

    AgentDiagnosticsSummary summary;
    summary.item_count = report.items.size();

    for (const auto& item : report.items) {
        // Broken replay lock -> Error issue
        if (item.replay_lock_status && *item.replay_lock_status == "Broken") {
            AgentDiagnosticsIssue issue;
            issue.severity = AgentDebugReportSeverity::Error;
            issue.issue_key = "broken_replay_lock";
            issue.record_id = item.record_id;
            issue.agent_id = item.agent_id;
            issue.detail_keys.push_back("replay_lock:Broken");
            summary.issues.push_back(std::move(issue));
            summary.error_count++;
        }

        // Unknown/Invalid status -> Error issue
        if (item.runtime_status == "Unknown" || item.runtime_status == "Invalid" ||
            item.decision_status == "Unknown" || item.decision_status == "Invalid") {
            AgentDiagnosticsIssue issue;
            issue.severity = AgentDebugReportSeverity::Error;
            issue.issue_key = "invalid_unknown_status";
            issue.record_id = item.record_id;
            issue.agent_id = item.agent_id;
            issue.detail_keys.push_back("runtime:" + item.runtime_status);
            issue.detail_keys.push_back("decision:" + item.decision_status);
            summary.issues.push_back(std::move(issue));
            summary.error_count++;
        }

        // NoDecision without explanation keys (Debug/Training mode) -> Warning
        if (item.decision_status == "NoDecision" &&
            report.mode != AgentDebugReportMode::Public &&
            item.reason_keys.empty() && item.phase_keys.empty()) {
            AgentDiagnosticsIssue issue;
            issue.severity = AgentDebugReportSeverity::Warning;
            issue.issue_key = "no_decision_no_explanation";
            issue.record_id = item.record_id;
            issue.agent_id = item.agent_id;
            summary.issues.push_back(std::move(issue));
            summary.warning_count++;
        }

        // Public mode with trace keys -> Error
        if (report.mode == AgentDebugReportMode::Public &&
            (!item.reason_keys.empty() || !item.phase_keys.empty() || !item.warning_keys.empty())) {
            AgentDiagnosticsIssue issue;
            issue.severity = AgentDebugReportSeverity::Error;
            issue.issue_key = "public_trace_leak";
            issue.record_id = item.record_id;
            issue.agent_id = item.agent_id;
            if (!item.reason_keys.empty()) issue.detail_keys.push_back("reason_keys_present");
            if (!item.phase_keys.empty()) issue.detail_keys.push_back("phase_keys_present");
            if (!item.warning_keys.empty()) issue.detail_keys.push_back("warning_keys_present");
            summary.issues.push_back(std::move(issue));
            summary.error_count++;
        }
    }

    // Determine overall status
    if (summary.error_count > 0) {
        summary.status = AgentDiagnosticsStatus::HasErrors;
    } else if (summary.warning_count > 0) {
        summary.status = AgentDiagnosticsStatus::HasWarnings;
    } else {
        summary.status = AgentDiagnosticsStatus::Clean;
    }

    auto diag_vr = summary.validateBasic();
    if (diag_vr.is_error()) return pathfinder::foundation::Result<AgentDiagnosticsSummary>::fail(diag_vr.errors());

    return pathfinder::foundation::Result<AgentDiagnosticsSummary>::ok(std::move(summary));
}

} // namespace pathfinder::agent
