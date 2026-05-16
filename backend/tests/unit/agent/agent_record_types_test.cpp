#include "pathfinder/agent/record_types.h"
#include "pathfinder/agent/record_builder.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::command;
using namespace pathfinder::pipeline;
using namespace pathfinder::replay;

// --- Enum roundtrip tests ---

void test_decision_record_status_roundtrip() {
    auto statuses = {
        AgentDecisionRecordStatus::Unknown,
        AgentDecisionRecordStatus::Selected,
        AgentDecisionRecordStatus::NoDecision,
        AgentDecisionRecordStatus::AdapterRejected,
        AgentDecisionRecordStatus::SubmitSkipped,
        AgentDecisionRecordStatus::PipelineSucceeded,
        AgentDecisionRecordStatus::PipelineFailed,
        AgentDecisionRecordStatus::Failed
    };
    for (auto s : statuses) {
        auto str = toString(s);
        auto back = agentDecisionRecordStatusFromString(str);
        assert(back.is_ok());
        if (s != AgentDecisionRecordStatus::Unknown) {
            assert(back.value() == s);
        }
    }
    std::cout << "  PASS: decision_record_status_roundtrip\n";
}

void test_tick_record_status_roundtrip() {
    auto statuses = {
        AgentTickRecordStatus::Unknown,
        AgentTickRecordStatus::Recorded,
        AgentTickRecordStatus::ReplayLinked,
        AgentTickRecordStatus::ReplayMatched,
        AgentTickRecordStatus::ReplayMismatched,
        AgentTickRecordStatus::Invalid
    };
    for (auto s : statuses) {
        auto str = toString(s);
        auto back = agentTickRecordStatusFromString(str);
        assert(back.is_ok());
        if (s != AgentTickRecordStatus::Unknown) {
            assert(back.value() == s);
        }
    }
    std::cout << "  PASS: tick_record_status_roundtrip\n";
}

// --- ID helper tests ---

void test_id_helpers() {
    auto tick_id = makeAgentTickRecordId(
        AgentId("agent_001"), Tick(5), StateVersion(3));
    assert(!tick_id.empty());
    assert(tick_id.value() == "agent_tick_agent_001_5_3");

    auto dec_id = makeAgentDecisionRecordId(DecisionId("dec_001"));
    assert(!dec_id.empty());
    assert(dec_id.value() == "agent_decision_record_dec_001");

    std::cout << "  PASS: id_helpers\n";
}

// --- AgentDecisionRecord validation ---

AgentDecisionRecord makeValidDecisionRecord() {
    AgentDecisionRecord rec;
    rec.record_id = AgentDecisionRecordId("agent_decision_record_dec_001");
    rec.decision_id = DecisionId("dec_001");
    rec.agent_id = AgentId("agent_001");
    rec.actor_id = EntityId("actor_001");
    rec.decision_tick = Tick(0);
    rec.observation_state_version = StateVersion(1);
    rec.status = AgentDecisionRecordStatus::Selected;
    rec.confidence = 0.8;
    rec.command_id = CommandId("cmd_001");
    return rec;
}

void test_valid_decision_record_passes() {
    auto rec = makeValidDecisionRecord();
    assert(rec.validateBasic().is_ok());
    std::cout << "  PASS: valid_decision_record_passes\n";
}

void test_missing_record_id_fails() {
    auto rec = makeValidDecisionRecord();
    rec.record_id = AgentDecisionRecordId();
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: missing_record_id_fails\n";
}

void test_unknown_status_fails() {
    auto rec = makeValidDecisionRecord();
    rec.status = AgentDecisionRecordStatus::Unknown;
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: unknown_status_fails\n";
}

void test_confidence_out_of_range_fails() {
    auto rec = makeValidDecisionRecord();
    rec.confidence = 1.5;
    assert(rec.validateBasic().is_error());
    rec.confidence = -0.1;
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: confidence_out_of_range_fails\n";
}

void test_pipeline_succeeded_requires_command_id() {
    auto rec = makeValidDecisionRecord();
    rec.status = AgentDecisionRecordStatus::PipelineSucceeded;
    rec.command_id = std::nullopt;
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: pipeline_succeeded_requires_command_id\n";
}

void test_no_decision_allows_empty_decision_id() {
    auto rec = makeValidDecisionRecord();
    rec.status = AgentDecisionRecordStatus::NoDecision;
    rec.decision_id = DecisionId();
    rec.command_id = std::nullopt;
    rec.reason_key = "no_supported_candidate";
    assert(rec.validateBasic().is_ok());
    std::cout << "  PASS: no_decision_allows_empty_decision_id\n";
}

