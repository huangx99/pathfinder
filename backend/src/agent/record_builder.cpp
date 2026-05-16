#include "pathfinder/agent/record_builder.h"

namespace pathfinder::agent {

foundation::Result<AgentTickRecord> AgentRecordBuilder::build(
    const AgentRecordBuildRequest& request) const {

    auto valid = request.validateBasic();
    if (valid.is_error()) {
        return foundation::Result<AgentTickRecord>::fail(valid.errors()[0]);
    }

    const auto& req = request.tick_request;
    const auto& res = request.tick_result;

    // Build AgentDecisionRecord
    AgentDecisionRecord dec_record;

    switch (res.status) {
        case AgentRuntimeStatus::PipelineSucceeded:
            dec_record.status = AgentDecisionRecordStatus::PipelineSucceeded;
            break;
        case AgentRuntimeStatus::PipelineFailed:
            dec_record.status = AgentDecisionRecordStatus::PipelineFailed;
            break;
        case AgentRuntimeStatus::SubmitSkipped:
            dec_record.status = AgentDecisionRecordStatus::SubmitSkipped;
            break;
        case AgentRuntimeStatus::NoDecision:
            dec_record.status = AgentDecisionRecordStatus::NoDecision;
            break;
        case AgentRuntimeStatus::DecisionMade:
            dec_record.status = AgentDecisionRecordStatus::Selected;
            break;
        case AgentRuntimeStatus::CommandCreated:
            dec_record.status = AgentDecisionRecordStatus::Selected;
            break;
        case AgentRuntimeStatus::Failed:
            dec_record.status = AgentDecisionRecordStatus::Failed;
            break;
        default:
            dec_record.status = AgentDecisionRecordStatus::Failed;
            break;
    }

    dec_record.agent_id = req.binding.agent_id;
    dec_record.actor_id = req.binding.primary_actor_id;
    dec_record.decision_tick = req.issued_tick;

    if (res.decision.decision_id.empty()) {
        if (res.status == AgentRuntimeStatus::NoDecision) {
            dec_record.decision_id = foundation::DecisionId(
                "decision_" + req.binding.agent_id.value() + "_" + std::to_string(req.issued_tick.value()));
        }
    } else {
        dec_record.decision_id = res.decision.decision_id;
    }

    dec_record.observation_state_version = res.decision.observation_state_version;
    dec_record.selected_candidate_id = res.trace.selected_candidate_id;
    dec_record.selected_command_action_id = res.trace.selected_command_action_id;

    if (res.decision.selected_intent.intent_type != AgentIntentType::Unknown) {
        dec_record.selected_intent_type = res.decision.selected_intent.intent_type;
        dec_record.selected_intent_action_id = res.decision.selected_intent.action_id;
        dec_record.selected_targets = res.decision.selected_intent.targets;
        dec_record.confidence = res.decision.selected_intent.confidence;
        dec_record.reason_key = res.decision.selected_intent.reason_key;
    }

    if (res.command.has_value()) {
        dec_record.command_id = res.command.value().command_id;
    }

    if (!dec_record.decision_id.empty()) {
        dec_record.record_id = makeAgentDecisionRecordId(dec_record.decision_id);
    }

    dec_record.phase_keys = res.trace.phase_keys;
    dec_record.rejected_reason_keys = res.trace.reason_keys;

    // Build AgentTickRecord
    AgentTickRecord tick_record;

    tick_record.agent_id = req.binding.agent_id;
    tick_record.actor_id = req.binding.primary_actor_id;
    tick_record.tick = req.issued_tick;
    tick_record.random_seed = req.random_seed;
    tick_record.runtime_status = res.status;

    tick_record.state_version_before = res.decision.observation_state_version;
    if (res.state_version_after.has_value()) {
        tick_record.state_version_after = res.state_version_after.value();
    } else if (res.pipeline_result.has_value()) {
        auto change_count = res.pipeline_result.value().state_changes.changes().size();
        tick_record.state_version_after = foundation::StateVersion(
            res.decision.observation_state_version.value() + change_count);
    } else {
        tick_record.state_version_after = res.decision.observation_state_version;
    }

    tick_record.record_status = AgentTickRecordStatus::Recorded;
    tick_record.decision_record = dec_record;
    tick_record.command = res.command;
    tick_record.input_hash = request.input_hash;
    tick_record.output_hash = request.output_hash;

    if (res.pipeline_result.has_value()) {
        const auto& pr = res.pipeline_result.value();
        tick_record.pipeline_status = pr.status;
        for (const auto& change : pr.state_changes.changes()) {
            tick_record.state_change_ids.push_back(change.change_id);
        }
        for (const auto& event : pr.events.events()) {
            tick_record.event_ids.push_back(event.event_id);
        }
        tick_record.error_count = pr.errors.size();
    }

    tick_record.phase_keys = res.trace.phase_keys;

    tick_record.record_id = makeAgentTickRecordId(
        tick_record.agent_id, tick_record.tick, tick_record.state_version_before);

    auto tick_valid = tick_record.validateBasic();
    if (tick_valid.is_error()) {
        return foundation::Result<AgentTickRecord>::fail(tick_valid.errors()[0]);
    }

    return foundation::Result<AgentTickRecord>::ok(std::move(tick_record));
}

} // namespace pathfinder::agent
