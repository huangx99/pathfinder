#include "pathfinder/agent/history_query.h"
#include "pathfinder/protocol/agent_projection.h"
#include "pathfinder/agent/replay_lock.h"
#include "pathfinder/agent/training_sample.h"
#include "pathfinder/pipeline/types.h"
#include "pathfinder/command/envelope.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::protocol;
using namespace pathfinder::foundation;
using namespace pathfinder::pipeline;
using namespace pathfinder::command;
using namespace pathfinder::replay;

// --- Helper: Build a NoDecision AgentTickRecord ---

AgentTickRecord buildNoDecisionTickRecord() {
    AgentTickRecord rec;
    rec.record_id = makeAgentTickRecordId(AgentId("agent_player_002"), Tick(5), StateVersion(3));
    rec.agent_id = AgentId("agent_player_002");
    rec.actor_id = EntityId("actor_002");
    rec.tick = Tick(5);
    rec.state_version_before = StateVersion(3);
    rec.state_version_after = StateVersion(3);
    rec.random_seed = RandomSeed(99);
    rec.runtime_status = AgentRuntimeStatus::NoDecision;
    rec.record_status = AgentTickRecordStatus::Recorded;
    rec.input_hash = HashValue(333);
    rec.output_hash = HashValue(333);
    // No pipeline_status, no command

    // Decision record: NoDecision with explanation
    rec.decision_record.record_id = AgentDecisionRecordId("agent_decision_record_dec_nodecision_001");
    rec.decision_record.decision_id = DecisionId("dec_nodecision_001");
    rec.decision_record.agent_id = AgentId("agent_player_002");
    rec.decision_record.actor_id = EntityId("actor_002");
    rec.decision_record.decision_tick = Tick(5);
    rec.decision_record.observation_state_version = StateVersion(3);
    rec.decision_record.status = AgentDecisionRecordStatus::NoDecision;
    rec.decision_record.confidence = 0.0;
    rec.decision_record.reason_key = "no_supported_candidate";
    rec.decision_record.phase_keys = {"observation_built", "action_space_built", "no_decision"};

    // Tick record phase_keys
    rec.phase_keys = {"observation_built", "action_space_built", "no_decision"};

    return rec;
}

// --- Integration test ---

