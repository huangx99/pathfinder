#include "pathfinder/save/save_package.h"
#include "pathfinder/replay/replay_runner.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::save;
using namespace pathfinder::foundation;
using namespace pathfinder::replay;
using namespace pathfinder::pipeline;

static SaveGamePackage buildReplayPackage(pathfinder::state::GameState& initial_out) {
    auto initial = pathfinder::rules::UnknownFruitFixture::createInitialState();
    initial.metadata.config_version = ConfigVersionId("cfg_p30_replay");
    initial.metadata.state_hash = HashValue::fromString("p30_replay_initial_state");
    auto cmd = pathfinder::rules::UnknownFruitFixture::createEatCommand();

    auto exec_state = initial;
    RulePipeline pipeline;
    PipelineContext ctx;
    ctx.command = cmd;
    ctx.state_metadata = exec_state.metadata;
    ctx.game_state = &exec_state;
    ctx.random_seed = RandomSeed(42);
    auto result = pipeline.execute(ctx);
    assert(result.is_ok());
    exec_state.metadata.config_version = ConfigVersionId("cfg_p30_replay");
    exec_state.metadata.state_hash = HashValue::fromString("p30_replay_after_state");

    CommandReplayRecord record;
    record.record_id = ReplayRecordId("rec_p30_replay_001");
    record.command_id = cmd.command_id;
    record.command = cmd;
    record.state_version_before = initial.metadata.state_version;
    record.state_version_after = exec_state.metadata.state_version;
    record.random_seed = RandomSeed(42);
    record.input_hash = HashValue::fromString("p30_replay_input");
    record.output_hash = HashValue::fromString("p30_replay_output");
    record.pipeline_status = result.value().status;
    record.error_count = result.value().errors.size();
    record.status = ReplayRecordStatus::Recorded;
    for (const auto& change : result.value().state_changes.changes()) record.state_change_ids.push_back(change.change_id);
    for (const auto& event : result.value().events.events()) record.event_ids.push_back(event.event_id);
    CommandReplayLog command_log;
    assert(command_log.append(record).is_ok());

    RandomReplayEntry random;
    random.entry_id = RandomReplayEntryId("rand_p30_replay_001");
    random.command_id = cmd.command_id;
    random.seed = RandomSeed(42);
    random.draw_index = 0;
    random.min_value = 0;
    random.max_value = 100;
    random.result_value = 0;
    random.reason = "p30_replay_seed";
    RandomReplayLog random_log;
    assert(random_log.append(random).is_ok());

    StateSnapshotRecord snapshot;
    snapshot.snapshot_id = SaveSnapshotId("snap_p30_replay_initial");
    snapshot.snapshot = initial;
    snapshot.snapshot_hash = HashValue::fromString("p30_replay_snapshot");
    snapshot.reason_keys = {"p30_replay_snapshot"};

    PipelineTraceRecord trace;
    trace.trace_id = SaveTraceId("trace_p30_replay_001");
    trace.command_id = cmd.command_id;
    trace.state_version = exec_state.metadata.state_version;
    trace.pipeline_stage_keys = {"accept_command", "resolve_rules", "apply_state_changes", "emit_events"};
    for (const auto& event : result.value().events.events()) trace.event_ids.push_back(event.event_id);
    trace.condition_trace_keys = {"condition:test:eq:p30_replay"};
    trace.reason_keys = {"p30_replay_trace"};

    SavePackageBuilderInput input;
    input.header.save_id = SavePackageId("save_p30_replay_001");
    input.header.game_build_version = "test_build_v1";
    input.header.game_id = GameId("game_pathfinder_test");
    input.header.player_id = PlayerId("player_test");
    input.header.config_version = ConfigVersionId("cfg_p30_replay");
    input.header.created_tick = Tick(20);
    input.header.base_state_version = initial.metadata.state_version;
    input.header.latest_state_version = exec_state.metadata.state_version;
    input.snapshot_section.snapshots = {snapshot};
    input.event_log_section.events = result.value().events.events();
    input.command_log_section.command_log = command_log;
    input.random_log_section.random_log = random_log;
    input.trace_section.traces = {trace};
    input.config_manifest.active_config_version = ConfigVersionId("cfg_p30_replay");
    input.config_manifest.required_config_versions = {ConfigVersionId("cfg_p30_replay")};
    input.config_manifest.config_content_hash = HashValue::fromString("p30_replay_config");
    input.config_manifest.compatibility_key = "p30_replay_compat";

    auto package = SavePackageBuilder{}.build(input);
    assert(package.is_ok());
    initial_out = initial;
    return package.value();
}

static void test_save_package_can_feed_replay() {
    pathfinder::state::GameState initial;
    auto package = buildReplayPackage(initial);
    assert(SavePackageValidator{}.validate(package).is_ok());

    auto plan = SaveRestorePlanner{}.plan(package, package.header.latest_state_version);
    assert(plan.is_ok());
    assert(plan.value().selected_snapshot_id.value() == "snap_p30_replay_initial");

    ReplayInput replay_input;
    replay_input.initial_state = package.snapshot_section.snapshots.front().snapshot;
    replay_input.command_log = package.command_log_section.command_log;
    replay_input.random_log = package.random_log_section.random_log;

    ReplayRunner runner;
    auto replay = runner.replayOne(replay_input);
    assert(replay.is_ok());
    assert(replay.value().report.status == ReplayCompareStatus::Match);
}

int main() {
    test_save_package_can_feed_replay();
    std::cout << "save_replay_flow_test passed\n";
    return 0;
}
