#include "pathfinder/agent/debug_report.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::protocol;

// --- Helper: make a valid AgentDebugReportItem ---
AgentDebugReportItem makeValidReportItem() {
    AgentDebugReportItem item;
    item.record_id = "rec_001";
    item.agent_id = "agent_001";
    item.actor_id = "actor_001";
    item.tick = 10;
    item.runtime_status = "PipelineSucceeded";
    item.decision_status = "PipelineSucceeded";
    item.command_id = "cmd_001";
    item.severity = AgentDebugReportSeverity::Info;
    return item;
}

// --- Helper: make a valid AgentHistoryItemProjection ---
AgentHistoryItemProjection makeValidHistoryItem() {
    AgentHistoryItemProjection proj;
    proj.record_id = "rec_001";
    proj.agent_id = "agent_001";
    proj.actor_id = "actor_001";
    proj.tick = 10;
    proj.state_version_before = 1;
    proj.state_version_after = 2;
    proj.runtime_status = "PipelineSucceeded";
    proj.decision_status = "PipelineSucceeded";
    proj.command_id = "cmd_001";
    proj.replay_lock_status = "Locked";
    proj.training_sample_status = "ReplayLocked";
    return proj;
}

// --- Helper: make a valid AgentHistoryProjection ---
AgentHistoryProjection makeValidHistoryProjection() {
    AgentHistoryProjection proj;
    proj.query_id = "q_001";
    proj.projection_mode = AgentHistoryProjectionMode::Debug;
    proj.total_matched = 1;
    proj.truncated = false;
    proj.items.push_back(makeValidHistoryItem());
    return proj;
}

// === Enum Roundtrip Tests ===

void mode_roundtrip() {
    assert(toString(AgentDebugReportMode::Unknown) == "Unknown");
    assert(toString(AgentDebugReportMode::Public) == "Public");
    assert(toString(AgentDebugReportMode::Debug) == "Debug");
    assert(toString(AgentDebugReportMode::Training) == "Training");

    auto r1 = agentDebugReportModeFromString("Public");
    assert(r1.is_ok() && r1.value() == AgentDebugReportMode::Public);
    auto r2 = agentDebugReportModeFromString("Debug");
    assert(r2.is_ok() && r2.value() == AgentDebugReportMode::Debug);
    auto r3 = agentDebugReportModeFromString("Training");
    assert(r3.is_ok() && r3.value() == AgentDebugReportMode::Training);
    auto r4 = agentDebugReportModeFromString("Unknown");
    assert(r4.is_ok() && r4.value() == AgentDebugReportMode::Unknown);
    auto r5 = agentDebugReportModeFromString("Invalid");
    assert(r5.is_error());
}

void severity_roundtrip() {
    assert(toString(AgentDebugReportSeverity::Unknown) == "Unknown");
    assert(toString(AgentDebugReportSeverity::Info) == "Info");
    assert(toString(AgentDebugReportSeverity::Warning) == "Warning");
    assert(toString(AgentDebugReportSeverity::Error) == "Error");

    auto r1 = agentDebugReportSeverityFromString("Info");
    assert(r1.is_ok() && r1.value() == AgentDebugReportSeverity::Info);
    auto r2 = agentDebugReportSeverityFromString("Warning");
    assert(r2.is_ok() && r2.value() == AgentDebugReportSeverity::Warning);
    auto r3 = agentDebugReportSeverityFromString("Error");
    assert(r3.is_ok() && r3.value() == AgentDebugReportSeverity::Error);
    auto r4 = agentDebugReportSeverityFromString("Unknown");
    assert(r4.is_ok() && r4.value() == AgentDebugReportSeverity::Unknown);
    auto r5 = agentDebugReportSeverityFromString("Bad");
    assert(r5.is_error());
}

void diagnostics_status_roundtrip() {
    assert(toString(AgentDiagnosticsStatus::Unknown) == "Unknown");
    assert(toString(AgentDiagnosticsStatus::Clean) == "Clean");
    assert(toString(AgentDiagnosticsStatus::HasWarnings) == "HasWarnings");
    assert(toString(AgentDiagnosticsStatus::HasErrors) == "HasErrors");

    auto r1 = agentDiagnosticsStatusFromString("Clean");
    assert(r1.is_ok() && r1.value() == AgentDiagnosticsStatus::Clean);
    auto r2 = agentDiagnosticsStatusFromString("HasWarnings");
    assert(r2.is_ok() && r2.value() == AgentDiagnosticsStatus::HasWarnings);
    auto r3 = agentDiagnosticsStatusFromString("HasErrors");
    assert(r3.is_ok() && r3.value() == AgentDiagnosticsStatus::HasErrors);
    auto r4 = agentDiagnosticsStatusFromString("Unknown");
    assert(r4.is_ok() && r4.value() == AgentDiagnosticsStatus::Unknown);
    auto r5 = agentDiagnosticsStatusFromString("Bad");
    assert(r5.is_error());
}

