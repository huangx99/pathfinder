#include "pathfinder/save/save_package.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::save;
using namespace pathfinder::foundation;
using namespace pathfinder::replay;
using namespace pathfinder::pipeline;

struct PackageFixture {
    SaveGamePackage package;
    pathfinder::state::GameState initial_state;
};

static PipelineTraceRecord traceFrom(const pathfinder::command::CommandEnvelope& cmd, const pathfinder::pipeline::PipelineResult& result, StateVersion version) {
    PipelineTraceRecord trace;
    trace.trace_id = SaveTraceId("trace_p30_001");
    trace.command_id = cmd.command_id;
    trace.state_version = version;
    for (const auto& step : defaultPipelineSteps()) trace.pipeline_stage_keys.push_back(toString(step.stage));
    for (const auto& event : result.events.events()) trace.event_ids.push_back(event.event_id);
    trace.condition_trace_keys = {"condition:test:eq:p30_replay"};
    trace.reason_keys = {"p30_pipeline_trace"};
    return trace;
}

static PackageFixture makePackage() {
    auto initial = pathfinder::rules::UnknownFruitFixture::createInitialState();
    initial.metadata.config_version = ConfigVersionId("cfg_test_v1");
    initial.metadata.state_hash = HashValue::fromString("initial_state_hash");
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
    exec_state.metadata.config_version = ConfigVersionId("cfg_test_v1");
    exec_state.metadata.state_hash = HashValue::fromString("after_state_hash");

    CommandReplayRecord record;
    record.record_id = ReplayRecordId("rec_p30_001");
    record.command_id = cmd.command_id;
    record.command = cmd;
    record.state_version_before = initial.metadata.state_version;
    record.state_version_after = exec_state.metadata.state_version;
    record.random_seed = RandomSeed(42);
    record.input_hash = HashValue::fromString("p30_input");
    record.output_hash = HashValue::fromString("p30_output");
    record.pipeline_status = result.value().status;
    record.error_count = result.value().errors.size();
    record.status = ReplayRecordStatus::Recorded;
    for (const auto& change : result.value().state_changes.changes()) record.state_change_ids.push_back(change.change_id);
    for (const auto& event : result.value().events.events()) record.event_ids.push_back(event.event_id);
    CommandReplayLog command_log;
    assert(command_log.append(record).is_ok());

    RandomReplayEntry random;
    random.entry_id = RandomReplayEntryId("rand_p30_001");
    random.command_id = cmd.command_id;
    random.seed = RandomSeed(42);
    random.draw_index = 0;
    random.min_value = 0;
    random.max_value = 100;
    random.result_value = 0;
    random.reason = "p30_seed";
    RandomReplayLog random_log;
    assert(random_log.append(random).is_ok());

    StateSnapshotRecord snapshot;
    snapshot.snapshot_id = SaveSnapshotId("snap_p30_001");
    snapshot.snapshot = initial;
    snapshot.snapshot_hash = HashValue::fromString("snapshot_initial");
    snapshot.reason_keys = {"p30_initial_snapshot"};

    SavePackageBuilderInput input;
    input.header.save_id = SavePackageId("save_p30_001");
    input.header.game_build_version = "test_build_v1";
    input.header.game_id = GameId("game_pathfinder_test");
    input.header.player_id = PlayerId("player_test");
    input.header.config_version = ConfigVersionId("cfg_test_v1");
    input.header.created_tick = Tick(10);
    input.header.base_state_version = initial.metadata.state_version;
    input.header.latest_state_version = exec_state.metadata.state_version;
    input.snapshot_section.snapshots = {snapshot};
    input.event_log_section.events = result.value().events.events();
    input.command_log_section.command_log = command_log;
    input.random_log_section.random_log = random_log;
    input.trace_section.traces = {traceFrom(cmd, result.value(), exec_state.metadata.state_version)};
    input.config_manifest.active_config_version = ConfigVersionId("cfg_test_v1");
    input.config_manifest.required_config_versions = {ConfigVersionId("cfg_test_v1")};
    input.config_manifest.config_content_hash = HashValue::fromString("config_content_v1");
    input.config_manifest.compatibility_key = "p30_test_compat";

    auto package = SavePackageBuilder{}.build(input);
    assert(package.is_ok());
    return {package.value(), initial};
}

static void test_enum_roundtrip() {
    assert(saveSectionKindFromString(toString(SaveSectionKind::StateSnapshot)).value() == SaveSectionKind::StateSnapshot);
    assert(restorePlanStepKindFromString(toString(RestorePlanStepKind::LoadSnapshot)).value() == RestorePlanStepKind::LoadSnapshot);
}

static void test_forbidden_keys() {
    assert(containsSaveForbiddenKey("hidden_truth_value"));
    assert(containsSaveForbiddenKey("true_hp_snapshot"));
    assert(!containsSaveForbiddenKey("state_snapshot"));
}

static void test_builder_and_validator() {
    auto fixture = makePackage();
    assert(SavePackageValidator{}.validate(fixture.package).is_ok());
    assert(fixture.package.index.find(SaveSectionKind::StateSnapshot) != nullptr);
    assert(fixture.package.index.find(SaveSectionKind::IntegrityManifest) != nullptr);
    assert(!fixture.package.header.package_hash.empty());
}

static void test_tampered_hash_fails() {
    auto fixture = makePackage();
    fixture.package.header.package_hash = HashValue::fromString("tampered");
    assert(SavePackageValidator{}.validate(fixture.package).is_error());
}

static void test_missing_snapshot_fails() {
    auto fixture = makePackage();
    fixture.package.snapshot_section.snapshots.clear();
    assert(SavePackageValidator{}.validate(fixture.package).is_error());
}

static void test_version_range_fails() {
    auto fixture = makePackage();
    auto input_package = fixture.package;
    input_package.header.latest_state_version = StateVersion(0);
    assert(SavePackageValidator{}.validate(input_package).is_error());
}

static void test_config_manifest_fails() {
    auto fixture = makePackage();
    fixture.package.config_manifest.config_content_hash = HashValue();
    assert(SavePackageValidator{}.validate(fixture.package).is_error());
}

static void test_restore_plan() {
    auto fixture = makePackage();
    auto plan = SaveRestorePlanner{}.plan(fixture.package, fixture.package.header.latest_state_version);
    assert(plan.is_ok());
    assert(plan.value().selected_snapshot_id.value() == "snap_p30_001");
    assert(plan.value().steps.size() == 7);
    assert(plan.value().steps.front().kind == RestorePlanStepKind::VerifyIntegrity);
    assert(plan.value().steps.back().kind == RestorePlanStepKind::Ready);
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "enum" || mode == "all") test_enum_roundtrip();
    if (mode == "forbidden" || mode == "all") test_forbidden_keys();
    if (mode == "builder" || mode == "all") test_builder_and_validator();
    if (mode == "tamper" || mode == "all") test_tampered_hash_fails();
    if (mode == "missing_snapshot" || mode == "all") test_missing_snapshot_fails();
    if (mode == "version" || mode == "all") test_version_range_fails();
    if (mode == "config" || mode == "all") test_config_manifest_fails();
    if (mode == "restore" || mode == "all") test_restore_plan();
    std::cout << "save_package_test passed\n";
    return 0;
}
