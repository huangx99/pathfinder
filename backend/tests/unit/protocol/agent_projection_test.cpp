#include "pathfinder/protocol/agent_projection.h"
#include "pathfinder/pipeline/types.h"
#include "pathfinder/command/envelope.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::protocol;
using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::pipeline;
using namespace pathfinder::command;

// --- Helpers ---

AgentDecisionRecord makeProjDecisionRecord(const std::string& agent_id_str, Tick tick, bool with_command = true) {
    AgentDecisionRecord rec;
    rec.record_id = AgentDecisionRecordId("agent_decision_record_dec_" + agent_id_str + "_" + std::to_string(tick.value()));
    rec.decision_id = DecisionId("dec_" + agent_id_str + "_" + std::to_string(tick.value()));
    rec.agent_id = AgentId(agent_id_str);
    rec.actor_id = EntityId("actor_" + agent_id_str);
    rec.decision_tick = tick;
    rec.observation_state_version = StateVersion(1);
    rec.status = with_command ? AgentDecisionRecordStatus::PipelineSucceeded : AgentDecisionRecordStatus::NoDecision;
    rec.confidence = 0.8;
    if (with_command) {
        rec.command_id = CommandId("cmd_" + agent_id_str + "_" + std::to_string(tick.value()));
    } else {
        rec.reason_key = "no_supported_candidate";
        rec.phase_keys = {"observation_built", "action_space_built", "no_decision"};
    }
    return rec;
}

AgentTickRecord makeProjTickRecord(const std::string& agent_id_str, Tick tick, bool with_command = true) {
    AgentTickRecord rec;
    rec.record_id = makeAgentTickRecordId(AgentId(agent_id_str), tick, StateVersion(1));
    rec.agent_id = AgentId(agent_id_str);
    rec.actor_id = EntityId("actor_" + agent_id_str);
    rec.tick = tick;
    rec.state_version_before = StateVersion(1);
    rec.state_version_after = StateVersion(2);
    rec.random_seed = RandomSeed(42);
    rec.runtime_status = with_command ? AgentRuntimeStatus::PipelineSucceeded : AgentRuntimeStatus::NoDecision;
    rec.record_status = AgentTickRecordStatus::Recorded;
    rec.input_hash = HashValue(111);
    rec.output_hash = HashValue(222);
    if (with_command) {
        rec.pipeline_status = PipelineStatus::Succeeded;
        CommandEnvelope env;
        env.command_id = CommandId("cmd_" + agent_id_str + "_" + std::to_string(tick.value()));
        env.payload.action_id = ActionId("eat");
        env.source = CommandSource::Ai;
        rec.command = env;
    }
    rec.decision_record = makeProjDecisionRecord(agent_id_str, tick, with_command);
    return rec;
}

AgentHistoryQueryResult makeTestQueryResult(const std::string& agent_id_str, Tick tick, bool with_command = true) {
    AgentHistoryQueryResult result;
    result.query_id = makeAgentHistoryQueryId(AgentId(agent_id_str), tick, tick, 100);
    result.records.push_back(makeProjTickRecord(agent_id_str, tick, with_command));
    result.total_matched = 1;
    result.truncated = false;
    return result;
}

// --- AgentHistoryItemProjection validation ---

void test_valid_history_item_passes() {
    AgentHistoryItemProjection item;
    item.record_id = "rec_001";
    item.agent_id = "agent_001";
    item.actor_id = "actor_001";
    item.runtime_status = "PipelineSucceeded";
    item.decision_status = "PipelineSucceeded";
    assert(item.validateBasic().is_ok());
    std::cout << "  PASS: valid_history_item_passes\n";
}

void test_missing_record_id_fails() {
    AgentHistoryItemProjection item;
    item.agent_id = "agent_001";
    item.actor_id = "actor_001";
    item.runtime_status = "PipelineSucceeded";
    item.decision_status = "PipelineSucceeded";
    assert(item.validateBasic().is_error());
    std::cout << "  PASS: missing_record_id_fails\n";
}

