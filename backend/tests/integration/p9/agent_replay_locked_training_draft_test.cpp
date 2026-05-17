#include "pathfinder/agent/runtime.h"
#include "pathfinder/agent/policy.h"
#include "pathfinder/agent/record_builder.h"
#include "pathfinder/agent/replay_bridge.h"
#include "pathfinder/agent/record_types.h"
#include "pathfinder/agent/replay_lock.h"
#include "pathfinder/agent/training_sample.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/object/types.h"
#include "pathfinder/replay/replay_runner.h"
#include "pathfinder/replay/random_replay.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::state;
using namespace pathfinder::rules;
using namespace pathfinder::command;
using namespace pathfinder::object;
using namespace pathfinder::replay;

void run_p9_unknown_fruit_replay_locked_training_draft() {
    // Step 1: Create initial state
    auto initial_state = UnknownFruitFixture::createInitialState();
    assert(initial_state.validateBasic().is_ok());

    // Step 2: Execute P7 Runtime tick
    AgentRuntimeOptions opts;
    opts.include_cognition_claims = true;
    opts.include_explore_candidates = true;
    opts.submit_to_pipeline = true;
    opts.submit_action_allowlist.push_back(ActionId(std::string("eat")));

    AgentTickRequest req;
    req.binding.agent_id = AgentId(std::string("agent_player_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = &initial_state;
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));
    req.issued_tick = Tick(0);
    req.random_seed = RandomSeed(42);
    req.command_source = CommandSource::Ai;
    req.options = opts;

    FirstSupportedPolicy policy;
    AgentRuntime runtime(policy);
    auto tick_result = runtime.tickOne(req);
    assert(tick_result.is_ok());
    assert(tick_result.value().status == AgentRuntimeStatus::PipelineSucceeded);
    std::cout << "  PASS: p9_runtime_executed\n";

    // Step 3: Build AgentTickRecord
    AgentRecordBuildRequest build_req;
    build_req.tick_request = req;
    build_req.tick_result = tick_result.value();
    build_req.input_hash = HashValue::fromString("input_state");
    build_req.output_hash = HashValue::fromString("output_state");

    AgentRecordBuilder record_builder;
    auto record_result = record_builder.build(build_req);
    assert(record_result.is_ok());
    auto& tick_record = record_result.value();
    assert(tick_record.validateBasic().is_ok());
    std::cout << "  PASS: p9_tick_record_built\n";

    // Step 4: Append to AgentTickLog
    AgentTickLog tick_log;
    auto append_result = tick_log.append(tick_record);
    assert(append_result.is_ok());
    assert(tick_log.size() == 1);
    std::cout << "  PASS: p9_tick_log_appended\n";

    // Step 5: Build CommandReplayLog via AgentReplayBridge
    AgentReplayBridge bridge;
    CommandReplayLog command_log;
    auto cmd_append = bridge.appendCommandReplayRecord(tick_record, command_log);
    assert(cmd_append.is_ok());
    assert(command_log.size() == 1);
    std::cout << "  PASS: p9_command_log_built\n";

    // Step 6: Build ReplayLockSet
    AgentReplayLockBuildRequest lock_req;
    lock_req.agent_log = &tick_log;
    lock_req.command_log = &command_log;
    lock_req.source_hash = HashValue(777);

    AgentReplayLockSetBuilder lock_builder;
    auto lock_result = lock_builder.build(lock_req);
    if (lock_result.is_error()) {
        std::cout << "  LOCK ERROR: " << toString(lock_result.errors()[0]) << std::endl;
    }
    assert(lock_result.is_ok());
    auto& lock_set = lock_result.value();
    assert(lock_set.validateBasic().is_ok());
    assert(lock_set.allReplayableWithoutPolicy());
    assert(!lock_set.hasBrokenEntry());
    assert(lock_set.entries[0].status == AgentReplayLockStatus::Locked);
    std::cout << "  PASS: p9_replay_lock_set_built\n";

    // Step 7: Build TrainingSampleDraft
    AgentTrainingSampleBuildRequest sample_req;
    sample_req.record = tick_record;
    sample_req.replay_lock_entry = lock_set.entries[0];
    sample_req.observation_hash = HashValue(555);

    AgentTrainingSampleDraftBuilder sample_builder;
    auto sample_result = sample_builder.build(sample_req);
    if (sample_result.is_error()) {
        std::cout << "  SAMPLE ERROR: " << toString(sample_result.errors()[0]) << std::endl;
    }
    assert(sample_result.is_ok());
    auto& sample = sample_result.value();
    assert(sample.validateBasic().is_ok());
    std::cout << "  PASS: p9_training_sample_built\n";

    // Step 8: Verify sample properties
    assert(sample.status == AgentTrainingSampleStatus::ReplayLocked);
    assert(sample.result.reward_status == AgentRewardDraftStatus::NotComputed);
    assert(sample.result.done_status == AgentDoneDraftStatus::NotComputed);
    assert(sample.trace.replay_locked);
    assert(sample.trace.replay_lock_status == AgentReplayLockStatus::Locked);
    assert(sample.observation.agent_id == AgentId("agent_player_001"));
    assert(sample.observation.observation_hash == HashValue(555));
    assert(sample.action.command_id.has_value());
    assert(sample.result.runtime_status == AgentRuntimeStatus::PipelineSucceeded);
    std::cout << "  PASS: p9_sample_properties_verified\n";

    // Step 9: Optional - verify replay still matches (P4 path, not P9 path)
    auto initial_state_for_replay = UnknownFruitFixture::createInitialState();
    ReplayRunner runner;
    ReplayInput replay_input;
    replay_input.initial_state = initial_state_for_replay;
    replay_input.command_log = command_log;

    RandomReplayLog random_log;
    RandomReplayEntry rand_entry;
    rand_entry.entry_id = RandomReplayEntryId("rand_entry_001");
    rand_entry.command_id = tick_record.decision_record.command_id.value();
    rand_entry.seed = tick_record.random_seed;
    rand_entry.draw_index = 0;
    rand_entry.min_value = 0;
    rand_entry.max_value = 100;
    rand_entry.result_value = 0;
    rand_entry.reason = "test";
    random_log.append(rand_entry);
    replay_input.random_log = random_log;

    auto replay_result = runner.replayOne(replay_input);
    assert(replay_result.is_ok());
    assert(replay_result.value().report.status == ReplayCompareStatus::Match);
    std::cout << "  PASS: p9_replay_still_matches\n";

    std::cout << "P9 unknown fruit replay-locked training draft test passed!\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P9 replay locked training draft smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "p9_replay_locked_training_draft") {
        run_p9_unknown_fruit_replay_locked_training_draft();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
