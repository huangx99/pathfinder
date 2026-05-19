#include "pathfinder/replay/replay_diagnostics.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::foundation;
using namespace pathfinder::pipeline;
using namespace pathfinder::replay;
using namespace pathfinder::save;

static bool hasIssue(const GoldenReplayReport& report, ReplayDiagnosticIssueKind kind) {
    for (const auto& issue : report.issues) {
        if (issue.kind == kind) return true;
    }
    return false;
}

static PipelineTraceRecord traceFrom(
    const pathfinder::command::CommandEnvelope& command,
    const pathfinder::pipeline::PipelineResult& result,
    StateVersion state_version,
    std::vector<std::string> condition_trace_keys = {"condition:test:eq:p31_replay"}) {
    PipelineTraceRecord trace;
    trace.trace_id = SaveTraceId("trace_p31_001");
    trace.command_id = command.command_id;
    trace.state_version = state_version;
    for (const auto& step : defaultPipelineSteps()) trace.pipeline_stage_keys.push_back(toString(step.stage));
    for (const auto& event : result.events.events()) trace.event_ids.push_back(event.event_id);
    trace.condition_trace_keys = std::move(condition_trace_keys);
    trace.reason_keys = {"p31_pipeline_trace"};
    return trace;
}

static SaveGamePackage makePackage(bool omit_event_ids = false) {
    auto initial = pathfinder::rules::UnknownFruitFixture::createInitialState();
    initial.metadata.config_version = ConfigVersionId("cfg_p31_replay");
    initial.metadata.state_hash = HashValue::fromString("p31_initial_state");
    auto command = pathfinder::rules::UnknownFruitFixture::createEatCommand();

    auto exec_state = initial;
    RulePipeline pipeline;
    PipelineContext context;
    context.command = command;
    context.state_metadata = exec_state.metadata;
    context.game_state = &exec_state;
    context.random_seed = RandomSeed(42);
    auto result = pipeline.execute(context);
    assert(result.is_ok());
    exec_state.metadata.config_version = ConfigVersionId("cfg_p31_replay");
    exec_state.metadata.state_hash = HashValue::fromString("p31_after_state");

    CommandReplayRecord record;
    record.record_id = ReplayRecordId("rec_p31_001");
    record.command_id = command.command_id;
    record.command = command;
    record.state_version_before = initial.metadata.state_version;
    record.state_version_after = exec_state.metadata.state_version;
    record.random_seed = RandomSeed(42);
    record.input_hash = HashValue::fromString("p31_input");
    record.output_hash = HashValue::fromString("p31_output");
    record.pipeline_status = result.value().status;
    record.error_count = result.value().errors.size();
    record.status = ReplayRecordStatus::Recorded;
    for (const auto& change : result.value().state_changes.changes()) record.state_change_ids.push_back(change.change_id);
    if (!omit_event_ids) {
        for (const auto& event : result.value().events.events()) record.event_ids.push_back(event.event_id);
    }
    CommandReplayLog command_log;
    assert(command_log.append(record).is_ok());

    RandomReplayEntry random;
    random.entry_id = RandomReplayEntryId("rand_p31_001");
    random.command_id = command.command_id;
    random.seed = RandomSeed(42);
    random.draw_index = 0;
    random.min_value = 0;
    random.max_value = 100;
    random.result_value = 0;
    random.reason = "p31_seed";
    RandomReplayLog random_log;
    assert(random_log.append(random).is_ok());

    StateSnapshotRecord snapshot;
    snapshot.snapshot_id = SaveSnapshotId("snap_p31_initial");
    snapshot.snapshot = initial;
    snapshot.snapshot_hash = HashValue::fromString("p31_snapshot_initial");
    snapshot.reason_keys = {"p31_initial_snapshot"};

    SavePackageBuilderInput input;
    input.header.save_id = SavePackageId("save_p31_001");
    input.header.game_build_version = "test_build_v1";
    input.header.game_id = GameId("game_pathfinder_test");
    input.header.player_id = PlayerId("player_test");
    input.header.config_version = ConfigVersionId("cfg_p31_replay");
    input.header.created_tick = Tick(31);
    input.header.base_state_version = initial.metadata.state_version;
    input.header.latest_state_version = exec_state.metadata.state_version;
    input.snapshot_section.snapshots = {snapshot};
    input.event_log_section.events = result.value().events.events();
    input.command_log_section.command_log = command_log;
    input.random_log_section.random_log = random_log;
    input.trace_section.traces = {traceFrom(command, result.value(), exec_state.metadata.state_version)};
    input.config_manifest.active_config_version = ConfigVersionId("cfg_p31_replay");
    input.config_manifest.required_config_versions = {ConfigVersionId("cfg_p31_replay")};
    input.config_manifest.config_content_hash = HashValue::fromString("p31_config_content");
    input.config_manifest.compatibility_key = "p31_replay_compat";

    auto package = SavePackageBuilder{}.build(input);
    assert(package.is_ok());
    return package.value();
}

