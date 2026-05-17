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

// --- Helper: Build a PipelineSucceeded AgentTickRecord ---

AgentTickRecord buildUnknownFruitTickRecord() {
    AgentTickRecord rec;
    rec.record_id = makeAgentTickRecordId(AgentId("agent_player_001"), Tick(0), StateVersion(1));
    rec.agent_id = AgentId("agent_player_001");
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

    // Command envelope
    CommandEnvelope env;
    env.command_id = CommandId("cmd_eat_fruit_001");
    env.payload.action_id = ActionId("eat");
    env.source = CommandSource::Ai;
    rec.command = env;

    // Decision record
    rec.decision_record.record_id = AgentDecisionRecordId("agent_decision_record_dec_fruit_001");
    rec.decision_record.decision_id = DecisionId("dec_fruit_001");
    rec.decision_record.agent_id = AgentId("agent_player_001");
    rec.decision_record.actor_id = EntityId("actor_001");
    rec.decision_record.decision_tick = Tick(0);
    rec.decision_record.observation_state_version = StateVersion(1);
    rec.decision_record.selected_candidate_id = ActionId("eat");
    rec.decision_record.selected_command_action_id = ActionId("eat");
    rec.decision_record.status = AgentDecisionRecordStatus::PipelineSucceeded;
    rec.decision_record.confidence = 0.9;
    rec.decision_record.command_id = CommandId("cmd_eat_fruit_001");

    return rec;
}

// --- Integration test ---

