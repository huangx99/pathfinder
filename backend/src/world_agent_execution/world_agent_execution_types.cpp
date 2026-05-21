#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_agent_execution {

using pathfinder::foundation::ErrorCategory;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::ErrorDetail;
using pathfinder::foundation::ErrorSeverity;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// WorldAgentExecutionDecision
// ---------------------------------------------------------------------------

std::string toString(WorldAgentExecutionDecision decision) {
    switch (decision) {
        case WorldAgentExecutionDecision::Unknown: return "unknown";
        case WorldAgentExecutionDecision::NoopNoGoal: return "noop_no_goal";
        case WorldAgentExecutionDecision::SelectedGoal: return "selected_goal";
        case WorldAgentExecutionDecision::PlannedStep: return "planned_step";
        case WorldAgentExecutionDecision::IssuedCommand: return "issued_command";
        case WorldAgentExecutionDecision::CommandSucceeded: return "command_succeeded";
        case WorldAgentExecutionDecision::CommandBlocked: return "command_blocked";
        case WorldAgentExecutionDecision::CommandFailed: return "command_failed";
        case WorldAgentExecutionDecision::PausedForInterrupt: return "paused_for_interrupt";
        case WorldAgentExecutionDecision::InsertedSubGoal: return "inserted_sub_goal";
        case WorldAgentExecutionDecision::WaitingForKnowledge: return "waiting_for_knowledge";
        case WorldAgentExecutionDecision::CompletedGoal: return "completed_goal";
        case WorldAgentExecutionDecision::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<WorldAgentExecutionDecision> worldAgentExecutionDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::Unknown);
    if (str == "noop_no_goal") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::NoopNoGoal);
    if (str == "selected_goal") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::SelectedGoal);
    if (str == "planned_step") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::PlannedStep);
    if (str == "issued_command") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::IssuedCommand);
    if (str == "command_succeeded") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::CommandSucceeded);
    if (str == "command_blocked") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::CommandBlocked);
    if (str == "command_failed") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::CommandFailed);
    if (str == "paused_for_interrupt") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::PausedForInterrupt);
    if (str == "inserted_sub_goal") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::InsertedSubGoal);
    if (str == "waiting_for_knowledge") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::WaitingForKnowledge);
    if (str == "completed_goal") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::CompletedGoal);
    if (str == "test_only") return Result<WorldAgentExecutionDecision>::ok(WorldAgentExecutionDecision::TestOnly);
    return Result<WorldAgentExecutionDecision>::fail(makeError(ErrorCode::validation_enum_unknown, "world_agent_execution_decision_unknown", str));
}

// ---------------------------------------------------------------------------
// WorldAgentExecutionFailureKind
// ---------------------------------------------------------------------------

std::string toString(WorldAgentExecutionFailureKind kind) {
    switch (kind) {
        case WorldAgentExecutionFailureKind::None: return "none";
        case WorldAgentExecutionFailureKind::InvalidRequest: return "invalid_request";
        case WorldAgentExecutionFailureKind::ActorMissing: return "actor_missing";
        case WorldAgentExecutionFailureKind::ContextBuildFailed: return "context_build_failed";
        case WorldAgentExecutionFailureKind::KnowledgeMissing: return "knowledge_missing";
        case WorldAgentExecutionFailureKind::ReasoningFailed: return "reasoning_failed";
        case WorldAgentExecutionFailureKind::PlanEmpty: return "plan_empty";
        case WorldAgentExecutionFailureKind::CommandCompileFailed: return "command_compile_failed";
        case WorldAgentExecutionFailureKind::CommandPipelineFailed: return "command_pipeline_failed";
        case WorldAgentExecutionFailureKind::CommandBlocked: return "command_blocked";
        case WorldAgentExecutionFailureKind::RuntimeUnavailable: return "runtime_unavailable";
        case WorldAgentExecutionFailureKind::InventoryUnavailable: return "inventory_unavailable";
        case WorldAgentExecutionFailureKind::OwnershipInvalid: return "ownership_invalid";
        case WorldAgentExecutionFailureKind::Interrupted: return "interrupted";
        case WorldAgentExecutionFailureKind::MaxDepthExceeded: return "max_depth_exceeded";
        case WorldAgentExecutionFailureKind::ForbiddenBySafety: return "forbidden_by_safety";
        case WorldAgentExecutionFailureKind::TestOnly: return "test_only";
    }
    return "none";
}