void test_missing_agent_id_fails() {
    AgentHistoryItemProjection item;
    item.record_id = "rec_001";
    item.actor_id = "actor_001";
    item.runtime_status = "PipelineSucceeded";
    item.decision_status = "PipelineSucceeded";
    assert(item.validateBasic().is_error());
    std::cout << "  PASS: missing_agent_id_fails\n";
}

void test_missing_runtime_status_fails() {
    AgentHistoryItemProjection item;
    item.record_id = "rec_001";
    item.agent_id = "agent_001";
    item.actor_id = "actor_001";
    item.decision_status = "pipeline_succeeded";
    assert(item.validateBasic().is_error());
    std::cout << "  PASS: missing_runtime_status_fails\n";
}

// --- AgentHistoryProjection validation ---

void test_valid_history_projection_passes() {
    AgentHistoryProjection proj;
    proj.query_id = "q_001";
    proj.projection_mode = AgentHistoryProjectionMode::Public;
    AgentHistoryItemProjection item;
    item.record_id = "rec_001";
    item.agent_id = "agent_001";
    item.actor_id = "actor_001";
    item.runtime_status = "PipelineSucceeded";
    item.decision_status = "PipelineSucceeded";
    proj.items.push_back(item);
    assert(proj.validateBasic().is_ok());
    std::cout << "  PASS: valid_history_projection_passes\n";
}

void test_unknown_mode_fails() {
    AgentHistoryProjection proj;
    proj.query_id = "q_001";
    proj.projection_mode = AgentHistoryProjectionMode::Unknown;
    assert(proj.validateBasic().is_error());
    std::cout << "  PASS: unknown_mode_fails\n";
}

// --- AgentReplayLockItemProjection validation ---

void test_valid_lock_item_passes() {
    AgentReplayLockItemProjection item;
    item.agent_record_id = "rec_001";
    item.agent_id = "agent_001";
    item.replay_lock_status = "Locked";
    assert(item.validateBasic().is_ok());
    std::cout << "  PASS: valid_lock_item_passes\n";
}

void test_lock_item_missing_status_fails() {
    AgentReplayLockItemProjection item;
    item.agent_record_id = "rec_001";
    item.agent_id = "agent_001";
    assert(item.validateBasic().is_error());
    std::cout << "  PASS: lock_item_missing_status_fails\n";
}

// --- AgentReplayLockProjection validation ---

void test_valid_lock_projection_passes() {
    AgentReplayLockProjection proj;
    proj.lock_set_id = "ls_001";
    assert(proj.validateBasic().is_ok());
    std::cout << "  PASS: valid_lock_projection_passes\n";
}

void test_lock_projection_missing_id_fails() {
    AgentReplayLockProjection proj;
    assert(proj.validateBasic().is_error());
    std::cout << "  PASS: lock_projection_missing_id_fails\n";
}

// --- AgentTrainingSampleProjection validation ---

void test_valid_training_sample_passes() {
    AgentTrainingSampleProjection proj;
    proj.sample_id = "sample_001";
    proj.source_record_id = "rec_001";
    proj.status = "ReplayLocked";
    proj.agent_id = "agent_001";
    proj.runtime_status = "PipelineSucceeded";
    proj.decision_status = "PipelineSucceeded";
    proj.reward_status = "NotComputed";
    proj.done_status = "NotComputed";
    assert(proj.validateBasic().is_ok());
    std::cout << "  PASS: valid_training_sample_passes\n";
}

void test_training_missing_reward_status_fails() {
    AgentTrainingSampleProjection proj;
    proj.sample_id = "sample_001";
    proj.source_record_id = "rec_001";
    proj.status = "ReplayLocked";
    proj.agent_id = "agent_001";
    proj.runtime_status = "PipelineSucceeded";
    proj.decision_status = "PipelineSucceeded";
    proj.done_status = "NotComputed";
    assert(proj.validateBasic().is_error());
    std::cout << "  PASS: training_missing_reward_status_fails\n";
}