void run_no_decision_projection_flow() {
    // Step 1: Build P8 NoDecision AgentTickRecord
    auto tick_record = buildNoDecisionTickRecord();
    assert(tick_record.validateBasic().is_ok());
    std::cout << "  PASS: p10_no_dec_tick_record_built\n";

    // Step 2: Build AgentTickLog
    AgentTickLog tick_log;
    auto append_result = tick_log.append(tick_record);
    assert(append_result.is_ok());
    std::cout << "  PASS: p10_no_dec_tick_log_built\n";

    // Step 3: Build P9 ExplainedOnly ReplayLockEntry
    AgentReplayLockSet lock_set;
    lock_set.lock_set_id = AgentReplayLockSetId("ls_no_decision_001");
    lock_set.source_hash = HashValue(888);

    AgentReplayLockEntry lock_entry;
    lock_entry.agent_record_id = tick_record.record_id;
    lock_entry.agent_id = AgentId("agent_player_002");
    lock_entry.tick = Tick(5);
    lock_entry.state_version_before = StateVersion(3);
    lock_entry.state_version_after = StateVersion(3);
    lock_entry.status = AgentReplayLockStatus::ExplainedOnly;
    lock_entry.reasons.push_back(AgentReplayLockReason::NoCommandExpected);
    lock_entry.reason_keys.push_back("no_command_expected");
    lock_set.entries.push_back(lock_entry);

    assert(lock_set.validateBasic().is_ok());
    assert(lock_set.allReplayableWithoutPolicy()); // ExplainedOnly counts as replayable
    assert(!lock_set.hasBrokenEntry());
    std::cout << "  PASS: p10_no_dec_lock_set_built\n";

    // Step 4: Build P9 NoDecision TrainingSampleDraft
    std::vector<AgentTrainingSampleDraft> training_samples;
    AgentTrainingSampleDraft sample;
    sample.sample_id = makeAgentTrainingSampleId(AgentId("agent_player_002"), Tick(5), StateVersion(3));
    sample.source_record_id = tick_record.record_id;
    sample.status = AgentTrainingSampleStatus::NoDecision;

    sample.observation.agent_id = AgentId("agent_player_002");
    sample.observation.actor_id = EntityId("actor_002");
    sample.observation.tick = Tick(5);
    sample.observation.state_version = StateVersion(3);
    sample.observation.observation_hash = HashValue(666);

    sample.action.decision_id = DecisionId("dec_nodecision_001");
    sample.action.decision_status = AgentDecisionRecordStatus::NoDecision;

    sample.result.runtime_status = AgentRuntimeStatus::NoDecision;
    sample.result.state_version_before = StateVersion(3);
    sample.result.state_version_after = StateVersion(3);
    sample.result.input_hash = HashValue(333);
    sample.result.output_hash = HashValue(333);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;

    sample.trace.replay_locked = false;
    sample.trace.replay_lock_status = AgentReplayLockStatus::ExplainedOnly;
    sample.trace.phase_keys = {"observation_built", "action_space_built", "no_decision"};
    sample.trace.reason_keys = {"no_supported_candidate"};

    assert(sample.validateBasic().is_ok());
    training_samples.push_back(sample);
    std::cout << "  PASS: p10_no_dec_training_sample_built\n";

    // Step 5: Debug mode projection
    AgentHistoryQueryService query_service;
    AgentHistoryQueryOptions query_opts;
    query_opts.query_id = makeAgentHistoryQueryId(AgentId("agent_player_002"), std::nullopt, std::nullopt, 100);
    query_opts.agent_id = AgentId("agent_player_002");
    query_opts.sort_order = AgentHistorySortOrder::TickAscending;
    query_opts.projection_mode = AgentHistoryProjectionMode::Debug;
    query_opts.include_debug_trace = true;

    auto query_result = query_service.query(tick_log, query_opts);
    assert(query_result.is_ok());
    assert(query_result.value().records.size() == 1);

    AgentHistoryProjectorInput proj_input;
    proj_input.query_result = query_result.value();
    proj_input.replay_lock_set = &lock_set;
    proj_input.training_samples = &training_samples;
    proj_input.projection_mode = AgentHistoryProjectionMode::Debug;
    proj_input.include_debug_trace = true;

    AgentHistoryProjector projector;
    auto debug_projection = projector.project(proj_input);
    assert(debug_projection.is_ok());
    const auto& debug_item = debug_projection.value().items[0];
    std::cout << "  PASS: p10_no_dec_debug_projection\n";

    // Step 6: Assert command_id is empty (NoDecision)
    assert(!debug_item.command_id.has_value());
    std::cout << "  PASS: p10_no_dec_command_id_empty\n";

    // Step 7: Assert replay_lock_status=ExplainedOnly
    assert(debug_item.replay_lock_status.has_value());
    assert(debug_item.replay_lock_status.value() == "ExplainedOnly");
    std::cout << "  PASS: p10_no_dec_replay_lock_explained_only\n";

    // Step 8: Assert training_sample_status=NoDecision
    assert(debug_item.training_sample_status.has_value());
    assert(debug_item.training_sample_status.value() == "NoDecision");
    std::cout << "  PASS: p10_no_dec_training_sample_no_decision\n";

    // Step 9: Assert reason_keys / phase_keys visible in Debug mode
    assert(!debug_item.phase_keys.empty());
    assert(!debug_item.reason_keys.empty());
    std::cout << "  PASS: p10_no_dec_debug_trace_keys_visible\n";

    // Step 10: Public mode re-project, assert trace keys cleared
    AgentHistoryQueryOptions public_opts;
    public_opts.query_id = makeAgentHistoryQueryId(AgentId("agent_player_002"), std::nullopt, std::nullopt, 100);
    public_opts.agent_id = AgentId("agent_player_002");
    public_opts.sort_order = AgentHistorySortOrder::TickAscending;
    public_opts.projection_mode = AgentHistoryProjectionMode::Public;
    public_opts.include_debug_trace = false;

    auto public_query = query_service.query(tick_log, public_opts);
    assert(public_query.is_ok());

    AgentHistoryProjectorInput public_proj_input;
    public_proj_input.query_result = public_query.value();
    public_proj_input.replay_lock_set = &lock_set;
    public_proj_input.training_samples = &training_samples;
    public_proj_input.projection_mode = AgentHistoryProjectionMode::Public;
    public_proj_input.include_debug_trace = false;

    auto public_projection = projector.project(public_proj_input);
    assert(public_projection.is_ok());
    const auto& public_item = public_projection.value().items[0];
    assert(public_item.phase_keys.empty());
    assert(public_item.reason_keys.empty());
    assert(public_item.warning_keys.empty());
    std::cout << "  PASS: p10_no_dec_public_trace_keys_cleared\n";

    // Step 11: Envelope validation
    AgentProtocolProjectionAdapter adapter;
    auto envelope = adapter.buildHistoryEnvelope(debug_projection.value());
    assert(envelope.validateBasic().is_ok());
    assert(envelope.payload.payload_type == ProtocolPayloadType::AgentHistoryProjection);
    std::cout << "  PASS: p10_no_dec_envelope_valid\n";
}

// --- main ---

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "PASS: p10_agent_no_decision_projection_flow smoke" << std::endl;
        return 0;
    }

    if (test_name == "p10_agent_no_decision_projection_flow") {
        run_no_decision_projection_flow();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
