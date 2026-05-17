#include "pathfinder/agent/debug_export.h"
#include "pathfinder/agent/debug_report.h"
#include "pathfinder/protocol/agent_projection.h"
#include "pathfinder/agent/history_query.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::protocol;

// === Test: NoDecision debug report ===

void p11_agent_no_decision_debug_report() {
    // 1. Construct P10 Debug mode AgentHistoryProjection with NoDecision item
    AgentHistoryItemProjection no_dec_proj;
    no_dec_proj.record_id = "rec_no_dec_001";
    no_dec_proj.agent_id = "agent_001";
    no_dec_proj.actor_id = "actor_001";
    no_dec_proj.tick = 15;
    no_dec_proj.state_version_before = 5;
    no_dec_proj.state_version_after = 6;
    no_dec_proj.runtime_status = "NoDecision";
    no_dec_proj.decision_status = "NoDecision";
    // No command_id for NoDecision
    no_dec_proj.replay_lock_status = "ExplainedOnly";
    no_dec_proj.phase_keys.push_back("observe");
    no_dec_proj.reason_keys.push_back("no_candidate_available");

    // Also add a PipelineSucceeded item
    AgentHistoryItemProjection proj;
    proj.record_id = "rec_pipe_001";
    proj.agent_id = "agent_001";
    proj.actor_id = "actor_001";
    proj.tick = 14;
    proj.state_version_before = 4;
    proj.state_version_after = 5;
    proj.runtime_status = "PipelineSucceeded";
    proj.decision_status = "PipelineSucceeded";
    proj.command_id = "cmd_001";
    proj.replay_lock_status = "Locked";

    AgentHistoryProjection history;
    history.query_id = "q_no_decision_debug";
    history.projection_mode = AgentHistoryProjectionMode::Debug;
    history.total_matched = 2;
    history.items.push_back(proj);
    history.items.push_back(no_dec_proj);

    // 2. AgentDebugReportBuilder generates Debug report
    AgentDebugReportBuilder builder;
    AgentDebugReportBuildRequest req;
    req.projection = history;
    req.mode = AgentDebugReportMode::Debug;
    req.include_trace_keys = true;

    auto result = builder.build(req);
    assert(result.is_ok());
    const auto& report = result.value();

    // 3. Assert no_decision_count = 1
    assert(report.no_decision_count == 1);

    // 4. Assert NoDecision item has no command_id
    const AgentDebugReportItem* no_dec_item = nullptr;
    for (const auto& item : report.items) {
        if (item.decision_status == "NoDecision") {
            no_dec_item = &item;
            break;
        }
    }
    assert(no_dec_item != nullptr);
    assert(!no_dec_item->command_id.has_value());

    // 5. Assert reason_keys or phase_keys exist
    assert(!no_dec_item->reason_keys.empty() || !no_dec_item->phase_keys.empty());

    // 6. AgentDiagnosticsBuilder does NOT produce Error for NoDecision
    AgentDiagnosticsBuilder diag_builder;
    auto diag_result = diag_builder.build(report);
    assert(diag_result.is_ok());
    assert(diag_result.value().status != AgentDiagnosticsStatus::HasErrors);

    // 7. Public mode: re-generate report, assert trace keys cleared
    AgentDebugReportBuildRequest public_req;
    public_req.projection = history;
    public_req.mode = AgentDebugReportMode::Public;
    public_req.include_trace_keys = false;

    auto public_result = builder.build(public_req);
    assert(public_result.is_ok());
    for (const auto& item : public_result.value().items) {
        assert(item.reason_keys.empty());
        assert(item.phase_keys.empty());
        assert(item.warning_keys.empty());
    }
}

// === Test Main ===

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P11 NoDecision integration test binary loaded." << std::endl;
        return 0;
    }

    if (test_name == "p11_agent_no_decision_debug_report") { p11_agent_no_decision_debug_report(); return 0; }

    std::cerr << "Unknown test: " << test_name << std::endl;
    return 1;
}
