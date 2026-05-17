#include "pathfinder/protocol/agent_projection.h"
#include "pathfinder/foundation/id.h"

namespace pathfinder::protocol {

using namespace pathfinder::agent;
using namespace pathfinder::foundation;

// --- AgentHistoryItemProjection ---

Result<void> AgentHistoryItemProjection::validateBasic() const {
    if (record_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.record_id_missing"));
    }
    if (agent_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.agent_id_missing"));
    }
    if (actor_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.actor_id_missing"));
    }
    if (runtime_status.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "projection.runtime_status_empty"));
    }
    if (decision_status.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "projection.decision_status_empty"));
    }
    return Result<void>::ok();
}

// --- AgentHistoryProjection ---

Result<void> AgentHistoryProjection::validateBasic() const {
    if (query_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.query_id_missing"));
    }
    if (projection_mode == AgentHistoryProjectionMode::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "projection.mode_unknown"));
    }
    for (const auto& item : items) {
        auto r = item.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

// --- AgentReplayLockItemProjection ---

Result<void> AgentReplayLockItemProjection::validateBasic() const {
    if (agent_record_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.lock_item_record_id_missing"));
    }
    if (agent_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.lock_item_agent_id_missing"));
    }
    if (replay_lock_status.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "projection.lock_item_status_empty"));
    }
    return Result<void>::ok();
}

// --- AgentReplayLockProjection ---

Result<void> AgentReplayLockProjection::validateBasic() const {
    if (lock_set_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.lock_set_id_missing"));
    }
    for (const auto& entry : entries) {
        auto r = entry.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

// --- AgentTrainingSampleProjection ---

Result<void> AgentTrainingSampleProjection::validateBasic() const {
    if (sample_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.sample_id_missing"));
    }
    if (source_record_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.source_record_id_missing"));
    }
    if (status.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "projection.sample_status_empty"));
    }
    if (agent_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::id_missing, "projection.sample_agent_id_missing"));
    }
    if (runtime_status.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "projection.sample_runtime_status_empty"));
    }
    if (decision_status.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "projection.sample_decision_status_empty"));
    }
    if (reward_status.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "projection.sample_reward_status_empty"));
    }
    if (done_status.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "projection.sample_done_status_empty"));
    }
    return Result<void>::ok();
}

// --- AgentHistoryProjectorInput ---

Result<void> AgentHistoryProjectorInput::validateBasic() const {
    auto r = query_result.validateBasic();
    if (r.is_error()) return r;
    if (projection_mode == AgentHistoryProjectionMode::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "projection.input_mode_unknown"));
    }
    return Result<void>::ok();
}

// --- AgentHistoryProjector ---