// === ID Helper Tests ===

void id_helper() {
    auto id = makeAgentDebugReportId("q_001", 5);
    assert(!id.empty());
    assert(id.value() == "agent_debug_report_q_001_5");
}

// === Item Validation Tests ===

void valid_item_passes() {
    auto item = makeValidReportItem();
    assert(item.validateBasic().is_ok());
}

void missing_record_id_fails() {
    auto item = makeValidReportItem();
    item.record_id = "";
    assert(item.validateBasic().is_error());
}

void missing_agent_id_fails() {
    auto item = makeValidReportItem();
    item.agent_id = "";
    assert(item.validateBasic().is_error());
}

void missing_actor_id_fails() {
    auto item = makeValidReportItem();
    item.actor_id = "";
    assert(item.validateBasic().is_error());
}

void missing_runtime_status_fails() {
    auto item = makeValidReportItem();
    item.runtime_status = "";
    assert(item.validateBasic().is_error());
}

void missing_decision_status_fails() {
    auto item = makeValidReportItem();
    item.decision_status = "";
    assert(item.validateBasic().is_error());
}

void severity_unknown_fails() {
    auto item = makeValidReportItem();
    item.severity = AgentDebugReportSeverity::Unknown;
    assert(item.validateBasic().is_error());
}

// === Report Validation Tests ===

void valid_report_passes() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_001", 1);
    report.mode = AgentDebugReportMode::Public;
    report.total_items = 1;
    report.items.push_back(makeValidReportItem());
    assert(report.validateBasic().is_ok());
}

void empty_report_id_fails() {
    AgentDebugReport report;
    report.mode = AgentDebugReportMode::Public;
    report.total_items = 0;
    assert(report.validateBasic().is_error());
}

void mode_unknown_fails() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_001", 0);
    report.mode = AgentDebugReportMode::Unknown;
    report.total_items = 0;
    assert(report.validateBasic().is_error());
}

void items_exceed_total_fails() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_001", 1);
    report.mode = AgentDebugReportMode::Public;
    report.total_items = 0;
    report.items.push_back(makeValidReportItem());
    assert(report.validateBasic().is_error());
}

void count_exceeds_total_fails() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_001", 0);
    report.mode = AgentDebugReportMode::Public;
    report.total_items = 0;
    report.replay_locked_count = 1;
    assert(report.validateBasic().is_error());
}

void invalid_item_in_report_fails() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_001", 1);
    report.mode = AgentDebugReportMode::Public;
    report.total_items = 1;
    auto bad_item = makeValidReportItem();
    bad_item.record_id = "";
    report.items.push_back(bad_item);
    assert(report.validateBasic().is_error());
}

// === Diagnostics Validation Tests ===

void valid_issue_passes() {
    AgentDiagnosticsIssue issue;
    issue.severity = AgentDebugReportSeverity::Warning;
    issue.issue_key = "test_issue";
    assert(issue.validateBasic().is_ok());
}

void issue_unknown_severity_fails() {
    AgentDiagnosticsIssue issue;
    issue.severity = AgentDebugReportSeverity::Unknown;
    issue.issue_key = "test_issue";
    assert(issue.validateBasic().is_error());
}

void issue_empty_key_fails() {
    AgentDiagnosticsIssue issue;
    issue.severity = AgentDebugReportSeverity::Warning;
    issue.issue_key = "";
    assert(issue.validateBasic().is_error());
}

void valid_diagnostics_passes() {
    AgentDiagnosticsSummary summary;
    summary.status = AgentDiagnosticsStatus::Clean;
    summary.item_count = 1;
    assert(summary.validateBasic().is_ok());
}

void diagnostics_unknown_status_fails() {
    AgentDiagnosticsSummary summary;
    summary.status = AgentDiagnosticsStatus::Unknown;
    assert(summary.validateBasic().is_error());
}

// === Builder Tests ===