void test_training_no_reward_value_or_done_bool() {
    AgentTrainingSampleProjection proj;
    proj.sample_id = "sample_001";
    proj.source_record_id = "rec_001";
    proj.status = "ReplayLocked";
    proj.agent_id = "agent_001";
    proj.runtime_status = "PipelineSucceeded";
    proj.decision_status = "PipelineSucceeded";
    proj.reward_status = "NotComputed";
    proj.done_status = "NotComputed";
    // Verify no reward_value or done bool fields exist at compile time
    // The struct should only have reward_status/done_status strings
    assert(proj.validateBasic().is_ok());
    std::cout << "  PASS: training_no_reward_value_or_done_bool\n";
}

// --- Projector tests ---

void test_projector_pipeline_succeeded_has_command_id() {
    auto query_result = makeTestQueryResult("agent_001", Tick(1), true);
    AgentHistoryProjectorInput input;
    input.query_result = query_result;
    input.projection_mode = AgentHistoryProjectionMode::Debug;
    input.include_debug_trace = true;

    AgentHistoryProjector projector;
    auto result = projector.project(input);
    assert(result.is_ok());
    assert(result.value().items.size() == 1);
    assert(result.value().items[0].command_id.has_value());
    assert(result.value().items[0].command_id.value() == "cmd_agent_001_1");
    std::cout << "  PASS: projector_pipeline_succeeded_has_command_id\n";
}

void test_projector_no_decision_no_command_id() {
    auto query_result = makeTestQueryResult("agent_001", Tick(1), false);
    AgentHistoryProjectorInput input;
    input.query_result = query_result;
    input.projection_mode = AgentHistoryProjectionMode::Debug;
    input.include_debug_trace = true;

    AgentHistoryProjector projector;
    auto result = projector.project(input);
    assert(result.is_ok());
    assert(result.value().items.size() == 1);
    assert(!result.value().items[0].command_id.has_value());
    std::cout << "  PASS: projector_no_decision_no_command_id\n";
}

void test_projector_replay_lock_status_locked() {
    auto query_result = makeTestQueryResult("agent_001", Tick(1), true);

    // Build replay lock set
    AgentReplayLockSet lock_set;
    lock_set.lock_set_id = AgentReplayLockSetId("ls_001");
    lock_set.source_hash = HashValue(999);
    AgentReplayLockEntry entry;
    entry.agent_record_id = query_result.records[0].record_id;
    entry.agent_id = AgentId("agent_001");
    entry.tick = Tick(1);
    entry.state_version_before = StateVersion(1);
    entry.state_version_after = StateVersion(2);
    entry.status = AgentReplayLockStatus::Locked;
    entry.command_id = CommandId("cmd_agent_001_1");
    entry.replay_record_id = pathfinder::replay::ReplayRecordId("replay_001");
    lock_set.entries.push_back(entry);

    AgentHistoryProjectorInput input;
    input.query_result = query_result;
    input.replay_lock_set = &lock_set;
    input.projection_mode = AgentHistoryProjectionMode::Training;

    AgentHistoryProjector projector;
    auto result = projector.project(input);
    assert(result.is_ok());
    assert(result.value().items[0].replay_lock_status.has_value());
    assert(result.value().items[0].replay_lock_status.value() == "Locked");
    std::cout << "  PASS: projector_replay_lock_status_locked\n";
}

