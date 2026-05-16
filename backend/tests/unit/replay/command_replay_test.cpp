#include "pathfinder/replay/types.h"
#include "pathfinder/replay/command_replay.h"
#include "pathfinder/replay/random_replay.h"
#include "pathfinder/replay/replay_runner.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::replay;
using namespace pathfinder::foundation;
using namespace pathfinder::command;
using namespace pathfinder::pipeline;

static CommandReplayRecord makeValidRecord() {
    auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();
    CommandReplayRecord record;
    record.record_id = ReplayRecordId("rec_001");
    record.command_id = cmd.command_id;
    record.command = cmd;
    record.state_version_before = StateVersion(1);
    record.state_version_after = StateVersion(2);
    record.random_seed = RandomSeed(42);
    record.input_hash = HashValue::fromString("input");
    record.output_hash = HashValue::fromString("output");
    record.pipeline_status = PipelineStatus::Succeeded;
    record.error_count = 0;
    record.status = ReplayRecordStatus::Recorded;
    return record;
}

static RandomReplayEntry makeValidRandomEntry() {
    RandomReplayEntry entry;
    entry.entry_id = RandomReplayEntryId("rand_001");
    entry.command_id = CommandId("cmd_001");
    entry.seed = RandomSeed(42);
    entry.draw_index = 0;
    entry.min_value = 1;
    entry.max_value = 100;
    entry.result_value = 50;
    entry.reason = "eat_unknown_fruit";
    return entry;
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    // --- Smoke ---
    if (test_name == "smoke") {
        assert(replay_module_version() == 1);
        std::cout << "PASS: replay smoke" << std::endl;
        return 0;
    }

    // --- CommandReplayRecord tests ---
    if (test_name == "valid_record_passes") {
        auto record = makeValidRecord();
        auto result = record.validateBasic();
        assert(result.is_ok());
        std::cout << "PASS: valid record passes" << std::endl;
        return 0;
    }

    if (test_name == "missing_record_id_fails") {
        auto record = makeValidRecord();
        record.record_id = ReplayRecordId();
        auto result = record.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: missing record_id fails" << std::endl;
        return 0;
    }

    if (test_name == "missing_command_id_fails") {
        auto record = makeValidRecord();
        record.command_id = CommandId();
        auto result = record.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: missing command_id fails" << std::endl;
        return 0;
    }

    if (test_name == "unknown_status_fails") {
        auto record = makeValidRecord();
        record.status = ReplayRecordStatus::Unknown;
        auto result = record.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: unknown status fails" << std::endl;
        return 0;
    }

    if (test_name == "invalid_record_id_format_fails") {
        auto record = makeValidRecord();
        record.record_id = ReplayRecordId("rec with spaces");
        auto result = record.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: invalid record_id format fails" << std::endl;
        return 0;
    }

    if (test_name == "invalid_command_id_format_fails") {
        auto record = makeValidRecord();
        record.command_id = CommandId("cmd with spaces");
        auto result = record.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: invalid command_id format fails" << std::endl;
        return 0;
    }

    if (test_name == "command_id_mismatch_fails") {
        auto record = makeValidRecord();
        record.command_id = CommandId("cmd_different_id");
        auto result = record.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: command_id mismatch fails" << std::endl;
        return 0;
    }

    if (test_name == "empty_input_hash_fails") {
        auto record = makeValidRecord();
        record.input_hash = HashValue();
        auto result = record.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: empty input_hash fails" << std::endl;
        return 0;
    }

    if (test_name == "empty_output_hash_fails") {
        auto record = makeValidRecord();
        record.output_hash = HashValue();
        auto result = record.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: empty output_hash fails" << std::endl;
        return 0;
    }

    if (test_name == "append_invalid_record_fails") {
        CommandReplayLog log;
        auto record = makeValidRecord();
        record.record_id = ReplayRecordId();  // empty id -> invalid
        auto result = log.append(record);
        assert(result.is_error());
        assert(log.size() == 0);
        std::cout << "PASS: append invalid record fails" << std::endl;
        return 0;
    }

    if (test_name == "duplicate_record_id_rejected") {
        CommandReplayLog log;
        auto record1 = makeValidRecord();
        auto result1 = log.append(record1);
        assert(result1.is_ok());

        auto record2 = makeValidRecord();
        auto result2 = log.append(record2);
        assert(result2.is_error());
        std::cout << "PASS: duplicate record_id rejected" << std::endl;
        return 0;
    }

    if (test_name == "find_by_command_id") {
        CommandReplayLog log;
        auto record = makeValidRecord();
        log.append(record);

        auto found = log.findByCommandId(record.command_id);
        assert(found.has_value());
        assert(found->record_id == record.record_id);

        auto not_found = log.findByCommandId(CommandId("nonexistent"));
        assert(!not_found.has_value());
        std::cout << "PASS: find by command_id works" << std::endl;
        return 0;
    }

    if (test_name == "empty_log_valid") {
        CommandReplayLog log;
        assert(log.size() == 0);
        auto result = log.validateBasic();
        assert(result.is_ok());
        std::cout << "PASS: empty log is valid" << std::endl;
        return 0;
    }

    if (test_name == "status_enum_roundtrip") {
        auto r1 = replayRecordStatusFromString("recorded");
        assert(r1.is_ok());
        assert(r1.value() == ReplayRecordStatus::Recorded);
        assert(toString(ReplayRecordStatus::Replayed) == "replayed");

        auto r2 = replayRecordStatusFromString("invalid");
        assert(r2.is_error());
        std::cout << "PASS: status enum roundtrip" << std::endl;
        return 0;
    }

    // --- RandomReplayEntry/Log tests ---
    if (test_name == "valid_entry_passes") {
        auto entry = makeValidRandomEntry();
        auto result = entry.validateBasic();
        assert(result.is_ok());
        std::cout << "PASS: valid entry passes" << std::endl;
        return 0;
    }

    if (test_name == "missing_entry_id_fails") {
        auto entry = makeValidRandomEntry();
        entry.entry_id = RandomReplayEntryId();
        auto result = entry.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: missing entry_id fails" << std::endl;
        return 0;
    }

    if (test_name == "min_greater_than_max_fails") {
        auto entry = makeValidRandomEntry();
        entry.min_value = 100;
        entry.max_value = 10;
        auto result = entry.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: min > max fails" << std::endl;
        return 0;
    }

    if (test_name == "result_out_of_range_fails") {
        auto entry = makeValidRandomEntry();
        entry.result_value = 200;
        auto result = entry.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: result out of range fails" << std::endl;
        return 0;
    }

    if (test_name == "negative_draw_index_fails") {
        auto entry = makeValidRandomEntry();
        entry.draw_index = -1;
        auto result = entry.validateBasic();
        assert(result.is_error());
        std::cout << "PASS: negative draw_index fails" << std::endl;
        return 0;
    }

    if (test_name == "append_invalid_entry_fails") {
        RandomReplayLog log;
        auto entry = makeValidRandomEntry();
        entry.entry_id = RandomReplayEntryId();  // empty id -> invalid
        auto result = log.append(entry);
        assert(result.is_error());
        assert(log.size() == 0);
        std::cout << "PASS: append invalid entry fails" << std::endl;
        return 0;
    }

    if (test_name == "append_and_find") {
        RandomReplayLog log;
        auto entry = makeValidRandomEntry();
        auto result = log.append(entry);
        assert(result.is_ok());
        assert(log.size() == 1);

        auto found = log.findByCommandId(CommandId("cmd_001"));
        assert(found.size() == 1);
        assert(found[0].entry_id == RandomReplayEntryId("rand_001"));

        auto not_found = log.findByCommandId(CommandId("cmd_999"));
        assert(not_found.empty());
        std::cout << "PASS: append and find works" << std::endl;
        return 0;
    }

    if (test_name == "duplicate_entry_rejected") {
        RandomReplayLog log;
        auto entry = makeValidRandomEntry();
        log.append(entry);
        auto result = log.append(entry);
        assert(result.is_error());
        std::cout << "PASS: duplicate entry rejected" << std::endl;
        return 0;
    }

    if (test_name == "stable_seed_comparison") {
        RandomReplayLog log;
        auto entry = makeValidRandomEntry();
        log.append(entry);

        auto entries = log.findByCommandId(CommandId("cmd_001"));
        assert(entries.size() == 1);
        assert(entries[0].seed == RandomSeed(42));
        assert(entries[0].draw_index == 0);
        assert(entries[0].result_value == 50);
        std::cout << "PASS: stable seed comparison" << std::endl;
        return 0;
    }

    // --- ReplayRunner tests ---
    if (test_name == "p3_eat_replay_matches") {
        // Create initial state and command from P3 fixture
        auto initial_state = pathfinder::rules::UnknownFruitFixture::createInitialState();
        auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();

        // Execute once to get the expected result
        pathfinder::pipeline::RulePipeline pipeline;
        pathfinder::state::GameState exec_state = initial_state;
        pathfinder::pipeline::PipelineContext exec_ctx;
        exec_ctx.command = cmd;
        exec_ctx.state_metadata = exec_state.metadata;
        exec_ctx.game_state = &exec_state;
        exec_ctx.random_seed = RandomSeed(42);
        auto first_result = pipeline.execute(exec_ctx);
        assert(first_result.is_ok());
        assert(first_result.value().status == PipelineStatus::Succeeded);

        // Build command replay record with actual results
        CommandReplayRecord record;
        record.record_id = ReplayRecordId("rec_test_001");
        record.command_id = cmd.command_id;
        record.command = cmd;
        record.state_version_before = initial_state.metadata.state_version;
        record.state_version_after = exec_state.metadata.state_version;
        record.random_seed = RandomSeed(42);
        record.input_hash = HashValue::fromString("input");
        record.output_hash = HashValue::fromString("output");
        record.pipeline_status = first_result.value().status;
        record.error_count = first_result.value().errors.size();
        record.status = ReplayRecordStatus::Recorded;
        for (const auto& sc : first_result.value().state_changes.changes()) {
            record.state_change_ids.push_back(sc.change_id);
        }
        for (const auto& ev : first_result.value().events.events()) {
            record.event_ids.push_back(ev.event_id);
        }

        // Build random log with seed entry
        RandomReplayLog rand_log;
        RandomReplayEntry rand_entry;
        rand_entry.entry_id = RandomReplayEntryId("rand_test_001");
        rand_entry.command_id = cmd.command_id;
        rand_entry.seed = RandomSeed(42);
        rand_entry.draw_index = 0;
        rand_entry.min_value = 0;
        rand_entry.max_value = 100;
        rand_entry.result_value = 0;
        rand_entry.reason = "eat_seed";
        rand_log.append(rand_entry);

        // Build replay input
        ReplayInput input;
        input.initial_state = initial_state;
        input.command_log.append(record);
        input.random_log = rand_log;

        // Replay
        ReplayRunner runner;
        auto replay_result = runner.replayOne(input);
        assert(replay_result.is_ok());
        assert(replay_result.value().report.status == ReplayCompareStatus::Match);
        std::cout << "PASS: p3 eat replay matches" << std::endl;
        return 0;
    }

    if (test_name == "same_input_gets_match") {
        auto initial_state = pathfinder::rules::UnknownFruitFixture::createInitialState();
        auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();

        // Execute once
        pathfinder::pipeline::RulePipeline pipeline;
        pathfinder::state::GameState state1 = initial_state;
        pathfinder::pipeline::PipelineContext ctx1;
        ctx1.command = cmd;
        ctx1.state_metadata = state1.metadata;
        ctx1.game_state = &state1;
        ctx1.random_seed = RandomSeed(42);
        auto result1 = pipeline.execute(ctx1);
        assert(result1.is_ok());

        // Build record
        CommandReplayRecord record;
        record.record_id = ReplayRecordId("rec_test_002");
        record.command_id = cmd.command_id;
        record.command = cmd;
        record.state_version_before = initial_state.metadata.state_version;
        record.state_version_after = state1.metadata.state_version;
        record.random_seed = RandomSeed(42);
        record.input_hash = HashValue::fromString("input");
        record.output_hash = HashValue::fromString("output");
        record.pipeline_status = result1.value().status;
        record.error_count = result1.value().errors.size();
        record.status = ReplayRecordStatus::Recorded;
        for (const auto& sc : result1.value().state_changes.changes()) {
            record.state_change_ids.push_back(sc.change_id);
        }
        for (const auto& ev : result1.value().events.events()) {
            record.event_ids.push_back(ev.event_id);
        }

        // Build random log with seed entry
        RandomReplayLog rand_log;
        RandomReplayEntry rand_entry;
        rand_entry.entry_id = RandomReplayEntryId("rand_test_002");
        rand_entry.command_id = cmd.command_id;
        rand_entry.seed = RandomSeed(42);
        rand_entry.draw_index = 0;
        rand_entry.min_value = 0;
        rand_entry.max_value = 100;
        rand_entry.result_value = 0;
        rand_entry.reason = "eat_seed";
        rand_log.append(rand_entry);

        ReplayInput input;
        input.initial_state = initial_state;
        input.command_log.append(record);
        input.random_log = rand_log;

        ReplayRunner runner;
        auto replay_result = runner.replayOne(input);
        assert(replay_result.is_ok());
        assert(replay_result.value().report.status == ReplayCompareStatus::Match);
        std::cout << "PASS: same input gets match" << std::endl;
        return 0;
    }

    if (test_name == "tampered_output_mismatch") {
        auto initial_state = pathfinder::rules::UnknownFruitFixture::createInitialState();
        auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();

        pathfinder::pipeline::RulePipeline pipeline;
        pathfinder::state::GameState exec_state = initial_state;
        pathfinder::pipeline::PipelineContext exec_ctx;
        exec_ctx.command = cmd;
        exec_ctx.state_metadata = exec_state.metadata;
        exec_ctx.game_state = &exec_state;
        exec_ctx.random_seed = RandomSeed(42);
        auto first_result = pipeline.execute(exec_ctx);
        assert(first_result.is_ok());

        // Build record with tampered event_count
        CommandReplayRecord record;
        record.record_id = ReplayRecordId("rec_test_003");
        record.command_id = cmd.command_id;
        record.command = cmd;
        record.state_version_before = initial_state.metadata.state_version;
        record.state_version_after = exec_state.metadata.state_version;
        record.random_seed = RandomSeed(42);
        record.input_hash = HashValue::fromString("input");
        record.output_hash = HashValue::fromString("tampered_output");
        record.pipeline_status = first_result.value().status;
        record.error_count = 0;
        record.status = ReplayRecordStatus::Recorded;
        for (const auto& sc : first_result.value().state_changes.changes()) {
            record.state_change_ids.push_back(sc.change_id);
        }
        // Add extra fake events to cause event_count mismatch
        record.event_ids.push_back(EventId("fake_event_1"));
        record.event_ids.push_back(EventId("fake_event_2"));
        record.event_ids.push_back(EventId("fake_event_3"));
        record.event_ids.push_back(EventId("fake_event_4"));
        record.event_ids.push_back(EventId("fake_event_5"));

        // Build random log with seed entry
        RandomReplayLog rand_log;
        RandomReplayEntry rand_entry;
        rand_entry.entry_id = RandomReplayEntryId("rand_test_003");
        rand_entry.command_id = cmd.command_id;
        rand_entry.seed = RandomSeed(42);
        rand_entry.draw_index = 0;
        rand_entry.min_value = 0;
        rand_entry.max_value = 100;
        rand_entry.result_value = 0;
        rand_entry.reason = "eat_seed";
        rand_log.append(rand_entry);

        ReplayInput input;
        input.initial_state = initial_state;
        input.command_log.append(record);
        input.random_log = rand_log;

        ReplayRunner runner;
        auto replay_result = runner.replayOne(input);
        assert(replay_result.is_ok());
        assert(replay_result.value().report.status == ReplayCompareStatus::Mismatch);
        assert(!replay_result.value().report.differences.empty());
        std::cout << "PASS: tampered output causes mismatch" << std::endl;
        return 0;
    }

    if (test_name == "random_log_missing_seed_fails") {
        auto initial_state = pathfinder::rules::UnknownFruitFixture::createInitialState();
        auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();

        // Build a valid record
        CommandReplayRecord record;
        record.record_id = ReplayRecordId("rec_test_004");
        record.command_id = cmd.command_id;
        record.command = cmd;
        record.state_version_before = initial_state.metadata.state_version;
        record.state_version_after = StateVersion(2);
        record.random_seed = RandomSeed(42);
        record.input_hash = HashValue::fromString("input");
        record.output_hash = HashValue::fromString("output");
        record.pipeline_status = PipelineStatus::Succeeded;
        record.error_count = 0;
        record.status = ReplayRecordStatus::Recorded;

        // Empty random log - no seed entry
        ReplayInput input;
        input.initial_state = initial_state;
        input.command_log.append(record);

        ReplayRunner runner;
        auto replay_result = runner.replayOne(input);
        assert(replay_result.is_error());
        std::cout << "PASS: random log missing seed fails" << std::endl;
        return 0;
    }

    if (test_name == "random_seed_mismatch_fails") {
        auto initial_state = pathfinder::rules::UnknownFruitFixture::createInitialState();
        auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();

        CommandReplayRecord record;
        record.record_id = ReplayRecordId("rec_test_005");
        record.command_id = cmd.command_id;
        record.command = cmd;
        record.state_version_before = initial_state.metadata.state_version;
        record.state_version_after = StateVersion(2);
        record.random_seed = RandomSeed(42);
        record.input_hash = HashValue::fromString("input");
        record.output_hash = HashValue::fromString("output");
        record.pipeline_status = PipelineStatus::Succeeded;
        record.error_count = 0;
        record.status = ReplayRecordStatus::Recorded;

        // Random log with different seed
        RandomReplayLog rand_log;
        RandomReplayEntry rand_entry;
        rand_entry.entry_id = RandomReplayEntryId("rand_test_005");
        rand_entry.command_id = cmd.command_id;
        rand_entry.seed = RandomSeed(999);  // Different seed!
        rand_entry.draw_index = 0;
        rand_entry.min_value = 0;
        rand_entry.max_value = 100;
        rand_entry.result_value = 0;
        rand_entry.reason = "eat_seed";
        rand_log.append(rand_entry);

        ReplayInput input;
        input.initial_state = initial_state;
        input.command_log.append(record);
        input.random_log = rand_log;

        ReplayRunner runner;
        auto replay_result = runner.replayOne(input);
        assert(replay_result.is_error());
        std::cout << "PASS: random seed mismatch fails" << std::endl;
        return 0;
    }

    if (test_name == "empty_log_returns_error") {
        ReplayInput input;
        input.initial_state = pathfinder::rules::UnknownFruitFixture::createInitialState();

        ReplayRunner runner;
        auto replay_result = runner.replayOne(input);
        assert(replay_result.is_error());
        std::cout << "PASS: empty log returns error" << std::endl;
        return 0;
    }

    return 1;
}
