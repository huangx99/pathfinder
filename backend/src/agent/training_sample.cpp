#include "pathfinder/agent/training_sample.h"

namespace pathfinder::agent {

// --- AgentTrainingSampleStatus ---

std::string toString(AgentTrainingSampleStatus status) {
    switch (status) {
        case AgentTrainingSampleStatus::Unknown: return "Unknown";
        case AgentTrainingSampleStatus::Draft: return "Draft";
        case AgentTrainingSampleStatus::ReplayLocked: return "ReplayLocked";
        case AgentTrainingSampleStatus::Skipped: return "Skipped";
        case AgentTrainingSampleStatus::NoDecision: return "NoDecision";
        case AgentTrainingSampleStatus::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

foundation::Result<AgentTrainingSampleStatus> agentTrainingSampleStatusFromString(const std::string& str) {
    if (str == "Unknown") return foundation::Result<AgentTrainingSampleStatus>::ok(AgentTrainingSampleStatus::Unknown);
    if (str == "Draft") return foundation::Result<AgentTrainingSampleStatus>::ok(AgentTrainingSampleStatus::Draft);
    if (str == "ReplayLocked") return foundation::Result<AgentTrainingSampleStatus>::ok(AgentTrainingSampleStatus::ReplayLocked);
    if (str == "Skipped") return foundation::Result<AgentTrainingSampleStatus>::ok(AgentTrainingSampleStatus::Skipped);
    if (str == "NoDecision") return foundation::Result<AgentTrainingSampleStatus>::ok(AgentTrainingSampleStatus::NoDecision);
    if (str == "Invalid") return foundation::Result<AgentTrainingSampleStatus>::ok(AgentTrainingSampleStatus::Invalid);
    return foundation::Result<AgentTrainingSampleStatus>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "Invalid AgentTrainingSampleStatus: " + str));
}

// --- AgentRewardDraftStatus ---

std::string toString(AgentRewardDraftStatus status) {
    switch (status) {
        case AgentRewardDraftStatus::Unknown: return "Unknown";
        case AgentRewardDraftStatus::NotComputed: return "NotComputed";
        case AgentRewardDraftStatus::PlaceholderOnly: return "PlaceholderOnly";
        default: return "Unknown";
    }
}

foundation::Result<AgentRewardDraftStatus> agentRewardDraftStatusFromString(const std::string& str) {
    if (str == "Unknown") return foundation::Result<AgentRewardDraftStatus>::ok(AgentRewardDraftStatus::Unknown);
    if (str == "NotComputed") return foundation::Result<AgentRewardDraftStatus>::ok(AgentRewardDraftStatus::NotComputed);
    if (str == "PlaceholderOnly") return foundation::Result<AgentRewardDraftStatus>::ok(AgentRewardDraftStatus::PlaceholderOnly);
    return foundation::Result<AgentRewardDraftStatus>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "Invalid AgentRewardDraftStatus: " + str));
}

// --- AgentDoneDraftStatus ---

std::string toString(AgentDoneDraftStatus status) {
    switch (status) {
        case AgentDoneDraftStatus::Unknown: return "Unknown";
        case AgentDoneDraftStatus::NotComputed: return "NotComputed";
        case AgentDoneDraftStatus::PlaceholderOnly: return "PlaceholderOnly";
        default: return "Unknown";
    }
}

foundation::Result<AgentDoneDraftStatus> agentDoneDraftStatusFromString(const std::string& str) {
    if (str == "Unknown") return foundation::Result<AgentDoneDraftStatus>::ok(AgentDoneDraftStatus::Unknown);
    if (str == "NotComputed") return foundation::Result<AgentDoneDraftStatus>::ok(AgentDoneDraftStatus::NotComputed);
    if (str == "PlaceholderOnly") return foundation::Result<AgentDoneDraftStatus>::ok(AgentDoneDraftStatus::PlaceholderOnly);
    return foundation::Result<AgentDoneDraftStatus>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "Invalid AgentDoneDraftStatus: " + str));
}

// --- AgentObservationSampleDraft::validateBasic ---

foundation::Result<void> AgentObservationSampleDraft::validateBasic() const {
    if (agent_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentObservationSampleDraft: agent_id is empty"));
    }
    if (observation_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentObservationSampleDraft: observation_hash is empty"));
    }
    return foundation::Result<void>::ok();
}

