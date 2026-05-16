#include "pathfinder/agent/runtime.h"
#include "pathfinder/agent/policy.h"
#include "pathfinder/agent/record_builder.h"
#include "pathfinder/agent/replay_bridge.h"
#include "pathfinder/agent/record_types.h"
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

void run_p8_unknown_fruit_replay() {
    // Step 1: Create initial state
    auto initial_state = UnknownFruitFixture::createInitialState();
    assert(initial_state.validateBasic().is_ok());

    // Step 2: Copy state for replay comparison
    auto initial_state_for_replay = initial_state;

    // Step 3: Execute P7 Runtime tick
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
    std::cout << "  PASS: p8_runtime_executed\n";

    // Step 4: Build AgentTickRecord
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
    std::cout << "  PASS: p8_tick_record_built\n";

    // Step 5: Append to AgentTickLog
    AgentTickLog tick_log;
    auto append_result = tick_log.append(tick_record);
    assert(append_result.is_ok());
    assert(tick_log.size() == 1);
    std::cout << "  PASS: p8_tick_log_appended\n";

    // Step 6: Convert to CommandReplayRecord via AgentReplayBridge
    AgentReplayBridge bridge;
    auto bridge_result = bridge.toCommandReplayRecord(tick_record);
    assert(bridge_result.is_ok());
    auto& replay_record = bridge_result.value();
    assert(replay_record.validateBasic().is_ok());
    std::cout << "  PASS: p8_replay_record_built\n";

    // Step 7: Append to CommandReplayLog
    CommandReplayLog command_log;
    auto cmd_append = bridge.appendCommandReplayRecord(tick_record, command_log);
    assert(cmd_append.is_ok());
    assert(command_log.size() == 1);
    std::cout << "  PASS: p8_command_log_appended\n";

    // Step 8: Replay using ReplayRunner (no AgentRuntime, no Policy)
    ReplayRunner runner;
    ReplayInput replay_input;
    replay_input.initial_state = initial_state_for_replay;
    replay_input.command_log = command_log;

    // Add random seed entry for the command
    RandomReplayLog random_log;
    RandomReplayEntry rand_entry;
    rand_entry.entry_id = RandomReplayEntryId("rand_entry_001");
    rand_entry.command_id = replay_record.command_id;
    rand_entry.seed = tick_record.random_seed;
    rand_entry.draw_index = 0;
    rand_entry.min_value = 0;
    rand_entry.max_value = 100;
    rand_entry.result_value = 0;
    rand_entry.reason = "test";
    random_log.append(rand_entry);
    replay_input.random_log = random_log;

    auto replay_result = runner.replayOne(replay_input);
    if (replay_result.is_error()) {
        std::cout << "  REPLAY ERROR: " << toString(replay_result.errors()[0]) << std::endl;
    }
    assert(replay_result.is_ok());
    if (replay_result.value().report.status != ReplayCompareStatus::Match) {
        std::cout << "  MISMATCH: expected_changes=" << replay_result.value().report.expected_state_change_count
                  << " actual=" << replay_result.value().report.actual_state_change_count << std::endl;
        std::cout << "  MISMATCH: expected_events=" << replay_result.value().report.expected_event_count
                  << " actual=" << replay_result.value().report.actual_event_count << std::endl;
        std::cout << "  MISMATCH: expected_status=" << toString(replay_result.value().report.expected_pipeline_status)
                  << " actual=" << toString(replay_result.value().report.actual_pipeline_status) << std::endl;
        for (const auto& diff : replay_result.value().report.differences) {
            std::cout << "  DIFF: " << diff << std::endl;
        }
    }
    assert(replay_result.value().report.status == ReplayCompareStatus::Match);
    std::cout << "  PASS: p8_replay_match\n";

    // Step 9: Verify replay lock check
    AgentReplayLockChecker checker;
    auto lock_result = checker.check(tick_record, command_log);
    assert(lock_result.has_agent_record);
    assert(lock_result.has_command_record);
    assert(lock_result.can_replay_without_policy);
    std::cout << "  PASS: p8_replay_locked\n";

    std::cout << "P8 unknown fruit record + replay tests passed!\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P8 smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "p8_unknown_fruit_replay") {
        run_p8_unknown_fruit_replay();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