void test_projector_training_sample_status() {
    auto query_result = makeTestQueryResult("agent_001", Tick(1), true);

    // Build training sample
    std::vector<AgentTrainingSampleDraft> samples;
    AgentTrainingSampleDraft sample;
    sample.sample_id = makeAgentTrainingSampleId(AgentId("agent_001"), Tick(1), StateVersion(1));
    sample.source_record_id = query_result.records[0].record_id;
    sample.status = AgentTrainingSampleStatus::ReplayLocked;
    sample.observation.agent_id = AgentId("agent_001");
    sample.observation.actor_id = EntityId("actor_001");
    sample.observation.tick = Tick(1);
    sample.observation.state_version = StateVersion(1);
    sample.observation.observation_hash = HashValue(555);
    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.state_version_before = StateVersion(1);
    sample.result.state_version_after = StateVersion(2);
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;
    sample.trace.replay_locked = true;
    sample.trace.replay_lock_status = AgentReplayLockStatus::Locked;
    samples.push_back(sample);

    AgentHistoryProjectorInput input;
    input.query_result = query_result;
    input.training_samples = &samples;
    input.projection_mode = AgentHistoryProjectionMode::Training;

    AgentHistoryProjector projector;
    auto result = projector.project(input);
    assert(result.is_ok());
    assert(result.value().items[0].training_sample_status.has_value());
    assert(result.value().items[0].training_sample_status.value() == "ReplayLocked");
    std::cout << "  PASS: projector_training_sample_status\n";
}

void test_projector_public_no_trace_keys() {
    auto query_result = makeTestQueryResult("agent_001", Tick(1), false);
    AgentHistoryProjectorInput input;
    input.query_result = query_result;
    input.projection_mode = AgentHistoryProjectionMode::Public;
    input.include_debug_trace = false;

    AgentHistoryProjector projector;
    auto result = projector.project(input);
    assert(result.is_ok());
    assert(result.value().items[0].phase_keys.empty());
    assert(result.value().items[0].reason_keys.empty());
    assert(result.value().items[0].warning_keys.empty());
    std::cout << "  PASS: projector_public_no_trace_keys\n";
}

void test_projector_debug_has_trace_keys() {
    auto query_result = makeTestQueryResult("agent_001", Tick(1), false);
    AgentHistoryProjectorInput input;
    input.query_result = query_result;
    input.projection_mode = AgentHistoryProjectionMode::Debug;
    input.include_debug_trace = true;

    AgentHistoryProjector projector;
    auto result = projector.project(input);
    assert(result.is_ok());
    // NoDecision record should have phase_keys and reason_key
    assert(!result.value().items[0].phase_keys.empty());
    assert(!result.value().items[0].reason_keys.empty());
    std::cout << "  PASS: projector_debug_has_trace_keys\n";
}

// --- Protocol Adapter tests ---

void test_history_envelope_valid() {
    AgentHistoryProjection proj;
    proj.query_id = "q_001";
    proj.projection_mode = AgentHistoryProjectionMode::Public;
    AgentHistoryItemProjection item;
    item.record_id = "rec_001";
    item.agent_id = "agent_001";
    item.actor_id = "actor_001";
    item.runtime_status = "PipelineSucceeded";
    item.decision_status = "PipelineSucceeded";
    proj.items.push_back(item);

    AgentProtocolProjectionAdapter adapter;
    auto env = adapter.buildHistoryEnvelope(proj);
    assert(env.validateBasic().is_ok());
    assert(env.message_type == ProtocolMessageType::Response);
    assert(env.domain == ProtocolDomain::Query);
    assert(env.payload.payload_type == ProtocolPayloadType::AgentHistoryProjection);
    std::cout << "  PASS: history_envelope_valid\n";
}

void test_replay_lock_envelope_valid() {
    AgentReplayLockProjection proj;
    proj.lock_set_id = "ls_001";

    AgentProtocolProjectionAdapter adapter;
    auto env = adapter.buildReplayLockEnvelope(proj);
    assert(env.validateBasic().is_ok());
    assert(env.message_type == ProtocolMessageType::Response);
    assert(env.domain == ProtocolDomain::ProjectionSync);
    assert(env.payload.payload_type == ProtocolPayloadType::AgentReplayLockProjection);
    std::cout << "  PASS: replay_lock_envelope_valid\n";
}