// --- AgentActionSampleDraft::validateBasic ---

foundation::Result<void> AgentActionSampleDraft::validateBasic() const {
    if (decision_status == AgentDecisionRecordStatus::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentActionSampleDraft: decision_status cannot be Unknown"));
    }
    if ((decision_status == AgentDecisionRecordStatus::PipelineSucceeded ||
         decision_status == AgentDecisionRecordStatus::PipelineFailed) &&
        !command_id.has_value()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::precondition_missing_capability,
                "AgentActionSampleDraft: PipelineSucceeded/PipelineFailed requires command_id"));
    }
    for (const auto& target : selected_targets) {
        auto valid = target.validateBasic();
        if (valid.is_error()) {
            return valid;
        }
    }
    return foundation::Result<void>::ok();
}

// --- AgentResultSampleDraft::validateBasic ---

foundation::Result<void> AgentResultSampleDraft::validateBasic() const {
    if (runtime_status == AgentRuntimeStatus::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentResultSampleDraft: runtime_status cannot be Unknown"));
    }
    if (input_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentResultSampleDraft: input_hash is empty"));
    }
    if (output_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentResultSampleDraft: output_hash is empty"));
    }
    if (reward_status == AgentRewardDraftStatus::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentResultSampleDraft: reward_status cannot be Unknown"));
    }
    if (done_status == AgentDoneDraftStatus::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentResultSampleDraft: done_status cannot be Unknown"));
    }
    return foundation::Result<void>::ok();
}

// --- AgentTraceSampleDraft::validateBasic ---

foundation::Result<void> AgentTraceSampleDraft::validateBasic() const {
    if (replay_locked && replay_lock_status != AgentReplayLockStatus::Locked) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed,
                "AgentTraceSampleDraft: replay_locked=true requires replay_lock_status=Locked"));
    }
    return foundation::Result<void>::ok();
}

// --- AgentTrainingSampleDraft::validateBasic ---

foundation::Result<void> AgentTrainingSampleDraft::validateBasic() const {
    if (sample_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentTrainingSampleDraft: sample_id is empty"));
    }
    if (source_record_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentTrainingSampleDraft: source_record_id is empty"));
    }
    if (status == AgentTrainingSampleStatus::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentTrainingSampleDraft: status cannot be Unknown"));
    }

    auto obs_valid = observation.validateBasic();
    if (obs_valid.is_error()) return obs_valid;

    auto act_valid = action.validateBasic();
    if (act_valid.is_error()) return act_valid;

    auto res_valid = result.validateBasic();
    if (res_valid.is_error()) return res_valid;

    auto trace_valid = trace.validateBasic();
    if (trace_valid.is_error()) return trace_valid;

    // NoDecision / SubmitSkipped must have explainability in trace
    if (status == AgentTrainingSampleStatus::NoDecision ||
        status == AgentTrainingSampleStatus::Skipped) {
        if (trace.reason_keys.empty() && trace.phase_keys.empty()) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::precondition_missing_capability,
                    "AgentTrainingSampleDraft: NoDecision/Skipped requires reason_keys or phase_keys in trace"));
        }
    }

    return foundation::Result<void>::ok();
}

// --- ID Helper ---

AgentTrainingSampleId makeAgentTrainingSampleId(
    const AgentId& agent_id,
    foundation::Tick tick,
    foundation::StateVersion state_version_before) {
    std::string id = "agent_training_sample_" + agent_id.value() + "_"
        + std::to_string(tick.value()) + "_"
        + std::to_string(state_version_before.value());
    return AgentTrainingSampleId(id);
}

// --- AgentTrainingSampleBuildRequest::validateBasic ---

foundation::Result<void> AgentTrainingSampleBuildRequest::validateBasic() const {
    auto rec_valid = record.validateBasic();
    if (rec_valid.is_error()) {
        return rec_valid;
    }
    if (observation_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentTrainingSampleBuildRequest: observation_hash is empty"));
    }
    return foundation::Result<void>::ok();
}

// --- AgentTrainingSampleDraftBuilder::build ---