void test_no_decision_no_explanation_fails() {
    auto rec = makeValidDecisionRecord();
    rec.status = AgentDecisionRecordStatus::NoDecision;
    rec.decision_id = DecisionId();
    rec.command_id = std::nullopt;
    rec.reason_key = "";
    rec.phase_keys.clear();
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: no_decision_no_explanation_fails\n";
}

void test_no_decision_with_phase_keys_passes() {
    auto rec = makeValidDecisionRecord();
    rec.status = AgentDecisionRecordStatus::NoDecision;
    rec.decision_id = DecisionId();
    rec.command_id = std::nullopt;
    rec.reason_key = "";
    rec.phase_keys = {"observation_built", "action_space_built", "no_decision"};
    assert(rec.validateBasic().is_ok());
    std::cout << "  PASS: no_decision_with_phase_keys_passes\n";
}

// --- AgentTickRecord validation ---

AgentTickRecord makeValidTickRecord() {
    AgentTickRecord rec;
    rec.record_id = makeAgentTickRecordId(AgentId("agent_001"), Tick(0), StateVersion(1));
    rec.agent_id = AgentId("agent_001");
    rec.actor_id = EntityId("actor_001");
    rec.tick = Tick(0);
    rec.state_version_before = StateVersion(1);
    rec.state_version_after = StateVersion(2);
    rec.random_seed = RandomSeed(42);
    rec.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    rec.record_status = AgentTickRecordStatus::Recorded;
    rec.input_hash = HashValue(111);
    rec.output_hash = HashValue(222);
    rec.pipeline_status = PipelineStatus::Succeeded;

    // Valid command
    CommandEnvelope env;
    env.command_id = CommandId("cmd_001");
    env.payload.action_id = ActionId("eat");
    env.source = CommandSource::Ai;
    rec.command = env;

    // Decision record
    rec.decision_record = makeValidDecisionRecord();
    rec.decision_record.command_id = CommandId("cmd_001");

    return rec;
}

void test_valid_tick_record_passes() {
    auto rec = makeValidTickRecord();
    assert(rec.validateBasic().is_ok());
    std::cout << "  PASS: valid_tick_record_passes\n";
}

void test_missing_input_hash_fails() {
    auto rec = makeValidTickRecord();
    rec.input_hash = HashValue();
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: missing_input_hash_fails\n";
}

void test_missing_output_hash_fails() {
    auto rec = makeValidTickRecord();
    rec.output_hash = HashValue();
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: missing_output_hash_fails\n";
}

void test_pipeline_succeeded_requires_pipeline_status() {
    auto rec = makeValidTickRecord();
    rec.pipeline_status = std::nullopt;
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: pipeline_succeeded_requires_pipeline_status\n";
}

void test_submit_skipped_no_command_required() {
    auto rec = makeValidTickRecord();
    rec.runtime_status = AgentRuntimeStatus::SubmitSkipped;
    rec.record_status = AgentTickRecordStatus::Recorded;
    rec.command = std::nullopt;
    rec.pipeline_status = std::nullopt;
    rec.decision_record.status = AgentDecisionRecordStatus::SubmitSkipped;
    rec.decision_record.command_id = std::nullopt;
    assert(rec.validateBasic().is_ok());
    std::cout << "  PASS: submit_skipped_no_command_required\n";
}

void test_unknown_record_status_fails() {
    auto rec = makeValidTickRecord();
    rec.record_status = AgentTickRecordStatus::Unknown;
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: unknown_record_status_fails\n";
}

void test_unknown_runtime_status_fails() {
    auto rec = makeValidTickRecord();
    rec.runtime_status = AgentRuntimeStatus::Unknown;
    assert(rec.validateBasic().is_error());
    std::cout << "  PASS: unknown_runtime_status_fails\n";
}

// --- AgentTickLog tests ---

void test_append_valid_record() {
    AgentTickLog log;
    auto rec = makeValidTickRecord();
    assert(log.append(rec).is_ok());
    assert(log.size() == 1);
    std::cout << "  PASS: append_valid_record\n";
}

void test_append_duplicate_rejected() {
    AgentTickLog log;
    auto rec = makeValidTickRecord();
    assert(log.append(rec).is_ok());
    assert(log.append(rec).is_error());
    assert(log.size() == 1);
    std::cout << "  PASS: append_duplicate_rejected\n";
}

void test_append_invalid_rejected() {
    AgentTickLog log;
    auto rec = makeValidTickRecord();
    rec.record_id = AgentTickRecordId();
    assert(log.append(rec).is_error());
    assert(log.size() == 0);
    std::cout << "  PASS: append_invalid_rejected\n";
}

void test_find_by_record_id() {
    AgentTickLog log;
    auto rec = makeValidTickRecord();
    log.append(rec);
    auto found = log.findByRecordId(rec.record_id);
    assert(found.has_value());
    assert(found.value().record_id == rec.record_id);
    std::cout << "  PASS: find_by_record_id\n";
}

