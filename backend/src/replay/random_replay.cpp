#include "pathfinder/replay/random_replay.h"

namespace pathfinder::replay {

pathfinder::foundation::Result<void> RandomReplayEntry::validateBasic() const {
    if (entry_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "replay.random_entry_id_missing"));
    }
    if (draw_index < 0) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "replay.draw_index_negative"));
    }
    if (min_value > max_value) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "replay.min_greater_than_max"));
    }
    if (result_value < min_value || result_value > max_value) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "replay.result_out_of_range"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> RandomReplayLog::append(RandomReplayEntry entry) {
    // Validate entry before accepting
    auto validation = entry.validateBasic();
    if (validation.is_error()) return validation;
    for (const auto& existing : entries_) {
        if (existing.entry_id == entry.entry_id) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::command_duplicate,
                    "replay.duplicate_random_entry_id",
                    "Duplicate entry_id: " + entry.entry_id.value()));
        }
    }
    entries_.push_back(std::move(entry));
    return pathfinder::foundation::Result<void>::ok();
}

std::vector<RandomReplayEntry> RandomReplayLog::findByCommandId(
    const pathfinder::foundation::CommandId& command_id) const {
    std::vector<RandomReplayEntry> result;
    for (const auto& entry : entries_) {
        if (entry.command_id == command_id) {
            result.push_back(entry);
        }
    }
    return result;
}

pathfinder::foundation::Result<void> RandomReplayLog::validateBasic() const {
    for (const auto& entry : entries_) {
        auto result = entry.validateBasic();
        if (result.is_error()) return result;
    }
    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::replay
