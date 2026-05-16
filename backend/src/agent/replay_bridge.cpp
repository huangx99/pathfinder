#include "pathfinder/agent/replay_bridge.h"

namespace pathfinder::agent {

// --- AgentReplayBridge ---

foundation::Result<replay::CommandReplayRecord> AgentReplayBridge::toCommandReplayRecord(
    const AgentTickRecord& record) const {

    if (record.runtime_status != AgentRuntimeStatus::PipelineSucceeded &&
        record.runtime_status != AgentRuntimeStatus::PipelineFailed) {
        return foundation::Result<replay::CommandReplayRecord>::fail(
            foundation::makeError(foundation::ErrorCode::precondition_missing_capability,
                "AgentReplayBridge: cannot convert record with status " +
                toString(record.runtime_status) + " to CommandReplayRecord"));
    }

    if (!record.command.has_value()) {
        return foundation::Result<replay::CommandReplayRecord>::fail(
            foundation::makeError(foundation::ErrorCode::precondition_missing_capability,
                "AgentReplayBridge: record has no command"));
    }

    auto valid = record.validateBasic();
    if (valid.is_error()) {
        return foundation::Result<replay::CommandReplayRecord>::fail(valid.errors()[0]);
    }

    const auto& cmd = record.command.value();

    replay::CommandReplayRecord replay_record;
    if (record.decision_record.replay_record_id.has_value()) {
        replay_record.record_id = record.decision_record.replay_record_id.value();
    } else {
        replay_record.record_id = replay::ReplayRecordId("agent_replay_" + cmd.command_id.value());
    }
    replay_record.command_id = cmd.command_id;
    replay_record.command = cmd;
    replay_record.state_version_before = record.state_version_before;
    replay_record.state_version_after = record.state_version_after;
    replay_record.random_seed = record.random_seed;
    replay_record.input_hash = record.input_hash;
    replay_record.output_hash = record.output_hash;
    replay_record.state_change_ids = record.state_change_ids;
    replay_record.event_ids = record.event_ids;

    if (record.pipeline_status.has_value()) {
        replay_record.pipeline_status = record.pipeline_status.value();
    } else {
        replay_record.pipeline_status = pipeline::PipelineStatus::NotStarted;
    }

    replay_record.error_count = record.error_count;
    replay_record.status = replay::ReplayRecordStatus::Recorded;

    auto replay_valid = replay_record.validateBasic();
    if (replay_valid.is_error()) {
        return foundation::Result<replay::CommandReplayRecord>::fail(replay_valid.errors()[0]);
    }

    return foundation::Result<replay::CommandReplayRecord>::ok(std::move(replay_record));
}

foundation::Result<void> AgentReplayBridge::appendCommandReplayRecord(
    const AgentTickRecord& record,
    replay::CommandReplayLog& command_log) const {

    auto replay_result = toCommandReplayRecord(record);
    if (replay_result.is_error()) {
        return foundation::Result<void>::fail(replay_result.errors()[0]);
    }

    return command_log.append(std::move(replay_result.value()));
}

// --- AgentReplayLockChecker ---

AgentReplayLockCheckResult AgentReplayLockChecker::check(
    const AgentTickRecord& agent_record,
    const replay::CommandReplayLog& command_log) const {

    AgentReplayLockCheckResult result;
    result.has_agent_record = true;

    if (agent_record.decision_record.command_id.has_value()) {
        auto cmd_id = agent_record.decision_record.command_id.value();
        auto found = command_log.findByCommandId(cmd_id);
        if (found.has_value()) {
            result.has_command_record = true;
            result.can_replay_without_policy = true;
            result.reason_keys.push_back("command_record_found");
        } else {
            result.has_command_record = false;
            result.can_replay_without_policy = false;
            result.reason_keys.push_back("command_record_missing");
        }
    } else {
        result.has_command_record = false;
        result.can_replay_without_policy = false;

        if (agent_record.runtime_status == AgentRuntimeStatus::NoDecision) {
            result.reason_keys.push_back("no_decision_no_command");
        } else if (agent_record.runtime_status == AgentRuntimeStatus::SubmitSkipped) {
            result.reason_keys.push_back("submit_skipped_no_command");
        } else {
            result.reason_keys.push_back("no_command_id");
        }
    }

    return result;
}

} // namespace pathfinder::agent