Result<WorldAgentExecutionFailureKind> worldAgentExecutionFailureKindFromString(const std::string& str) {
    if (str == "none") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::None);
    if (str == "invalid_request") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::InvalidRequest);
    if (str == "actor_missing") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::ActorMissing);
    if (str == "context_build_failed") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::ContextBuildFailed);
    if (str == "knowledge_missing") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::KnowledgeMissing);
    if (str == "reasoning_failed") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::ReasoningFailed);
    if (str == "plan_empty") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::PlanEmpty);
    if (str == "command_compile_failed") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::CommandCompileFailed);
    if (str == "command_pipeline_failed") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::CommandPipelineFailed);
    if (str == "command_blocked") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::CommandBlocked);
    if (str == "runtime_unavailable") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::RuntimeUnavailable);
    if (str == "inventory_unavailable") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::InventoryUnavailable);
    if (str == "ownership_invalid") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::OwnershipInvalid);
    if (str == "interrupted") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::Interrupted);
    if (str == "max_depth_exceeded") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::MaxDepthExceeded);
    if (str == "forbidden_by_safety") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::ForbiddenBySafety);
    if (str == "test_only") return Result<WorldAgentExecutionFailureKind>::ok(WorldAgentExecutionFailureKind::TestOnly);
    return Result<WorldAgentExecutionFailureKind>::fail(makeError(ErrorCode::validation_enum_unknown, "world_agent_execution_failure_kind_unknown", str));
}

// ---------------------------------------------------------------------------
// WorldAgentGoalSourceKind
// ---------------------------------------------------------------------------

std::string toString(WorldAgentGoalSourceKind kind) {
    switch (kind) {
        case WorldAgentGoalSourceKind::Unknown: return "unknown";
        case WorldAgentGoalSourceKind::InternalNeed: return "internal_need";
        case WorldAgentGoalSourceKind::PlayerOrder: return "player_order";
        case WorldAgentGoalSourceKind::WorldThreat: return "world_threat";
        case WorldAgentGoalSourceKind::ExternalInterrupt: return "external_interrupt";
        case WorldAgentGoalSourceKind::KnowledgeOpportunity: return "knowledge_opportunity";
        case WorldAgentGoalSourceKind::QueuedGoal: return "queued_goal";
        case WorldAgentGoalSourceKind::SystemMaintenance: return "system_maintenance";
        case WorldAgentGoalSourceKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<WorldAgentGoalSourceKind> worldAgentGoalSourceKindFromString(const std::string& str) {
    if (str == "unknown") return Result<WorldAgentGoalSourceKind>::ok(WorldAgentGoalSourceKind::Unknown);
    if (str == "internal_need") return Result<WorldAgentGoalSourceKind>::ok(WorldAgentGoalSourceKind::InternalNeed);
    if (str == "player_order") return Result<WorldAgentGoalSourceKind>::ok(WorldAgentGoalSourceKind::PlayerOrder);
    if (str == "world_threat") return Result<WorldAgentGoalSourceKind>::ok(WorldAgentGoalSourceKind::WorldThreat);
    if (str == "external_interrupt") return Result<WorldAgentGoalSourceKind>::ok(WorldAgentGoalSourceKind::ExternalInterrupt);
    if (str == "knowledge_opportunity") return Result<WorldAgentGoalSourceKind>::ok(WorldAgentGoalSourceKind::KnowledgeOpportunity);
    if (str == "queued_goal") return Result<WorldAgentGoalSourceKind>::ok(WorldAgentGoalSourceKind::QueuedGoal);
    if (str == "system_maintenance") return Result<WorldAgentGoalSourceKind>::ok(WorldAgentGoalSourceKind::SystemMaintenance);
    if (str == "test_only") return Result<WorldAgentGoalSourceKind>::ok(WorldAgentGoalSourceKind::TestOnly);
    return Result<WorldAgentGoalSourceKind>::fail(makeError(ErrorCode::validation_enum_unknown, "world_agent_goal_source_kind_unknown", str));
}

// ---------------------------------------------------------------------------
// WorldAgentStepCommandKind
// ---------------------------------------------------------------------------

std::string toString(WorldAgentStepCommandKind kind) {
    switch (kind) {
        case WorldAgentStepCommandKind::Unknown: return "unknown";
        case WorldAgentStepCommandKind::Move: return "move";
        case WorldAgentStepCommandKind::Gather: return "gather";
        case WorldAgentStepCommandKind::Chop: return "chop";
        case WorldAgentStepCommandKind::Mine: return "mine";
        case WorldAgentStepCommandKind::Dig: return "dig";
        case WorldAgentStepCommandKind::Pickup: return "pickup";
        case WorldAgentStepCommandKind::Drop: return "drop";
        case WorldAgentStepCommandKind::Eat: return "eat";
        case WorldAgentStepCommandKind::Use: return "use";
        case WorldAgentStepCommandKind::Craft: return "craft";
        case WorldAgentStepCommandKind::Teach: return "teach";
        case WorldAgentStepCommandKind::Attack: return "attack";
        case WorldAgentStepCommandKind::Flee: return "flee";
        case WorldAgentStepCommandKind::Wait: return "wait";
        case WorldAgentStepCommandKind::CastAreaEffect: return "cast_area_effect";
        case WorldAgentStepCommandKind::TriggerAreaEvent: return "trigger_area_event";
        case WorldAgentStepCommandKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<WorldAgentStepCommandKind> worldAgentStepCommandKindFromString(const std::string& str) {
    if (str == "unknown") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Unknown);
    if (str == "move") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Move);
    if (str == "gather") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Gather);
    if (str == "chop") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Chop);
    if (str == "mine") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Mine);
    if (str == "dig") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Dig);
    if (str == "pickup") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Pickup);
    if (str == "drop") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Drop);
    if (str == "eat") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Eat);
    if (str == "use") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Use);
    if (str == "craft") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Craft);
    if (str == "teach") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Teach);
    if (str == "attack") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Attack);
    if (str == "flee") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Flee);
    if (str == "wait") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::Wait);
    if (str == "cast_area_effect") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::CastAreaEffect);
    if (str == "trigger_area_event") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::TriggerAreaEvent);
    if (str == "test_only") return Result<WorldAgentStepCommandKind>::ok(WorldAgentStepCommandKind::TestOnly);
    return Result<WorldAgentStepCommandKind>::fail(makeError(ErrorCode::validation_enum_unknown, "world_agent_step_command_kind_unknown", str));
}