void builder_pipeline_succeeded_has_command_id() {
    AgentDebugReportBuilder builder;
    AgentDebugReportBuildRequest req;
    req.projection = makeValidHistoryProjection();
    req.mode = AgentDebugReportMode::Debug;
    req.include_trace_keys = true;

    auto result = builder.build(req);
    assert(result.is_ok());
    const auto& report = result.value();
    assert(report.items.size() == 1);
    assert(report.items[0].command_id.has_value());
    assert(*report.items[0].command_id == "cmd_001");
    assert(report.replay_locked_count == 1);
}

void builder_no_decision_no_command_id() {
    AgentHistoryItemProjection proj;
    proj.record_id = "rec_002";
    proj.agent_id = "agent_001";
    proj.actor_id = "actor_001";
    proj.tick = 11;
    proj.state_version_before = 2;
    proj.state_version_after = 3;
    proj.runtime_status = "NoDecision";
    proj.decision_status = "NoDecision";
    proj.reason_keys.push_back("no_candidate_available");

    AgentHistoryProjection history;
    history.query_id = "q_002";
    history.projection_mode = AgentHistoryProjectionMode::Debug;
    history.total_matched = 1;
    history.items.push_back(proj);

    AgentDebugReportBuilder builder;
    AgentDebugReportBuildRequest req;
    req.projection = history;
    req.mode = AgentDebugReportMode::Debug;
    req.include_trace_keys = true;

    auto result = builder.build(req);
    assert(result.is_ok());
    const auto& report = result.value();
    assert(report.items.size() == 1);
    assert(!report.items[0].command_id.has_value());
    assert(report.no_decision_count == 1);
    assert(!report.items[0].reason_keys.empty());
}

void builder_broken_item_severity_error() {
    AgentHistoryItemProjection proj;
    proj.record_id = "rec_003";
    proj.agent_id = "agent_001";
    proj.actor_id = "actor_001";
    proj.tick = 12;
    proj.state_version_before = 3;
    proj.state_version_after = 4;
    proj.runtime_status = "PipelineSucceeded";
    proj.decision_status = "PipelineSucceeded";
    proj.command_id = "cmd_003";
    proj.replay_lock_status = "Broken";

    AgentHistoryProjection history;
    history.query_id = "q_003";
    history.projection_mode = AgentHistoryProjectionMode::Debug;
    history.total_matched = 1;
    history.items.push_back(proj);

    AgentDebugReportBuilder builder;
    AgentDebugReportBuildRequest req;
    req.projection = history;
    req.mode = AgentDebugReportMode::Debug;

    auto result = builder.build(req);
    assert(result.is_ok());
    const auto& report = result.value();
    assert(report.items[0].severity == AgentDebugReportSeverity::Error);
    assert(report.broken_count == 1);
}

void builder_public_clears_trace_keys() {
    AgentHistoryItemProjection proj;
    proj.record_id = "rec_004";
    proj.agent_id = "agent_001";
    proj.actor_id = "actor_001";
    proj.tick = 13;
    proj.state_version_before = 4;
    proj.state_version_after = 5;
    proj.runtime_status = "PipelineSucceeded";
    proj.decision_status = "PipelineSucceeded";
    proj.command_id = "cmd_004";
    proj.reason_keys.push_back("some_reason");
    proj.phase_keys.push_back("some_phase");
    proj.warning_keys.push_back("some_warning");

    AgentHistoryProjection history;
    history.query_id = "q_004";
    history.projection_mode = AgentHistoryProjectionMode::Public;
    history.total_matched = 1;
    history.items.push_back(proj);

    AgentDebugReportBuilder builder;
    AgentDebugReportBuildRequest req;
    req.projection = history;
    req.mode = AgentDebugReportMode::Public;
    req.include_trace_keys = false;

    auto result = builder.build(req);
    assert(result.is_ok());
    const auto& report = result.value();
    assert(report.items[0].reason_keys.empty());
    assert(report.items[0].phase_keys.empty());
    assert(report.items[0].warning_keys.empty());
}

