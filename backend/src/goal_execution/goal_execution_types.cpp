#include "pathfinder/goal_execution/goal_execution_types.h"

#include "pathfinder/foundation/error.h"

namespace pathfinder::goal_execution {
namespace {
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
}

std::string toString(ExecutionFrameStatus status) {
    switch (status) {
        case ExecutionFrameStatus::Pending: return "pending";
        case ExecutionFrameStatus::Running: return "running";
        case ExecutionFrameStatus::Paused: return "paused";
        case ExecutionFrameStatus::Completed: return "completed";
        case ExecutionFrameStatus::Cancelled: return "cancelled";
        case ExecutionFrameStatus::Failed: return "failed";
        case ExecutionFrameStatus::Expired: return "expired";
        case ExecutionFrameStatus::WaitingForReasoning: return "waiting_for_reasoning";
        case ExecutionFrameStatus::TestOnly: return "test_only";
        default: return "unknown";
    }
}

std::string toString(ActionDriverKind kind) {
    switch (kind) {
        case ActionDriverKind::Eat: return "eat";
        case ActionDriverKind::Gather: return "gather";
        case ActionDriverKind::ChopWood: return "chop_wood";
        case ActionDriverKind::SharpenTool: return "sharpen_tool";
        case ActionDriverKind::BuildStructure: return "build_structure";
        case ActionDriverKind::UseObject: return "use_object";
        case ActionDriverKind::MoveTo: return "move_to";
        case ActionDriverKind::CounterThreat: return "counter_threat";
        case ActionDriverKind::AdvancedConstruct: return "advanced_construct";
        case ActionDriverKind::TestOnly: return "test_only";
        default: return "unknown";
    }
}

std::string toString(InternalBlockerKind kind) {
    switch (kind) {
        case InternalBlockerKind::MissingObject: return "missing_object";
        case InternalBlockerKind::InsufficientQuantity: return "insufficient_quantity";
        case InternalBlockerKind::ToolStateInsufficient: return "tool_state_insufficient";
        case InternalBlockerKind::ObjectStateInvalid: return "object_state_invalid";
        case InternalBlockerKind::KnowledgeMissing: return "knowledge_missing";
        case InternalBlockerKind::LocationMismatch: return "location_mismatch";
        case InternalBlockerKind::ResourceDepleted: return "resource_depleted";
        case InternalBlockerKind::ResourceReserved: return "resource_reserved";
        case InternalBlockerKind::PathBlocked: return "path_blocked";
        case InternalBlockerKind::MissingCondition: return "missing_condition";
        case InternalBlockerKind::TestOnly: return "test_only";
        default: return "unknown";
    }
}

std::string toString(ExternalInterruptKind kind) {
    switch (kind) {
        case ExternalInterruptKind::ThreatAppeared: return "threat_appeared";
        case ExternalInterruptKind::ThreatEscalated: return "threat_escalated";
        case ExternalInterruptKind::WeatherChanged: return "weather_changed";
        case ExternalInterruptKind::DependentInDanger: return "dependent_in_danger";
        case ExternalInterruptKind::ResourceDestroyed: return "resource_destroyed";
        case ExternalInterruptKind::ToolLostOrBroken: return "tool_lost_or_broken";
        case ExternalInterruptKind::PathBlocked: return "path_blocked";
        case ExternalInterruptKind::FireSpread: return "fire_spread";
        case ExternalInterruptKind::SocialConflict: return "social_conflict";
        case ExternalInterruptKind::DiscoveryFound: return "discovery_found";
        case ExternalInterruptKind::CommandOverride: return "command_override";
        case ExternalInterruptKind::TestOnly: return "test_only";
        default: return "unknown";
    }
}

std::string toString(InterruptDecisionKind kind) {
    switch (kind) {
        case InterruptDecisionKind::Ignore: return "ignore";
        case InterruptDecisionKind::ObserveOnly: return "observe_only";
        case InterruptDecisionKind::PauseAndInsertEmergencyGoal: return "pause_and_insert_emergency_goal";
        case InterruptDecisionKind::CancelCurrentGoal: return "cancel_current_goal";
        case InterruptDecisionKind::ReplanCurrentGoal: return "replan_current_goal";
        case InterruptDecisionKind::ResumeAfterHandling: return "resume_after_handling";
        case InterruptDecisionKind::FailCurrentGoal: return "fail_current_goal";
        case InterruptDecisionKind::TestOnly: return "test_only";
        default: return "ignore";
    }
}

std::string toString(ResumeDecisionKind kind) {
    switch (kind) {
        case ResumeDecisionKind::Resume: return "resume";
        case ResumeDecisionKind::Replan: return "replan";
        case ResumeDecisionKind::Cancel: return "cancel";
        case ResumeDecisionKind::Wait: return "wait";
        case ResumeDecisionKind::Fail: return "fail";
        case ResumeDecisionKind::TestOnly: return "test_only";
        default: return "resume";
    }
}

std::string toString(MaterialConsumeTiming timing) {
    switch (timing) {
        case MaterialConsumeTiming::OnStart: return "on_start";
        case MaterialConsumeTiming::OnFinish: return "on_finish";
        case MaterialConsumeTiming::PerTick: return "per_tick";
        case MaterialConsumeTiming::ReservationOnly: return "reservation_only";
        case MaterialConsumeTiming::TestOnly: return "test_only";
        default: return "reservation_only";
    }
}

std::string toString(ReservationStatus status) {
    switch (status) {
        case ReservationStatus::Active: return "active";
        case ReservationStatus::Released: return "released";
        case ReservationStatus::Consumed: return "consumed";
        case ReservationStatus::Expired: return "expired";
        case ReservationStatus::TestOnly: return "test_only";
        default: return "active";
    }
}

std::string toString(AgentCapabilityTier tier) {
    switch (tier) {
        case AgentCapabilityTier::InstinctAgent: return "instinct_agent";
        case AgentCapabilityTier::SimpleAgent: return "simple_agent";
        case AgentCapabilityTier::CognitiveAgent: return "cognitive_agent";
        case AgentCapabilityTier::AdvancedAgent: return "advanced_agent";
        case AgentCapabilityTier::TestOnly: return "test_only";
        default: return "simple_agent";
    }
}

Result<void> MaterialRequirement::validateBasic() const {
    if (requirement_id.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "material requirement id empty"));
    if (object_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "material requirement object empty"));
    if (required_quantity <= 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "material requirement quantity invalid"));
    return Result<void>::ok();
}

Result<void> MaterialRequirementSet::validateBasic() const {
    if (set_id.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "material requirement set id empty"));
    for (const auto& requirement : requirements) {
        auto valid = requirement.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

} // namespace pathfinder::goal_execution
