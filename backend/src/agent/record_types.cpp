#include "pathfinder/agent/record_types.h"

namespace pathfinder::agent {

// --- ID Helpers ---

AgentTickRecordId makeAgentTickRecordId(
    const AgentId& agent_id,
    foundation::Tick tick,
    foundation::StateVersion state_version) {
    std::string id = "agent_tick_" + agent_id.value() + "_"
        + std::to_string(tick.value()) + "_"
        + std::to_string(state_version.value());
    return AgentTickRecordId(id);
}

AgentDecisionRecordId makeAgentDecisionRecordId(
    const foundation::DecisionId& decision_id) {
    std::string id = "agent_decision_record_" + decision_id.value();
    return AgentDecisionRecordId(id);
}

// --- AgentDecisionRecordStatus ---

std::string toString(AgentDecisionRecordStatus status) {
    switch (status) {
        case AgentDecisionRecordStatus::Unknown: return "Unknown";
        case AgentDecisionRecordStatus::Selected: return "Selected";
        case AgentDecisionRecordStatus::NoDecision: return "NoDecision";
        case AgentDecisionRecordStatus::AdapterRejected: return "AdapterRejected";
        case AgentDecisionRecordStatus::SubmitSkipped: return "SubmitSkipped";
        case AgentDecisionRecordStatus::PipelineSucceeded: return "PipelineSucceeded";
        case AgentDecisionRecordStatus::PipelineFailed: return "PipelineFailed";
        case AgentDecisionRecordStatus::Failed: return "Failed";
        default: return "Unknown";
    }
}

foundation::Result<AgentDecisionRecordStatus> agentDecisionRecordStatusFromString(const std::string& str) {
    if (str == "Unknown") return foundation::Result<AgentDecisionRecordStatus>::ok(AgentDecisionRecordStatus::Unknown);
    if (str == "Selected") return foundation::Result<AgentDecisionRecordStatus>::ok(AgentDecisionRecordStatus::Selected);
    if (str == "NoDecision") return foundation::Result<AgentDecisionRecordStatus>::ok(AgentDecisionRecordStatus::NoDecision);
    if (str == "AdapterRejected") return foundation::Result<AgentDecisionRecordStatus>::ok(AgentDecisionRecordStatus::AdapterRejected);
    if (str == "SubmitSkipped") return foundation::Result<AgentDecisionRecordStatus>::ok(AgentDecisionRecordStatus::SubmitSkipped);
    if (str == "PipelineSucceeded") return foundation::Result<AgentDecisionRecordStatus>::ok(AgentDecisionRecordStatus::PipelineSucceeded);
    if (str == "PipelineFailed") return foundation::Result<AgentDecisionRecordStatus>::ok(AgentDecisionRecordStatus::PipelineFailed);
    if (str == "Failed") return foundation::Result<AgentDecisionRecordStatus>::ok(AgentDecisionRecordStatus::Failed);
    return foundation::Result<AgentDecisionRecordStatus>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "Invalid AgentDecisionRecordStatus: " + str));
}

// --- AgentTickRecordStatus ---

std::string toString(AgentTickRecordStatus status) {
    switch (status) {
        case AgentTickRecordStatus::Unknown: return "Unknown";
        case AgentTickRecordStatus::Recorded: return "Recorded";
        case AgentTickRecordStatus::ReplayLinked: return "ReplayLinked";
        case AgentTickRecordStatus::ReplayMatched: return "ReplayMatched";
        case AgentTickRecordStatus::ReplayMismatched: return "ReplayMismatched";
        case AgentTickRecordStatus::Invalid: return "Invalid";
        default: return "Unknown";
    }
}

foundation::Result<AgentTickRecordStatus> agentTickRecordStatusFromString(const std::string& str) {
    if (str == "Unknown") return foundation::Result<AgentTickRecordStatus>::ok(AgentTickRecordStatus::Unknown);
    if (str == "Recorded") return foundation::Result<AgentTickRecordStatus>::ok(AgentTickRecordStatus::Recorded);
    if (str == "ReplayLinked") return foundation::Result<AgentTickRecordStatus>::ok(AgentTickRecordStatus::ReplayLinked);
    if (str == "ReplayMatched") return foundation::Result<AgentTickRecordStatus>::ok(AgentTickRecordStatus::ReplayMatched);
    if (str == "ReplayMismatched") return foundation::Result<AgentTickRecordStatus>::ok(AgentTickRecordStatus::ReplayMismatched);
    if (str == "Invalid") return foundation::Result<AgentTickRecordStatus>::ok(AgentTickRecordStatus::Invalid);
    return foundation::Result<AgentTickRecordStatus>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "Invalid AgentTickRecordStatus: " + str));
}