foundation::Result<AgentTrainingSampleDraft> AgentTrainingSampleDraftBuilder::build(
    const AgentTrainingSampleBuildRequest& request) const {

    auto req_valid = request.validateBasic();
    if (req_valid.is_error()) {
        return foundation::Result<AgentTrainingSampleDraft>::fail(req_valid.errors()[0]);
    }

    const auto& rec = request.record;

    AgentTrainingSampleDraft sample;
    sample.sample_id = makeAgentTrainingSampleId(rec.agent_id, rec.tick, rec.state_version_before);
    sample.source_record_id = rec.record_id;

    // Determine status from runtime_status
    if (rec.runtime_status == AgentRuntimeStatus::PipelineSucceeded ||
        rec.runtime_status == AgentRuntimeStatus::PipelineFailed) {
        if (request.replay_lock_entry.has_value() &&
            request.replay_lock_entry.value().status == AgentReplayLockStatus::Locked) {
            sample.status = AgentTrainingSampleStatus::ReplayLocked;
        } else {
            sample.status = AgentTrainingSampleStatus::Draft;
        }
    } else if (rec.runtime_status == AgentRuntimeStatus::SubmitSkipped) {
        sample.status = AgentTrainingSampleStatus::Skipped;
    } else if (rec.runtime_status == AgentRuntimeStatus::NoDecision) {
        sample.status = AgentTrainingSampleStatus::NoDecision;
    } else {
        sample.status = AgentTrainingSampleStatus::Invalid;
    }

    // Observation
    sample.observation.agent_id = rec.agent_id;
    sample.observation.actor_id = rec.actor_id;
    sample.observation.tick = rec.tick;
    sample.observation.state_version = rec.state_version_before;
    sample.observation.observed_object_count = 0;  // P9 does not replicate full observation
    sample.observation.observed_threat_count = 0;
    sample.observation.knowledge_claim_count = 0;
    sample.observation.observation_hash = request.observation_hash;

    // Action
    sample.action.decision_id = rec.decision_record.decision_id;
    sample.action.selected_candidate_id = rec.decision_record.selected_candidate_id;
    sample.action.selected_command_action_id = rec.decision_record.selected_command_action_id;
    sample.action.intent_type = rec.decision_record.selected_intent_type;
    sample.action.selected_targets = rec.decision_record.selected_targets;
    sample.action.command_id = rec.decision_record.command_id;
    sample.action.decision_status = rec.decision_record.status;

    // Result
    sample.result.runtime_status = rec.runtime_status;
    sample.result.pipeline_status = rec.pipeline_status;
    sample.result.state_version_before = rec.state_version_before;
    sample.result.state_version_after = rec.state_version_after;
    sample.result.state_change_count = rec.state_change_ids.size();
    sample.result.event_count = rec.event_ids.size();
    sample.result.error_count = rec.error_count;
    sample.result.input_hash = rec.input_hash;
    sample.result.output_hash = rec.output_hash;
    sample.result.reward_status = AgentRewardDraftStatus::NotComputed;
    sample.result.done_status = AgentDoneDraftStatus::NotComputed;

    // Trace: phase_keys from tick record and decision record
    sample.trace.phase_keys = rec.phase_keys;
    for (const auto& pk : rec.decision_record.phase_keys) {
        sample.trace.phase_keys.push_back(pk);
    }
    // reason_keys from decision record
    if (!rec.decision_record.reason_key.empty()) {
        sample.trace.reason_keys.push_back(rec.decision_record.reason_key);
    }
    for (const auto& rk : rec.decision_record.rejected_reason_keys) {
        sample.trace.reason_keys.push_back(rk);
    }
    sample.trace.warning_keys = rec.warning_keys;
    if (request.replay_lock_entry.has_value()) {
        sample.trace.replay_locked = (request.replay_lock_entry.value().status == AgentReplayLockStatus::Locked);
        sample.trace.replay_lock_status = request.replay_lock_entry.value().status;
    }

    auto sample_valid = sample.validateBasic();
    if (sample_valid.is_error()) {
        return foundation::Result<AgentTrainingSampleDraft>::fail(sample_valid.errors()[0]);
    }

    return foundation::Result<AgentTrainingSampleDraft>::ok(std::move(sample));
}

} // namespace pathfinder::agent
