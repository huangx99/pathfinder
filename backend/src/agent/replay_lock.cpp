#include "pathfinder/agent/replay_lock.h"
#include <algorithm>

namespace pathfinder::agent {

// --- AgentReplayLockStatus ---

std::string toString(AgentReplayLockStatus status) {
    switch (status) {
        case AgentReplayLockStatus::Unknown: return "Unknown";
        case AgentReplayLockStatus::Locked: return "Locked";
        case AgentReplayLockStatus::ExplainedOnly: return "ExplainedOnly";
        case AgentReplayLockStatus::Broken: return "Broken";
        case AgentReplayLockStatus::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

foundation::Result<AgentReplayLockStatus> agentReplayLockStatusFromString(const std::string& str) {
    if (str == "Unknown") return foundation::Result<AgentReplayLockStatus>::ok(AgentReplayLockStatus::Unknown);
    if (str == "Locked") return foundation::Result<AgentReplayLockStatus>::ok(AgentReplayLockStatus::Locked);
    if (str == "ExplainedOnly") return foundation::Result<AgentReplayLockStatus>::ok(AgentReplayLockStatus::ExplainedOnly);
    if (str == "Broken") return foundation::Result<AgentReplayLockStatus>::ok(AgentReplayLockStatus::Broken);
    if (str == "Invalid") return foundation::Result<AgentReplayLockStatus>::ok(AgentReplayLockStatus::Invalid);
    return foundation::Result<AgentReplayLockStatus>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "Invalid AgentReplayLockStatus: " + str));
}

// --- AgentReplayLockReason ---

std::string toString(AgentReplayLockReason reason) {
    switch (reason) {
        case AgentReplayLockReason::Unknown: return "Unknown";
        case AgentReplayLockReason::CommandRecordMatched: return "CommandRecordMatched";
        case AgentReplayLockReason::NoCommandExpected: return "NoCommandExpected";
        case AgentReplayLockReason::CommandRecordMissing: return "CommandRecordMissing";
        case AgentReplayLockReason::CommandIdMismatch: return "CommandIdMismatch";
        case AgentReplayLockReason::AgentRecordInvalid: return "AgentRecordInvalid";
        case AgentReplayLockReason::CommandRecordInvalid: return "CommandRecordInvalid";
        default: return "Unknown";
    }
}

foundation::Result<AgentReplayLockReason> agentReplayLockReasonFromString(const std::string& str) {
    if (str == "Unknown") return foundation::Result<AgentReplayLockReason>::ok(AgentReplayLockReason::Unknown);
    if (str == "CommandRecordMatched") return foundation::Result<AgentReplayLockReason>::ok(AgentReplayLockReason::CommandRecordMatched);
    if (str == "NoCommandExpected") return foundation::Result<AgentReplayLockReason>::ok(AgentReplayLockReason::NoCommandExpected);
    if (str == "CommandRecordMissing") return foundation::Result<AgentReplayLockReason>::ok(AgentReplayLockReason::CommandRecordMissing);
    if (str == "CommandIdMismatch") return foundation::Result<AgentReplayLockReason>::ok(AgentReplayLockReason::CommandIdMismatch);
    if (str == "AgentRecordInvalid") return foundation::Result<AgentReplayLockReason>::ok(AgentReplayLockReason::AgentRecordInvalid);
    if (str == "CommandRecordInvalid") return foundation::Result<AgentReplayLockReason>::ok(AgentReplayLockReason::CommandRecordInvalid);
    return foundation::Result<AgentReplayLockReason>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "Invalid AgentReplayLockReason: " + str));
}

// --- AgentReplayLockEntry::validateBasic ---

foundation::Result<void> AgentReplayLockEntry::validateBasic() const {
    if (agent_record_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentReplayLockEntry: agent_record_id is empty"));
    }
    if (agent_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentReplayLockEntry: agent_id is empty"));
    }
    if (status == AgentReplayLockStatus::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentReplayLockEntry: status cannot be Unknown"));
    }
    if (status == AgentReplayLockStatus::Locked) {
        if (!command_id.has_value()) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::precondition_missing_capability, "AgentReplayLockEntry: Locked requires command_id"));
        }
        if (!replay_record_id.has_value()) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::precondition_missing_capability, "AgentReplayLockEntry: Locked requires replay_record_id"));
        }
    }
    if (status == AgentReplayLockStatus::Broken) {
        if (reasons.empty()) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::precondition_missing_capability, "AgentReplayLockEntry: Broken requires at least one reason"));
        }
    }
    return foundation::Result<void>::ok();
}

// --- AgentReplayLockSet::validateBasic ---

foundation::Result<void> AgentReplayLockSet::validateBasic() const {
    if (lock_set_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentReplayLockSet: lock_set_id is empty"));
    }
    if (entries.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentReplayLockSet: entries cannot be empty"));
    }
    if (source_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentReplayLockSet: source_hash is empty"));
    }
    for (const auto& entry : entries) {
        auto result = entry.validateBasic();
        if (result.is_error()) {
            return result;
        }
    }
    return foundation::Result<void>::ok();
}

bool AgentReplayLockSet::allReplayableWithoutPolicy() const {
    if (entries.empty()) return false;
    for (const auto& entry : entries) {
        if (entry.status != AgentReplayLockStatus::Locked &&
            entry.status != AgentReplayLockStatus::ExplainedOnly) {
            return false;
        }
    }
    return true;
}