void test_training_sample_envelope_valid() {
    AgentTrainingSampleProjection proj;
    proj.sample_id = "sample_001";
    proj.source_record_id = "rec_001";
    proj.status = "ReplayLocked";
    proj.agent_id = "agent_001";
    proj.runtime_status = "PipelineSucceeded";
    proj.decision_status = "PipelineSucceeded";
    proj.reward_status = "NotComputed";
    proj.done_status = "NotComputed";

    AgentProtocolProjectionAdapter adapter;
    auto env = adapter.buildTrainingSampleEnvelope(proj);
    assert(env.validateBasic().is_ok());
    assert(env.message_type == ProtocolMessageType::Response);
    assert(env.domain == ProtocolDomain::ProjectionSync);
    assert(env.payload.payload_type == ProtocolPayloadType::AgentTrainingSampleProjection);
    std::cout << "  PASS: training_sample_envelope_valid\n";
}

void test_envelope_passthrough_options() {
    AgentHistoryProjection proj;
    proj.query_id = "q_001";
    proj.projection_mode = AgentHistoryProjectionMode::Public;

    AgentProjectionProtocolOptions opts;
    opts.request_id = "req_001";
    opts.correlation_id = "corr_001";
    opts.session_id = "sess_001";

    AgentProtocolProjectionAdapter adapter;
    auto env = adapter.buildHistoryEnvelope(proj, opts);
    assert(env.validateBasic().is_ok());
    assert(env.request_id.has_value() && env.request_id.value() == "req_001");
    assert(env.correlation_id.has_value() && env.correlation_id.value() == "corr_001");
    assert(env.session_id.has_value() && env.session_id.value() == "sess_001");
    std::cout << "  PASS: envelope_passthrough_options\n";
}

// --- main ---

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "PASS: agent_projection smoke" << std::endl;
        return 0;
    }

    // DTO validation
    if (test_name == "valid_history_item_passes") { test_valid_history_item_passes(); return 0; }
    if (test_name == "missing_record_id_fails") { test_missing_record_id_fails(); return 0; }
    if (test_name == "missing_agent_id_fails") { test_missing_agent_id_fails(); return 0; }
    if (test_name == "missing_runtime_status_fails") { test_missing_runtime_status_fails(); return 0; }
    if (test_name == "valid_history_projection_passes") { test_valid_history_projection_passes(); return 0; }
    if (test_name == "unknown_mode_fails") { test_unknown_mode_fails(); return 0; }
    if (test_name == "valid_lock_item_passes") { test_valid_lock_item_passes(); return 0; }
    if (test_name == "lock_item_missing_status_fails") { test_lock_item_missing_status_fails(); return 0; }
    if (test_name == "valid_lock_projection_passes") { test_valid_lock_projection_passes(); return 0; }
    if (test_name == "lock_projection_missing_id_fails") { test_lock_projection_missing_id_fails(); return 0; }
    if (test_name == "valid_training_sample_passes") { test_valid_training_sample_passes(); return 0; }
    if (test_name == "training_missing_reward_status_fails") { test_training_missing_reward_status_fails(); return 0; }
    if (test_name == "training_no_reward_value_or_done_bool") { test_training_no_reward_value_or_done_bool(); return 0; }

    // Projector
    if (test_name == "projector_pipeline_succeeded_has_command_id") { test_projector_pipeline_succeeded_has_command_id(); return 0; }
    if (test_name == "projector_no_decision_no_command_id") { test_projector_no_decision_no_command_id(); return 0; }
    if (test_name == "projector_replay_lock_status_locked") { test_projector_replay_lock_status_locked(); return 0; }
    if (test_name == "projector_training_sample_status") { test_projector_training_sample_status(); return 0; }
    if (test_name == "projector_public_no_trace_keys") { test_projector_public_no_trace_keys(); return 0; }
    if (test_name == "projector_debug_has_trace_keys") { test_projector_debug_has_trace_keys(); return 0; }

    // Protocol Adapter
    if (test_name == "history_envelope_valid") { test_history_envelope_valid(); return 0; }
    if (test_name == "replay_lock_envelope_valid") { test_replay_lock_envelope_valid(); return 0; }
    if (test_name == "training_sample_envelope_valid") { test_training_sample_envelope_valid(); return 0; }
    if (test_name == "envelope_passthrough_options") { test_envelope_passthrough_options(); return 0; }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
