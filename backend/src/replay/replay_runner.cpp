#include "pathfinder/replay/replay_runner.h"

namespace pathfinder::replay {

std::string toString(ReplayCompareStatus status) {
    switch (status) {
        case ReplayCompareStatus::Unknown: return "unknown";
        case ReplayCompareStatus::Match: return "match";
        case ReplayCompareStatus::Mismatch: return "mismatch";
    }
    return "unknown";
}

pathfinder::foundation::Result<ReplayResult> ReplayRunner::replayOne(const ReplayInput& input) {
    // Validate initial state
    auto state_validation = input.initial_state.validateBasic();
    if (state_validation.is_error()) {
        return pathfinder::foundation::Result<ReplayResult>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.invalid_initial_state",
                "Initial state validation failed"));
    }
    // Validate command log
    auto cmd_log_validation = input.command_log.validateBasic();
    if (cmd_log_validation.is_error()) {
        return pathfinder::foundation::Result<ReplayResult>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.invalid_command_log",
                "Command log validation failed"));
    }
    // Validate random log
    auto rand_log_validation = input.random_log.validateBasic();
    if (rand_log_validation.is_error()) {
        return pathfinder::foundation::Result<ReplayResult>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.invalid_random_log",
                "Random log validation failed"));
    }
    // Validate: command log must have exactly 1 record
    if (input.command_log.size() == 0) {
        return pathfinder::foundation::Result<ReplayResult>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.empty_command_log",
                "Command log is empty"));
    }
    if (input.command_log.size() > 1) {
        return pathfinder::foundation::Result<ReplayResult>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::common_not_implemented,
                "replay.multi_command_not_supported",
                "P4 only supports single command replay"));
    }

    const auto& record = input.command_log.records()[0];

    // Check random log has seed entry for this command
    auto rand_entries = input.random_log.findByCommandId(record.command_id);
    if (rand_entries.empty()) {
        return pathfinder::foundation::Result<ReplayResult>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.random_seed_missing",
                "No random seed entry found for command_id: " + record.command_id.value()));
    }
    // Check seed consistency
    bool seed_match = false;
    for (const auto& entry : rand_entries) {
        if (entry.seed == record.random_seed) {
            seed_match = true;
            break;
        }
    }
    if (!seed_match) {
        return pathfinder::foundation::Result<ReplayResult>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.random_seed_mismatch",
                "Random log seed does not match record.random_seed"));
    }

    // Create a mutable copy of the initial state for replay
    pathfinder::state::GameState replay_state = input.initial_state;
    replay_state.metadata.state_version = record.state_version_before;

    // Build pipeline context
    pathfinder::pipeline::PipelineContext context;
    context.command = record.command;
    context.state_metadata = replay_state.metadata;
    context.game_state = &replay_state;
    context.random_seed = record.random_seed;

    // Execute through RulePipeline
    auto exec_result = pipeline_.execute(context);
    if (exec_result.is_error()) {
        return pathfinder::foundation::Result<ReplayResult>::fail(exec_result.errors()[0]);
    }

    auto& actual = exec_result.value();

    // Build compare report
    ReplayCompareReport report;
    report.expected_state_change_count = record.state_change_ids.size();
    report.actual_state_change_count = actual.state_changes.size();
    report.expected_event_count = record.event_ids.size();
    report.actual_event_count = actual.events.size();
    report.expected_pipeline_status = record.pipeline_status;
    report.actual_pipeline_status = actual.status;
    report.expected_state_version_after = record.state_version_after;
    report.actual_state_version_after = replay_state.metadata.state_version;

    // Compare
    bool match = true;
    if (report.expected_state_change_count != report.actual_state_change_count) {
        report.differences.push_back("state_change_count mismatch: expected " +
            std::to_string(report.expected_state_change_count) + " got " +
            std::to_string(report.actual_state_change_count));
        match = false;
    }
    if (report.expected_event_count != report.actual_event_count) {
        report.differences.push_back("event_count mismatch: expected " +
            std::to_string(report.expected_event_count) + " got " +
            std::to_string(report.actual_event_count));
        match = false;
    }
    if (report.expected_pipeline_status != report.actual_pipeline_status) {
        report.differences.push_back("pipeline_status mismatch");
        match = false;
    }
    if (report.expected_state_version_after != report.actual_state_version_after) {
        report.differences.push_back("state_version_after mismatch");
        match = false;
    }

    report.status = match ? ReplayCompareStatus::Match : ReplayCompareStatus::Mismatch;

    ReplayResult result;
    result.report = report;
    result.pipeline_result = std::move(actual);
    result.replay_state = replay_state;
    return pathfinder::foundation::Result<ReplayResult>::ok(std::move(result));
}

} // namespace pathfinder::replay