void builder_debug_keeps_trace_keys() {
    AgentHistoryItemProjection proj;
    proj.record_id = "rec_005";
    proj.agent_id = "agent_001";
    proj.actor_id = "actor_001";
    proj.tick = 14;
    proj.state_version_before = 5;
    proj.state_version_after = 6;
    proj.runtime_status = "NoDecision";
    proj.decision_status = "NoDecision";
    proj.reason_keys.push_back("no_candidate");
    proj.phase_keys.push_back("observation");

    AgentHistoryProjection history;
    history.query_id = "q_005";
    history.projection_mode = AgentHistoryProjectionMode::Debug;
    history.total_matched = 1;
    history.items.push_back(proj);

    AgentDebugReportBuilder builder;
    AgentDebugReportBuildRequest req;
    req.projection = history;
    req.mode = AgentDebugReportMode::Debug;
    req.include_trace_keys = true;

    auto result = builder.build(req);
    assert(result.is_ok());
    const auto& report = result.value();
    assert(!report.items[0].reason_keys.empty());
    assert(!report.items[0].phase_keys.empty());
}

void builder_explained_only_count() {
    AgentHistoryItemProjection proj;
    proj.record_id = "rec_006";
    proj.agent_id = "agent_001";
    proj.actor_id = "actor_001";
    proj.tick = 15;
    proj.state_version_before = 6;
    proj.state_version_after = 7;
    proj.runtime_status = "SubmitSkipped";
    proj.decision_status = "SubmitSkipped";
    proj.replay_lock_status = "ExplainedOnly";

    AgentHistoryProjection history;
    history.query_id = "q_006";
    history.projection_mode = AgentHistoryProjectionMode::Debug;
    history.total_matched = 1;
    history.items.push_back(proj);

    AgentDebugReportBuilder builder;
    AgentDebugReportBuildRequest req;
    req.projection = history;
    req.mode = AgentDebugReportMode::Debug;

    auto result = builder.build(req);
    assert(result.is_ok());
    assert(result.value().explained_only_count == 1);
}

void builder_public_trace_keys_not_allowed() {
    AgentDebugReportBuildRequest req;
    req.projection = makeValidHistoryProjection();
    req.mode = AgentDebugReportMode::Public;
    req.include_trace_keys = true;
    assert(req.validateBasic().is_error());
}

// === Diagnostics Builder Tests ===

void diagnostics_clean_report() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_001", 1);
    report.mode = AgentDebugReportMode::Debug;
    report.total_items = 1;
    auto item = makeValidReportItem();
    report.items.push_back(item);

    AgentDiagnosticsBuilder builder;
    auto result = builder.build(report);
    assert(result.is_ok());
    assert(result.value().status == AgentDiagnosticsStatus::Clean);
    assert(result.value().warning_count == 0);
    assert(result.value().error_count == 0);
}

void diagnostics_broken_item_error() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_002", 1);
    report.mode = AgentDebugReportMode::Debug;
    report.total_items = 1;
    auto item = makeValidReportItem();
    item.replay_lock_status = "Broken";
    item.severity = AgentDebugReportSeverity::Error;
    report.items.push_back(item);

    AgentDiagnosticsBuilder builder;
    auto result = builder.build(report);
    assert(result.is_ok());
    assert(result.value().status == AgentDiagnosticsStatus::HasErrors);
    assert(result.value().error_count >= 1);
}

void diagnostics_no_decision_no_explanation_warning() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_003", 1);
    report.mode = AgentDebugReportMode::Debug;
    report.total_items = 1;
    auto item = makeValidReportItem();
    item.decision_status = "NoDecision";
    item.severity = AgentDebugReportSeverity::Info;
    item.command_id = std::nullopt;
    // No reason_keys or phase_keys
    report.items.push_back(item);

    AgentDiagnosticsBuilder builder;
    auto result = builder.build(report);
    assert(result.is_ok());
    assert(result.value().status == AgentDiagnosticsStatus::HasWarnings);
    assert(result.value().warning_count >= 1);
}

void diagnostics_public_trace_leak_error() {
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_004", 1);
    report.mode = AgentDebugReportMode::Public;
    report.total_items = 1;
    auto item = makeValidReportItem();
    item.reason_keys.push_back("leaked_reason");
    report.items.push_back(item);

    AgentDiagnosticsBuilder builder;
    auto result = builder.build(report);
    assert(result.is_ok());
    assert(result.value().status == AgentDiagnosticsStatus::HasErrors);
    bool found = false;
    for (const auto& issue : result.value().issues) {
        if (issue.issue_key == "public_trace_leak") found = true;
    }
    assert(found);
}