// ---------------------------------------------------------------------------
// validateBasic
// ---------------------------------------------------------------------------

Result<void> WorldAgentTickRequest::validateBasic() const {
    if (request_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "request.request_id_required"));
    }
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "request.actor_key_required"));
    }
    if (max_subgoal_depth == 0 || max_subgoal_depth > 64) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "request.max_subgoal_depth_out_of_range"));
    }
    return Result<void>::ok();
}

Result<void> WorldAgentDecisionContext::validateBasic() const {
    if (context_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "context.context_id_required"));
    }
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "context.actor_key_required"));
    }
    if (actor.actor_key != actor_key) {
        return Result<void>::fail(makeError(ErrorCode::command_invalid_argument, "context.actor_mismatch"));
    }
    return Result<void>::ok();
}

Result<void> WorldAgentCompiledCommand::validateBasic() const {
    if (ok) {
        if (command.command_id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "compiled_command.command_id_required"));
        }
        auto v = command.validateBasic();
        if (v.is_error()) return v;
        if (step_command_kind == WorldAgentStepCommandKind::Unknown) {
            return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "compiled_command.step_kind_required"));
        }
    }
    return Result<void>::ok();
}

Result<void> WorldAgentTickResult::validateBasic() const {
    if (ok) {
        if (actor_key.empty()) {
            return Result<void>::fail(makeError(ErrorCode::command_missing_required_field, "tick_result.actor_key_required"));
        }
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// Forbidden keys
// ---------------------------------------------------------------------------

std::vector<std::string> worldAgentExecutionForbiddenKeys() {
    return {
        "hidden_truth",
        "true_trait",
        "real_effect",
        "raw_state",
        "actual_hp",
        "true_hp",
        "hp_delta",
        "death",
        "kill",
        "corpse",
        "loot",
        "drop_secret",
        "random_damage",
        "frontend_unlock",
        "direct_state"
    };
}

bool containsWorldAgentExecutionForbiddenKey(const std::string& text) {
    const auto& keys = worldAgentExecutionForbiddenKeys();
    for (const auto& key : keys) {
        if (text.find(key) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsWorldAgentExecutionForbiddenKey(const std::vector<std::string>& values) {
    for (const auto& v : values) {
        if (containsWorldAgentExecutionForbiddenKey(v)) return true;
    }
    return false;
}

} // namespace pathfinder::world_agent_execution