Result<AgentHistoryProjection> AgentHistoryProjector::project(
    const AgentHistoryProjectorInput& input) const {
    auto vr = input.validateBasic();
    if (vr.is_error()) return Result<AgentHistoryProjection>::fail(vr.errors());

    AgentHistoryProjection projection;
    projection.query_id = input.query_result.query_id.value();
    projection.projection_mode = input.projection_mode;
    projection.total_matched = input.query_result.total_matched;
    projection.truncated = input.query_result.truncated;

    // Build lookup maps for replay lock and training samples
    std::unordered_map<std::string, const AgentReplayLockEntry*> lock_map;
    if (input.replay_lock_set) {
        for (const auto& entry : input.replay_lock_set->entries) {
            lock_map[entry.agent_record_id.value()] = &entry;
        }
    }

    std::unordered_map<std::string, const AgentTrainingSampleDraft*> sample_map;
    if (input.training_samples) {
        for (const auto& sample : *input.training_samples) {
            sample_map[sample.source_record_id.value()] = &sample;
        }
    }

    for (const auto& record : input.query_result.records) {
        AgentHistoryItemProjection item;
        item.record_id = record.record_id.value();
        item.agent_id = record.agent_id.value();
        item.actor_id = record.actor_id.value();
        item.tick = static_cast<int64_t>(record.tick.value());
        item.state_version_before = record.state_version_before.value();
        item.state_version_after = record.state_version_after.value();

        item.runtime_status = toString(record.runtime_status);
        item.decision_status = toString(record.decision_record.status);

        if (!record.decision_record.selected_candidate_id.empty()) {
            item.selected_candidate_id = record.decision_record.selected_candidate_id.value();
        }
        if (!record.decision_record.selected_command_action_id.empty()) {
            item.selected_command_action_id = record.decision_record.selected_command_action_id.value();
        }
        if (record.decision_record.command_id.has_value()) {
            item.command_id = record.decision_record.command_id.value().value();
        }
        if (record.decision_record.replay_record_id.has_value()) {
            item.replay_record_id = record.decision_record.replay_record_id.value().value();
        }

        // Associate replay lock
        auto lock_it = lock_map.find(record.record_id.value());
        if (lock_it != lock_map.end()) {
            item.replay_lock_status = toString(lock_it->second->status);
        }

        // Associate training sample
        auto sample_it = sample_map.find(record.record_id.value());
        if (sample_it != sample_map.end()) {
            item.training_sample_status = toString(sample_it->second->status);
        }

        item.state_change_count = record.state_change_ids.size();
        item.event_count = record.event_ids.size();
        item.error_count = record.error_count;

        // Phase/reason/warning keys: only in Debug/Training mode with include_debug_trace
        if (input.include_debug_trace &&
            (input.projection_mode == AgentHistoryProjectionMode::Debug ||
             input.projection_mode == AgentHistoryProjectionMode::Training)) {
            // Collect phase keys from both tick record and decision record
            item.phase_keys = record.phase_keys;
            for (const auto& pk : record.decision_record.phase_keys) {
                bool found = false;
                for (const auto& existing : item.phase_keys) {
                    if (existing == pk) { found = true; break; }
                }
                if (!found) item.phase_keys.push_back(pk);
            }
            item.warning_keys = record.warning_keys;
            // Add reason keys from decision record
            if (!record.decision_record.reason_key.empty()) {
                item.reason_keys.push_back(record.decision_record.reason_key);
            }
            for (const auto& rk : record.decision_record.rejected_reason_keys) {
                item.reason_keys.push_back(rk);
            }
        }

        projection.items.push_back(std::move(item));
    }

    return Result<AgentHistoryProjection>::ok(std::move(projection));
}

// --- AgentProtocolProjectionAdapter ---

ProtocolEnvelope AgentProtocolProjectionAdapter::buildHistoryEnvelope(
    const AgentHistoryProjection& projection,
    const AgentProjectionProtocolOptions& options) const {
    ProtocolEnvelope env;
    env.protocol_version = options.protocol_version;
    env.message_id = "agent_history_projection_" + projection.query_id;
    env.message_type = ProtocolMessageType::Response;
    env.domain = ProtocolDomain::Query;
    env.payload.payload_type = ProtocolPayloadType::AgentHistoryProjection;
    env.payload.message_key = "agent.history_projection";
    env.request_id = options.request_id;
    env.correlation_id = options.correlation_id;
    env.session_id = options.session_id;
    return env;
}

ProtocolEnvelope AgentProtocolProjectionAdapter::buildReplayLockEnvelope(
    const AgentReplayLockProjection& projection,
    const AgentProjectionProtocolOptions& options) const {
    ProtocolEnvelope env;
    env.protocol_version = options.protocol_version;
    env.message_id = "agent_replay_lock_projection_" + projection.lock_set_id;
    env.message_type = ProtocolMessageType::Response;
    env.domain = ProtocolDomain::ProjectionSync;
    env.payload.payload_type = ProtocolPayloadType::AgentReplayLockProjection;
    env.payload.message_key = "agent.replay_lock_projection";
    env.request_id = options.request_id;
    env.correlation_id = options.correlation_id;
    env.session_id = options.session_id;
    return env;
}

ProtocolEnvelope AgentProtocolProjectionAdapter::buildTrainingSampleEnvelope(
    const AgentTrainingSampleProjection& projection,
    const AgentProjectionProtocolOptions& options) const {
    ProtocolEnvelope env;
    env.protocol_version = options.protocol_version;
    env.message_id = "agent_training_sample_projection_" + projection.sample_id;
    env.message_type = ProtocolMessageType::Response;
    env.domain = ProtocolDomain::ProjectionSync;
    env.payload.payload_type = ProtocolPayloadType::AgentTrainingSampleProjection;
    env.payload.message_key = "agent.training_sample_projection";
    env.request_id = options.request_id;
    env.correlation_id = options.correlation_id;
    env.session_id = options.session_id;
    return env;
}

} // namespace pathfinder::protocol