void test_find_by_agent_id() {
    AgentTickLog log;
    auto rec = makeValidTickRecord();
    log.append(rec);
    auto found = log.findByAgentId(AgentId("agent_001"));
    assert(found.size() == 1);
    assert(found[0].agent_id == AgentId("agent_001"));
    std::cout << "  PASS: find_by_agent_id\n";
}

void test_find_by_decision_id() {
    AgentTickLog log;
    auto rec = makeValidTickRecord();
    log.append(rec);
    auto found = log.findByDecisionId(DecisionId("dec_001"));
    assert(found.has_value());
    std::cout << "  PASS: find_by_decision_id\n";
}

void test_empty_log_valid() {
    AgentTickLog log;
    assert(log.validateBasic().is_ok());
    std::cout << "  PASS: empty_log_valid\n";
}

void test_find_nonexistent_returns_empty() {
    AgentTickLog log;
    assert(!log.findByRecordId(AgentTickRecordId("nonexistent")).has_value());
    assert(log.findByAgentId(AgentId("nonexistent")).empty());
    assert(!log.findByDecisionId(DecisionId("nonexistent")).has_value());
    std::cout << "  PASS: find_nonexistent_returns_empty\n";
}

// --- AgentRecordBuildRequest validation ---

void test_build_request_missing_hash_fails() {
    AgentRecordBuildRequest req;
    // tick_request and tick_result are default (invalid), but hash check comes first
    req.input_hash = HashValue();
    req.output_hash = HashValue(1);
    assert(req.validateBasic().is_error());

    req.input_hash = HashValue(1);
    req.output_hash = HashValue();
    assert(req.validateBasic().is_error());
    std::cout << "  PASS: build_request_missing_hash_fails\n";
}

// --- Main ---

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "Agent record types smoke test passed" << std::endl;
        return 0;
    }

    // Enum roundtrip
    if (test_name == "decision_record_status_roundtrip") { test_decision_record_status_roundtrip(); return 0; }
    if (test_name == "tick_record_status_roundtrip") { test_tick_record_status_roundtrip(); return 0; }

    // ID helpers
    if (test_name == "id_helpers") { test_id_helpers(); return 0; }

    // Decision record validation
    if (test_name == "valid_decision_record_passes") { test_valid_decision_record_passes(); return 0; }
    if (test_name == "missing_record_id_fails") { test_missing_record_id_fails(); return 0; }
    if (test_name == "unknown_status_fails") { test_unknown_status_fails(); return 0; }
    if (test_name == "confidence_out_of_range_fails") { test_confidence_out_of_range_fails(); return 0; }
    if (test_name == "pipeline_succeeded_requires_command_id") { test_pipeline_succeeded_requires_command_id(); return 0; }
    if (test_name == "no_decision_allows_empty_decision_id") { test_no_decision_allows_empty_decision_id(); return 0; }
    if (test_name == "no_decision_no_explanation_fails") { test_no_decision_no_explanation_fails(); return 0; }
    if (test_name == "no_decision_with_phase_keys_passes") { test_no_decision_with_phase_keys_passes(); return 0; }

    // Tick record validation
    if (test_name == "valid_tick_record_passes") { test_valid_tick_record_passes(); return 0; }
    if (test_name == "missing_input_hash_fails") { test_missing_input_hash_fails(); return 0; }
    if (test_name == "missing_output_hash_fails") { test_missing_output_hash_fails(); return 0; }
    if (test_name == "pipeline_succeeded_requires_pipeline_status") { test_pipeline_succeeded_requires_pipeline_status(); return 0; }
    if (test_name == "submit_skipped_no_command_required") { test_submit_skipped_no_command_required(); return 0; }
    if (test_name == "unknown_record_status_fails") { test_unknown_record_status_fails(); return 0; }
    if (test_name == "unknown_runtime_status_fails") { test_unknown_runtime_status_fails(); return 0; }

    // AgentTickLog
    if (test_name == "append_valid_record") { test_append_valid_record(); return 0; }
    if (test_name == "append_duplicate_rejected") { test_append_duplicate_rejected(); return 0; }
    if (test_name == "append_invalid_rejected") { test_append_invalid_rejected(); return 0; }
    if (test_name == "find_by_record_id") { test_find_by_record_id(); return 0; }
    if (test_name == "find_by_agent_id") { test_find_by_agent_id(); return 0; }
    if (test_name == "find_by_decision_id") { test_find_by_decision_id(); return 0; }
    if (test_name == "empty_log_valid") { test_empty_log_valid(); return 0; }
    if (test_name == "find_nonexistent_returns_empty") { test_find_nonexistent_returns_empty(); return 0; }

    // Build request
    if (test_name == "build_request_missing_hash_fails") { test_build_request_missing_hash_fails(); return 0; }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
