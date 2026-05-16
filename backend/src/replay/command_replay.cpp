#include "pathfinder/replay/command_replay.h"
#include <algorithm>

namespace pathfinder::replay {

std::string toString(ReplayRecordStatus status) {
    switch (status) {
        case ReplayRecordStatus::Unknown: return "unknown";
        case ReplayRecordStatus::Recorded: return "recorded";
        case ReplayRecordStatus::Replayed: return "replayed";
        case ReplayRecordStatus::Mismatch: return "mismatch";
    }
    return "unknown";
}

pathfinder::foundation::Result<ReplayRecordStatus> replayRecordStatusFromString(const std::string& str) {
    if (str == "unknown") return pathfinder::foundation::Result<ReplayRecordStatus>::ok(ReplayRecordStatus::Unknown);
    if (str == "recorded") return pathfinder::foundation::Result<ReplayRecordStatus>::ok(ReplayRecordStatus::Recorded);
    if (str == "replayed") return pathfinder::foundation::Result<ReplayRecordStatus>::ok(ReplayRecordStatus::Replayed);
    if (str == "mismatch") return pathfinder::foundation::Result<ReplayRecordStatus>::ok(ReplayRecordStatus::Mismatch);
    return pathfinder::foundation::Result<ReplayRecordStatus>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "replay.record_status_invalid",
            "Unknown ReplayRecordStatus: " + str));
}

pathfinder::foundation::Result<void> CommandReplayRecord::validateBasic() const {
    if (record_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "replay.record_id_missing"));
    }
    if (!pathfinder::foundation::isValidIdString(record_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "replay.record_id_invalid"));
    }
    if (command_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "replay.command_id_missing"));
    }
    if (!pathfinder::foundation::isValidIdString(command_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "replay.command_id_invalid"));
    }
    auto cmd_result = command.validateBasic();
    if (cmd_result.is_error()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.command_invalid",
                "Command validation failed"));
    }
    if (command_id != command.command_id) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.command_id_mismatch",
                "record.command_id != record.command.command_id"));
    }
    if (input_hash.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.input_hash_empty"));
    }
    if (output_hash.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.output_hash_empty"));
    }
    if (status == ReplayRecordStatus::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "replay.status_unknown"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> CommandReplayLog::append(CommandReplayRecord record) {
    // Validate record before accepting
    auto validation = record.validateBasic();
    if (validation.is_error()) return validation;
    // Reject duplicate record_id
    for (const auto& existing : records_) {
        if (existing.record_id == record.record_id) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::command_duplicate,
                    "replay.duplicate_record_id",
                    "Duplicate record_id: " + record.record_id.value()));
        }
    }
    records_.push_back(std::move(record));
    return pathfinder::foundation::Result<void>::ok();
}

std::optional<CommandReplayRecord> CommandReplayLog::findByCommandId(
    const pathfinder::foundation::CommandId& command_id) const {
    for (const auto& record : records_) {
        if (record.command_id == command_id) {
            return record;
        }
    }
    return std::nullopt;
}

pathfinder::foundation::Result<void> CommandReplayLog::validateBasic() const {
    for (const auto& record : records_) {
        auto result = record.validateBasic();
        if (result.is_error()) return result;
    }
    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::replay
