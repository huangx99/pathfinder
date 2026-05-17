#include "pathfinder/agent/debug_export.h"
#include "pathfinder/agent/debug_report.h"
#include "pathfinder/protocol/agent_projection.h"
#include "pathfinder/agent/history_query.h"
#include "pathfinder/agent/replay_lock.h"
#include "pathfinder/agent/training_sample.h"
#include "pathfinder/agent/record_types.h"
#include "pathfinder/agent/runtime_types.h"
#include "pathfinder/agent/types.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/command/target.h"
#include "pathfinder/pipeline/types.h"
#include "pathfinder/replay/command_replay.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::protocol;
using namespace pathfinder::command;
using namespace pathfinder::pipeline;
using namespace pathfinder::replay;

// === Test: unknown fruit debug report export flow ===

void p11_agent_debug_report_export_flow() {
    // 1. Construct P10 AgentHistoryProjection with a ReplayLocked unknown fruit item
    AgentHistoryItemProjection proj;
    proj.record_id = "rec_001";
    proj.agent_id = "agent_unknown_fruit";
    proj.actor_id = "actor_001";
    proj.tick = 10;
    proj.state_version_before = 1;
    proj.state_version_after = 2;
    proj.runtime_status = "PipelineSucceeded";
    proj.decision_status = "PipelineSucceeded";
    proj.selected_candidate_id = "eat";
    proj.selected_command_action_id = "eat";
    proj.command_id = "cmd_001";
    proj.replay_record_id = "agent_replay_cmd_001";
    proj.replay_lock_status = "Locked";
    proj.training_sample_status = "ReplayLocked";
    proj.state_change_count = 1;
    proj.event_count = 2;
    proj.error_count = 0;
    proj.phase_keys.push_back("observe");
    proj.phase_keys.push_back("decide");
    proj.reason_keys.push_back("candidate_selected");
    proj.warning_keys.push_back("unknown_object");

    AgentHistoryProjection history;
    history.query_id = "q_unknown_fruit";
    history.projection_mode = AgentHistoryProjectionMode::Training;
    history.total_matched = 1;
    history.truncated = false;
    history.items.push_back(proj);

    // 2. AgentDebugReportBuilder generates Training mode report
    AgentDebugReportBuilder report_builder;
    AgentDebugReportBuildRequest report_req;
    report_req.projection = history;
    report_req.mode = AgentDebugReportMode::Training;
    report_req.include_trace_keys = true;

    auto report_result = report_builder.build(report_req);
    assert(report_result.is_ok());
    const auto& report = report_result.value();

    // 3. Assert replay_locked_count = 1
    assert(report.replay_locked_count == 1);
    assert(report.items.size() == 1);
    assert(report.items[0].command_id.has_value());
    assert(*report.items[0].command_id == "cmd_001");

    // 4. AgentDiagnosticsBuilder generates diagnostics
    AgentDiagnosticsBuilder diag_builder;
    auto diag_result = diag_builder.build(report);
    assert(diag_result.is_ok());
    const auto& diag = diag_result.value();

    // 5. Assert diagnostics.status = Clean
    assert(diag.status == AgentDiagnosticsStatus::Clean);

    // 6. AgentExportDraftBuilder generates MarkdownLike draft
    AgentExportDraftBuilder export_builder;
    AgentExportDraftBuildRequest export_req;
    export_req.report = report;
    export_req.diagnostics = diag;
    export_req.format = AgentExportFormat::MarkdownLike;

    auto export_result = export_builder.build(export_req);
    assert(export_result.is_ok());
    const auto& draft = export_result.value();

    // 7. Assert manifest/chunks validateBasic pass
    assert(draft.validateBasic().is_ok());
    assert(draft.manifest.chunk_count == draft.chunks.size());
    assert(!draft.chunks[0].payload.empty());

    // 8. Assert no file written, no path access (verified by boundary scan)
}

// === Test Main ===

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P11 integration test binary loaded." << std::endl;
        return 0;
    }

    if (test_name == "p11_agent_debug_report_export_flow") { p11_agent_debug_report_export_flow(); return 0; }

    std::cerr << "Unknown test: " << test_name << std::endl;
    return 1;
}