void diagnostics_no_decision_public_no_warning() {
    // Public mode NoDecision without explanation should NOT warn (keys already cleared)
    AgentDebugReport report;
    report.report_id = makeAgentDebugReportId("q_005", 1);
    report.mode = AgentDebugReportMode::Public;
    report.total_items = 1;
    auto item = makeValidReportItem();
    item.decision_status = "NoDecision";
    item.severity = AgentDebugReportSeverity::Info;
    item.command_id = std::nullopt;
    // Public mode: reason_keys/phase_keys already empty (cleared by builder)
    report.items.push_back(item);

    AgentDiagnosticsBuilder builder;
    auto result = builder.build(report);
    assert(result.is_ok());
    // Public mode should NOT generate the no_explanation warning
    assert(result.value().status == AgentDiagnosticsStatus::Clean);
}

// === Test Main ===

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P11 DebugReport test binary loaded." << std::endl;
        return 0;
    }

    // Enum roundtrip
    if (test_name == "mode_roundtrip") { mode_roundtrip(); return 0; }
    if (test_name == "severity_roundtrip") { severity_roundtrip(); return 0; }
    if (test_name == "diagnostics_status_roundtrip") { diagnostics_status_roundtrip(); return 0; }

    // ID helper
    if (test_name == "id_helper") { id_helper(); return 0; }

    // Item validation
    if (test_name == "valid_item_passes") { valid_item_passes(); return 0; }
    if (test_name == "missing_record_id_fails") { missing_record_id_fails(); return 0; }
    if (test_name == "missing_agent_id_fails") { missing_agent_id_fails(); return 0; }
    if (test_name == "missing_actor_id_fails") { missing_actor_id_fails(); return 0; }
    if (test_name == "missing_runtime_status_fails") { missing_runtime_status_fails(); return 0; }
    if (test_name == "missing_decision_status_fails") { missing_decision_status_fails(); return 0; }
    if (test_name == "severity_unknown_fails") { severity_unknown_fails(); return 0; }

    // Report validation
    if (test_name == "valid_report_passes") { valid_report_passes(); return 0; }
    if (test_name == "empty_report_id_fails") { empty_report_id_fails(); return 0; }
    if (test_name == "mode_unknown_fails") { mode_unknown_fails(); return 0; }
    if (test_name == "items_exceed_total_fails") { items_exceed_total_fails(); return 0; }
    if (test_name == "count_exceeds_total_fails") { count_exceeds_total_fails(); return 0; }
    if (test_name == "invalid_item_in_report_fails") { invalid_item_in_report_fails(); return 0; }

    // Diagnostics validation
    if (test_name == "valid_issue_passes") { valid_issue_passes(); return 0; }
    if (test_name == "issue_unknown_severity_fails") { issue_unknown_severity_fails(); return 0; }
    if (test_name == "issue_empty_key_fails") { issue_empty_key_fails(); return 0; }
    if (test_name == "valid_diagnostics_passes") { valid_diagnostics_passes(); return 0; }
    if (test_name == "diagnostics_unknown_status_fails") { diagnostics_unknown_status_fails(); return 0; }

    // Builder tests
    if (test_name == "builder_pipeline_succeeded_has_command_id") { builder_pipeline_succeeded_has_command_id(); return 0; }
    if (test_name == "builder_no_decision_no_command_id") { builder_no_decision_no_command_id(); return 0; }
    if (test_name == "builder_broken_item_severity_error") { builder_broken_item_severity_error(); return 0; }
    if (test_name == "builder_public_clears_trace_keys") { builder_public_clears_trace_keys(); return 0; }
    if (test_name == "builder_debug_keeps_trace_keys") { builder_debug_keeps_trace_keys(); return 0; }
    if (test_name == "builder_explained_only_count") { builder_explained_only_count(); return 0; }
    if (test_name == "builder_public_trace_keys_not_allowed") { builder_public_trace_keys_not_allowed(); return 0; }

    // Diagnostics builder tests
    if (test_name == "diagnostics_clean_report") { diagnostics_clean_report(); return 0; }
    if (test_name == "diagnostics_broken_item_error") { diagnostics_broken_item_error(); return 0; }
    if (test_name == "diagnostics_no_decision_no_explanation_warning") { diagnostics_no_decision_no_explanation_warning(); return 0; }
    if (test_name == "diagnostics_public_trace_leak_error") { diagnostics_public_trace_leak_error(); return 0; }
    if (test_name == "diagnostics_no_decision_public_no_warning") { diagnostics_no_decision_public_no_warning(); return 0; }

    std::cerr << "Unknown test: " << test_name << std::endl;
    return 1;
}