void run_unknown_fruit_history_projection_flow() {
    // Step 1: Build P8 unknown fruit AgentTickRecord
    auto tick_record = buildUnknownFruitTickRecord();
    assert(tick_record.validateBasic().is_ok());
    std::cout << "  PASS: p10_tick_record_built\n";

    // Step 2: Build AgentTickLog
    AgentTickLog tick_log;
    auto append_result = tick_log.append(tick_record);
    assert(append_result.is_ok());
    assert(tick_log.size() == 1);
    std::cout << "  PASS: p10_tick_log_built\n";

    // Step 3: Build P9 Locked ReplayLockEntry / AgentReplayLockSet
    AgentReplayLockSet lock_set;
    lock_set.lock_set_id = AgentReplayLockSetId("ls_unknown_fruit_001");
    lock_set.source_hash = HashValue(777);

    AgentReplayLockEntry lock_entry;
    lock_entry.agent_record_id = tick_record.record_id;
    lock_entry.agent_id = AgentId("agent_player_001");
    lock_entry.tick = Tick(0);
    lock_entry.state_version_before = StateVersion(1);
    lock_entry.state_version_after = StateVersion(2);
    lock_entry.status = AgentReplayLockStatus::Locked;
    lock_entry.command_id = CommandId("cmd_eat_fruit_001");
    lock_entry.replay_record_id = ReplayRecordId("replay_eat_fruit_001");
    lock_set.entries.push_back(lock_entry);

    assert(lock_set.validateBasic().is_ok());
    assert(lock_set.allReplayableWithoutPolicy());
    std::cout << "  PASS: p10_replay_lock_set_built\n";

    // Step 4: Build P9 ReplayLocked TrainingSampleDraft
    std::vector<AgentTrainingSampleDraft> training_samples;
    AgentTrainingSampleDraft sample;
    sample.sample_id = makeAgentTrainingSampleId(AgentId("agent_player_001"), Tick(0), StateVersion(1));
    sample.source_record_id = tick_record.record_id;
    sample.status = AgentTrainingSampleStatus::ReplayLocked;

    sample.observation.agent_id = AgentId("agent_player_001");
    sample.observation.actor_id = EntityId("actor_001");
    sample.observation.tick = Tick(0);
    sample.observation.state_version = StateVersion(1);
    sample.observation.observed_object_count = 1;
    sample.observation.observation_hash = HashValue(555);

    sample.action.decision_id = DecisionId("dec_fruit_001");
    sample.action.selected_candidate_id = ActionId("eat");
    sample.action.selected_command_action_id = ActionId("eat");
    sample.action.command_id = CommandId("cmd_eat_fruit_001");
    sample.action.decision_status = AgentDecisionRecordStatus::PipelineSucceeded;

    sample.result.runtime_status = AgentRuntimeStatus::PipelineSucceeded;
    sample.result.pipeline_status = PipelineStatus::Succeeded;
    sample.result.state_version_before = StateVersion(1);
    sample.result.state_version_after = StateVersion(2);
    sample.result.state_change_count = 1;
    sample.result.input_hash = HashValue(111);
    sample.result.output_hash = HashValue(222);
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;

    sample.trace.replay_locked = true;
    sample.trace.replay_lock_status = AgentReplayLockStatus::Locked;

    assert(sample.validateBasic().is_ok());
    training_samples.push_back(sample);
    std::cout << "  PASS: p10_training_sample_built\n";

    // Step 5: AgentHistoryQueryService query
    AgentHistoryQueryService query_service;
    AgentHistoryQueryOptions query_opts;
    query_opts.query_id = makeAgentHistoryQueryId(AgentId("agent_player_001"), std::nullopt, std::nullopt, 100);
    query_opts.agent_id = AgentId("agent_player_001");
    query_opts.sort_order = AgentHistorySortOrder::TickAscending;
    query_opts.projection_mode = AgentHistoryProjectionMode::Training;
    query_opts.limit = 100;

    auto query_result = query_service.query(tick_log, query_opts);
    assert(query_result.is_ok());
    assert(query_result.value().records.size() == 1);
    assert(query_result.value().total_matched == 1);
    std::cout << "  PASS: p10_query_succeeded\n";

    // Step 6: AgentHistoryProjector generate Training mode projection
    AgentHistoryProjectorInput proj_input;
    proj_input.query_result = query_result.value();
    proj_input.replay_lock_set = &lock_set;
    proj_input.training_samples = &training_samples;
    proj_input.projection_mode = AgentHistoryProjectionMode::Training;
    proj_input.include_debug_trace = true;

    AgentHistoryProjector projector;
    auto projection = projector.project(proj_input);
    assert(projection.is_ok());
    assert(projection.value().items.size() == 1);
    std::cout << "  PASS: p10_projection_generated\n";

    // Step 7: Assert command_id exists
    const auto& item = projection.value().items[0];
    assert(item.command_id.has_value());
    assert(item.command_id.value() == "cmd_eat_fruit_001");
    std::cout << "  PASS: p10_command_id_exists\n";

    // Step 8: Assert replay_lock_status=Locked
    assert(item.replay_lock_status.has_value());
    assert(item.replay_lock_status.value() == "Locked");
    std::cout << "  PASS: p10_replay_lock_status_locked\n";

    // Step 9: Assert training_sample_status=ReplayLocked
    assert(item.training_sample_status.has_value());
    assert(item.training_sample_status.value() == "ReplayLocked");
    std::cout << "  PASS: p10_training_sample_status_replay_locked\n";

    // Step 10: Generate ProtocolEnvelope and validate
    AgentProtocolProjectionAdapter adapter;
    auto envelope = adapter.buildHistoryEnvelope(projection.value());
    assert(envelope.validateBasic().is_ok());
    assert(envelope.message_type == ProtocolMessageType::Response);
    assert(envelope.domain == ProtocolDomain::Query);
    assert(envelope.payload.payload_type == ProtocolPayloadType::AgentHistoryProjection);
    std::cout << "  PASS: p10_envelope_valid\n";
}

// --- main ---

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "PASS: p10_agent_history_projection_flow smoke" << std::endl;
        return 0;
    }

    if (test_name == "p10_agent_history_projection_flow") {
        run_unknown_fruit_history_projection_flow();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