bool AgentReplayLockSet::hasBrokenEntry() const {
    for (const auto& entry : entries) {
        if (entry.status == AgentReplayLockStatus::Broken) {
            return true;
        }
    }
    return false;
}

// --- ID Helper ---

AgentReplayLockSetId makeAgentReplayLockSetId(
    foundation::Tick first_tick,
    foundation::Tick last_tick,
    size_t entry_count) {
    std::string id = "agent_replay_lock_set_"
        + std::to_string(first_tick.value()) + "_"
        + std::to_string(last_tick.value()) + "_"
        + std::to_string(entry_count);
    return AgentReplayLockSetId(id);
}

// --- AgentReplayLockBuildRequest::validateBasic ---

foundation::Result<void> AgentReplayLockBuildRequest::validateBasic() const {
    if (!agent_log) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentReplayLockBuildRequest: agent_log is null"));
    }
    if (!command_log) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentReplayLockBuildRequest: command_log is null"));
    }
    if (source_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentReplayLockBuildRequest: source_hash is empty"));
    }
    return foundation::Result<void>::ok();
}

// --- AgentReplayLockSetBuilder::build ---

foundation::Result<AgentReplayLockSet> AgentReplayLockSetBuilder::build(
    const AgentReplayLockBuildRequest& request) const {

    auto req_valid = request.validateBasic();
    if (req_valid.is_error()) {
        return foundation::Result<AgentReplayLockSet>::fail(req_valid.errors()[0]);
    }

    const auto& agent_records = request.agent_log->records();
    if (agent_records.empty()) {
        return foundation::Result<AgentReplayLockSet>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed,
                "AgentReplayLockSetBuilder: agent_log is empty"));
    }

    std::vector<AgentReplayLockEntry> entries;
    entries.reserve(agent_records.size());

    for (const auto& record : agent_records) {
        AgentReplayLockEntry entry;
        entry.agent_record_id = record.record_id;
        entry.agent_id = record.agent_id;
        entry.tick = record.tick;
        entry.state_version_before = record.state_version_before;
        entry.state_version_after = record.state_version_after;
        entry.decision_id = record.decision_record.decision_id;
        entry.command_id = record.decision_record.command_id;

        // Check agent record validity
        auto agent_valid = record.validateBasic();
        if (agent_valid.is_error()) {
            entry.status = AgentReplayLockStatus::Invalid;
            entry.reasons.push_back(AgentReplayLockReason::AgentRecordInvalid);
            entry.reason_keys.push_back("agent_record_invalid");
            entries.push_back(std::move(entry));
            continue;
        }

        // No command expected: SubmitSkipped or NoDecision
        if (record.runtime_status == AgentRuntimeStatus::SubmitSkipped ||
            record.runtime_status == AgentRuntimeStatus::NoDecision) {
            entry.status = AgentReplayLockStatus::ExplainedOnly;
            entry.reasons.push_back(AgentReplayLockReason::NoCommandExpected);
            entry.reason_keys.push_back(record.runtime_status == AgentRuntimeStatus::SubmitSkipped
                ? "submit_skipped_no_command" : "no_decision_no_command");
            entries.push_back(std::move(entry));
            continue;
        }

        // Has command_id -> must find matching CommandReplayRecord
        if (record.decision_record.command_id.has_value()) {
            auto cmd_id = record.decision_record.command_id.value();
            auto found = request.command_log->findByCommandId(cmd_id);
            if (found.has_value()) {
                // Verify the replay record itself
                auto replay_valid = found.value().validateBasic();
                if (replay_valid.is_error()) {
                    entry.status = AgentReplayLockStatus::Broken;
                    entry.reasons.push_back(AgentReplayLockReason::CommandRecordInvalid);
                    entry.reason_keys.push_back("command_record_invalid");
                } else {
                    entry.status = AgentReplayLockStatus::Locked;
                    entry.reasons.push_back(AgentReplayLockReason::CommandRecordMatched);
                    entry.reason_keys.push_back("command_record_matched");
                    entry.replay_record_id = found.value().record_id;
                }
            } else {
                entry.status = AgentReplayLockStatus::Broken;
                entry.reasons.push_back(AgentReplayLockReason::CommandRecordMissing);
                entry.reason_keys.push_back("command_record_missing");
            }
        } else {
            // No command_id but not SubmitSkipped/NoDecision - treat as Invalid
            entry.status = AgentReplayLockStatus::Invalid;
            entry.reasons.push_back(AgentReplayLockReason::AgentRecordInvalid);
            entry.reason_keys.push_back("no_command_id_unexpected");
        }

        entries.push_back(std::move(entry));
    }

    // Build lock set
    AgentReplayLockSet lock_set;
    lock_set.source_hash = request.source_hash;

    foundation::Tick first_tick = entries.front().tick;
    foundation::Tick last_tick = entries.back().tick;
    lock_set.lock_set_id = makeAgentReplayLockSetId(first_tick, last_tick, entries.size());
    lock_set.entries = std::move(entries);

    auto set_valid = lock_set.validateBasic();
    if (set_valid.is_error()) {
        return foundation::Result<AgentReplayLockSet>::fail(set_valid.errors()[0]);
    }

    return foundation::Result<AgentReplayLockSet>::ok(std::move(lock_set));
}

} // namespace pathfinder::agent