// --- AgentDecisionRecord::validateBasic ---

foundation::Result<void> AgentDecisionRecord::validateBasic() const {
    if (record_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentDecisionRecord: record_id is empty"));
    }
    if (agent_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentDecisionRecord: agent_id is empty"));
    }
    if (status == AgentDecisionRecordStatus::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentDecisionRecord: status cannot be Unknown"));
    }
    if (confidence < 0.0 || confidence > 1.0) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_value_out_of_range, "AgentDecisionRecord: confidence must be in [0.0, 1.0]"));
    }
    if ((status == AgentDecisionRecordStatus::PipelineSucceeded ||
         status == AgentDecisionRecordStatus::PipelineFailed) &&
        !command_id.has_value()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::precondition_missing_capability, "AgentDecisionRecord: PipelineSucceeded/PipelineFailed requires command_id"));
    }
    if (status == AgentDecisionRecordStatus::NoDecision) {
        bool has_reason = !reason_key.empty();
        bool has_phases = !phase_keys.empty();
        if (!has_reason && !has_phases) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::precondition_missing_capability, "AgentDecisionRecord: NoDecision requires reason_key or phase_keys for explainability"));
        }
    }
    return foundation::Result<void>::ok();
}

// --- AgentTickRecord::validateBasic ---

foundation::Result<void> AgentTickRecord::validateBasic() const {
    if (record_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentTickRecord: record_id is empty"));
    }
    if (record_status == AgentTickRecordStatus::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentTickRecord: record_status cannot be Unknown"));
    }
    if (runtime_status == AgentRuntimeStatus::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "AgentTickRecord: runtime_status cannot be Unknown"));
    }
    if (input_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentTickRecord: input_hash is empty"));
    }
    if (output_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentTickRecord: output_hash is empty"));
    }
    if (command.has_value() && decision_record.command_id.has_value()) {
        if (command.value().command_id != decision_record.command_id.value()) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::validation_failed, "AgentTickRecord: command.command_id != decision_record.command_id"));
        }
    }
    if (pipeline_status.has_value() && !command.has_value()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::precondition_missing_capability, "AgentTickRecord: pipeline_status requires command"));
    }
    if (runtime_status == AgentRuntimeStatus::PipelineSucceeded) {
        if (!pipeline_status.has_value() || pipeline_status.value() != pipeline::PipelineStatus::Succeeded) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::validation_failed, "AgentTickRecord: PipelineSucceeded requires pipeline_status=Succeeded"));
        }
    }
    auto dec_result = decision_record.validateBasic();
    if (dec_result.is_error()) {
        return dec_result;
    }
    return foundation::Result<void>::ok();
}

// --- AgentTickLog ---

foundation::Result<void> AgentTickLog::append(AgentTickRecord record) {
    auto valid = record.validateBasic();
    if (valid.is_error()) {
        return valid;
    }
    for (const auto& r : records_) {
        if (r.record_id == record.record_id) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::command_duplicate, "AgentTickLog: duplicate record_id"));
        }
    }
    records_.push_back(std::move(record));
    return foundation::Result<void>::ok();
}

std::optional<AgentTickRecord> AgentTickLog::findByRecordId(AgentTickRecordId id) const {
    for (const auto& r : records_) {
        if (r.record_id == id) {
            return r;
        }
    }
    return std::nullopt;
}

std::vector<AgentTickRecord> AgentTickLog::findByAgentId(AgentId agent_id) const {
    std::vector<AgentTickRecord> result;
    for (const auto& r : records_) {
        if (r.agent_id == agent_id) {
            result.push_back(r);
        }
    }
    return result;
}

std::optional<AgentTickRecord> AgentTickLog::findByDecisionId(foundation::DecisionId decision_id) const {
    for (const auto& r : records_) {
        if (r.decision_record.decision_id == decision_id) {
            return r;
        }
    }
    return std::nullopt;
}

foundation::Result<void> AgentTickLog::validateBasic() const {
    for (const auto& r : records_) {
        auto valid = r.validateBasic();
        if (valid.is_error()) {
            return valid;
        }
    }
    return foundation::Result<void>::ok();
}

// --- AgentRecordBuildRequest::validateBasic ---

foundation::Result<void> AgentRecordBuildRequest::validateBasic() const {
    auto req_valid = tick_request.validateBasic();
    if (req_valid.is_error()) {
        return req_valid;
    }
    auto res_valid = tick_result.validateBasic();
    if (res_valid.is_error()) {
        return res_valid;
    }
    if (input_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentRecordBuildRequest: input_hash is empty"));
    }
    if (output_hash.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "AgentRecordBuildRequest: output_hash is empty"));
    }
    return foundation::Result<void>::ok();
}

} // namespace pathfinder::agent