static void test_enum_roundtrip() {
    assert(goldenReplayStatusFromString(toString(GoldenReplayStatus::Passed)).value() == GoldenReplayStatus::Passed);
    assert(replayDiagnosticSeverityFromString(toString(ReplayDiagnosticSeverity::Critical)).value() == ReplayDiagnosticSeverity::Critical);
    assert(replayDiagnosticIssueKindFromString(toString(ReplayDiagnosticIssueKind::BaselineMismatch)).value() == ReplayDiagnosticIssueKind::BaselineMismatch);
}

static void test_golden_replay_passes() {
    auto package = makePackage();
    auto baseline = GoldenReplayBaselineBuilder{}.build(package, package.header.latest_state_version);
    assert(baseline.is_ok());
    assert(baseline.value().expected_condition_trace_keys.size() == 1);

    GoldenReplayRequest request;
    request.package = package;
    request.target_state_version = package.header.latest_state_version;
    request.baseline = baseline.value();
    request.expected_condition_trace_keys = {"condition:test:eq:p31_replay"};
    request.allow_unexpected_condition_traces = false;

    auto report = GoldenReplayRunner{}.run(request);
    assert(report.is_ok());
    assert(report.value().passed());
    assert(report.value().compare_report.status == ReplayCompareStatus::Match);
    assert(report.value().observed_condition_trace_keys.size() == 1);
}

static void test_package_integrity_diagnostic() {
    auto package = makePackage();
    package.header.package_hash = HashValue::fromString("p31_tampered_package_hash");

    GoldenReplayRequest request;
    request.package = package;
    request.target_state_version = package.header.latest_state_version;

    auto report = GoldenReplayRunner{}.run(request);
    assert(report.is_ok());
    assert(report.value().status == GoldenReplayStatus::Failed);
    assert(hasIssue(report.value(), ReplayDiagnosticIssueKind::PackageIntegrityInvalid));
}

static void test_replay_mismatch_diagnostic() {
    auto package = makePackage(true);

    GoldenReplayRequest request;
    request.package = package;
    request.target_state_version = package.header.latest_state_version;
    request.expected_condition_trace_keys = {"condition:test:eq:p31_replay"};

    auto report = GoldenReplayRunner{}.run(request);
    assert(report.is_ok());
    assert(report.value().status == GoldenReplayStatus::Failed);
    assert(hasIssue(report.value(), ReplayDiagnosticIssueKind::ReplayMismatch));
    assert(hasIssue(report.value(), ReplayDiagnosticIssueKind::EventCountMismatch));
}

static void test_condition_trace_diagnostic() {
    auto package = makePackage();

    GoldenReplayRequest request;
    request.package = package;
    request.target_state_version = package.header.latest_state_version;
    request.expected_condition_trace_keys = {"condition:test:eq:p31_missing_trace"};
    request.allow_unexpected_condition_traces = false;

    auto report = GoldenReplayRunner{}.run(request);
    assert(report.is_ok());
    assert(report.value().status == GoldenReplayStatus::Failed);
    assert(hasIssue(report.value(), ReplayDiagnosticIssueKind::ConditionTraceMissing));
    assert(hasIssue(report.value(), ReplayDiagnosticIssueKind::ConditionTraceUnexpected));
    assert(report.value().missing_condition_trace_keys.size() == 1);
    assert(report.value().unexpected_condition_trace_keys.size() == 1);
}

static void test_baseline_mismatch_diagnostic() {
    auto package = makePackage();
    auto baseline = GoldenReplayBaselineBuilder{}.build(package, package.header.latest_state_version);
    assert(baseline.is_ok());
    auto altered_baseline = baseline.value();
    altered_baseline.deterministic_signature = HashValue::fromString("p31_other_signature");

    GoldenReplayRequest request;
    request.package = package;
    request.target_state_version = package.header.latest_state_version;
    request.baseline = altered_baseline;

    auto report = GoldenReplayRunner{}.run(request);
    assert(report.is_ok());
    assert(report.value().status == GoldenReplayStatus::Failed);
    assert(hasIssue(report.value(), ReplayDiagnosticIssueKind::BaselineMismatch));
}

static void test_restore_plan_diagnostic() {
    auto package = makePackage();

    GoldenReplayRequest request;
    request.package = package;
    request.target_state_version = package.header.latest_state_version.next();

    auto report = GoldenReplayRunner{}.run(request);
    assert(report.is_ok());
    assert(report.value().status == GoldenReplayStatus::Failed);
    assert(hasIssue(report.value(), ReplayDiagnosticIssueKind::RestorePlanInvalid));
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    const std::string test_name = argv[1];
    if (test_name == "enum") test_enum_roundtrip();
    else if (test_name == "pass") test_golden_replay_passes();
    else if (test_name == "integrity") test_package_integrity_diagnostic();
    else if (test_name == "mismatch") test_replay_mismatch_diagnostic();
    else if (test_name == "condition") test_condition_trace_diagnostic();
    else if (test_name == "baseline") test_baseline_mismatch_diagnostic();
    else if (test_name == "restore") test_restore_plan_diagnostic();
    else return 2;
    std::cout << "golden_replay_diagnostics_test " << test_name << " passed\n";
    return 0;
}
