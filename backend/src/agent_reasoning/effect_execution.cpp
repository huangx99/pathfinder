#include "pathfinder/agent_reasoning/effect_execution.h"

#include "pathfinder/agent_reasoning/reaction_planning_adapter.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/reaction/reaction_fixtures.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <utility>

namespace pathfinder::agent_reasoning {

using pathfinder::condition::ConditionEvaluationContext;
using pathfinder::condition::ConditionExpressionEvaluator;
using pathfinder::condition::ConditionExpressionRef;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::world_interaction::AgentAutonomyActionKind;
using pathfinder::world_interaction::AgentAutonomyResult;
using pathfinder::world_interaction::InteractionFailureKind;
using pathfinder::world_interaction::WorldChange;
using pathfinder::world_interaction::WorldChangeKind;
using pathfinder::world_interaction::WorldObjectInstance;
using pathfinder::world_interaction::WorldSnapshot;

namespace {

Result<void> fail(const std::string& message) {
    return Result<void>::fail(makeError(ErrorCode::validation_failed, message));
}

template <typename T>
Result<T> failValue(const std::string& message) {
    return Result<T>::fail(makeError(ErrorCode::validation_failed, message));
}

bool forbiddenText(const std::string& value) {
    static const std::vector<std::string> forbidden = {
        "hidden_truth", "true_trait", "real_effect", "raw_state", "actual_hp", "true_hp",
        "hp_delta", "death", "kill", "corpse", "loot", "frontend_unlock", "direct_state",
        "internal_memory_raw", "private_reasoning", "secret_goal"};
    return std::any_of(forbidden.begin(), forbidden.end(), [&](const auto& key) { return value.find(key) != std::string::npos; });
}

bool forbiddenText(const std::vector<std::string>& values) {
    return std::any_of(values.begin(), values.end(), [](const auto& value) { return forbiddenText(value); });
}

bool isSnakeKey(const std::string& value) {
    if (value.empty()) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::islower(c) || std::isdigit(c) || c == '_' || c == '.';
    });
}

ConditionExpressionRef condition(std::string key) {
    ConditionExpressionRef ref;
    ref.inline_canonical_key = std::move(key);
    return ref;
}

PlanPrecondition precondition(std::string domain, std::string key, std::string required, bool can_plan) {
    PlanPrecondition item;
    item.condition_id = "condition." + domain + "." + key + "." + required;
    item.condition_expression = "condition:test:eq:" + domain + "." + key + "." + required;
    item.missing_domain = std::move(domain);
    item.missing_key = std::move(key);
    item.required_value = std::move(required);
    item.can_be_planned = can_plan;
    return item;
}

PlanPrecondition quantityPrecondition(const std::string& key, int required, bool can_plan) {
    return precondition("object.quantity", key, "gte." + std::to_string(required), can_plan);
}

PlanPrecondition statePrecondition(const std::string& object_key, const std::string& state_key, int required, bool can_plan) {
    return precondition("object.state", object_key + "." + state_key, "gte." + std::to_string(required), can_plan);
}

EffectExecutionOperation op(
    std::string id,
    EffectExecutionOpKind kind,
    EffectExecutionTargetKind target,
    std::string summary,
    int quantity = 0,
    double number = 0.0,
    std::string state_key = {},
    ExecutionValueSourceKind key_source = ExecutionValueSourceKind::Constant,
    std::string runtime_key = {},
    std::string output_key = {}) {
    EffectExecutionOperation value;
    value.operation_id = std::move(id);
    value.op_kind = kind;
    value.target_kind = target;
    value.safe_summary_zh_cn = std::move(summary);
    value.quantity_value = quantity;
    value.number_value = number;
    value.state_key = std::move(state_key);
    value.key_source = key_source;
    value.runtime_key = std::move(runtime_key);
    value.effect_output_key = std::move(output_key);
    value.trace_keys = {"p41.operation:" + value.operation_id};
    return value;
}

EffectExecutionSpec spec(
    std::string spec_id,
    std::string effect_key,
    std::vector<ConditionExpressionRef> conditions,
    std::vector<EffectExecutionOperation> operations,
    std::string summary) {
    EffectExecutionSpec value;
    value.spec_id = std::move(spec_id);
    value.effect_key = std::move(effect_key);
    value.condition_refs = std::move(conditions);
    value.operations = std::move(operations);
    value.safe_summary_zh_cn = std::move(summary);
    value.trace_keys = {"p41.spec:" + value.spec_id};
    return value;
}

std::string safeFactForQuantity(const std::string& key, int required) {
    return "object.quantity." + key + ".gte." + std::to_string(required);
}

std::string safeFactForState(const std::string& key, const std::string& state, int required) {
    return "object.state." + key + "." + state + ".gte." + std::to_string(required);
}

std::vector<std::string> safeFactsFromSnapshot(const WorldSnapshot& snapshot) {
    std::vector<std::string> facts;
    for (const auto& [key, object] : snapshot.objects_by_key) {
        for (int threshold = 1; threshold <= 6; ++threshold) {
            if (object.quantity >= threshold) facts.push_back(safeFactForQuantity(key, threshold));
        }
        for (const auto& [state_key, value] : object.numeric_states) {
            for (int threshold = 1; threshold <= 6; ++threshold) {
                if (value >= threshold) facts.push_back(safeFactForState(key, state_key, threshold));
                if (key == "wood" && state_key == "processed" && value >= threshold) facts.push_back(safeFactForQuantity("wood_processed", threshold));
            }
        }
        for (const auto& tag : object.state_tags) facts.push_back("object.tag." + key + "." + tag);
    }
    for (const auto& [key, threat] : snapshot.threats_by_key) {
        if (threat.active && !threat.resolved) facts.push_back("threat.active." + key);
    }
    return facts;
}

const WorldObjectInstance* objectByKey(const WorldSnapshot& snapshot, const std::string& key) {
    auto it = snapshot.objects_by_key.find(key);
    return it == snapshot.objects_by_key.end() ? nullptr : &it->second;
}

double numericState(const WorldObjectInstance& object, const std::string& state_key) {
    auto it = object.numeric_states.find(state_key);
    return it == object.numeric_states.end() ? 0.0 : it->second;
}

std::string targetKey(const EffectExecutionOperation& operation, const WorldExecutionRequest& request) {
    if (!operation.runtime_key.empty()) return operation.runtime_key;
    if (operation.key_source == ExecutionValueSourceKind::EffectOutputKey) return operation.effect_output_key;
    if (operation.key_source == ExecutionValueSourceKind::RequestObjectKey) return request.object_key;
    if (operation.key_source == ExecutionValueSourceKind::RequestTargetKey) return request.target_object_key.empty() ? request.target_threat_key : request.target_object_key;
    if (operation.key_source == ExecutionValueSourceKind::RequestActorKey) return request.actor_key;
    if (!operation.effect_output_key.empty()) return operation.effect_output_key;
    switch (operation.target_kind) {
        case EffectExecutionTargetKind::ActorSelf: return request.actor_key;
        case EffectExecutionTargetKind::ObjectSelf: return request.object_key;
        case EffectExecutionTargetKind::ObjectTarget: return request.target_object_key;
        case EffectExecutionTargetKind::ThreatTarget: return request.target_threat_key.empty() ? request.target_object_key : request.target_threat_key;
        case EffectExecutionTargetKind::RuntimeKey: return operation.runtime_key;
        default: return {};
    }
}

bool isExecutableOperationKind(EffectExecutionOpKind kind) {
    switch (kind) {
        case EffectExecutionOpKind::ConsumeObjectQuantity:
        case EffectExecutionOpKind::AddObjectQuantity:
        case EffectExecutionOpKind::SetObjectQuantity:
        case EffectExecutionOpKind::AddObjectStateNumber:
        case EffectExecutionOpKind::SetObjectStateNumber:
        case EffectExecutionOpKind::AddObjectTag:
        case EffectExecutionOpKind::RemoveObjectTag:
        case EffectExecutionOpKind::ChangeActorNeed:
        case EffectExecutionOpKind::ChangeThreatLevel:
        case EffectExecutionOpKind::ResolveThreat:
        case EffectExecutionOpKind::EmitWorldEvent:
        case EffectExecutionOpKind::QueueFollowupGoal:
            return true;
        default:
            return false;
    }
}

WorldChange changeForOperation(const EffectExecutionOperation& operation, const WorldExecutionRequest& request, const std::string& prefix) {
    WorldChange change;
    change.change_id = prefix + operation.operation_id;
    change.player_summary_zh_cn = operation.safe_summary_zh_cn;
    change.reason_keys = operation.trace_keys;
    const auto target = targetKey(operation, request);
    if (operation.op_kind == EffectExecutionOpKind::ConsumeObjectQuantity) {
        change.kind = WorldChangeKind::ObjectConsumed;
        change.target_instance_id = target;
        change.quantity_delta = -std::abs(operation.quantity_value);
    } else if (operation.op_kind == EffectExecutionOpKind::AddObjectQuantity) {
        change.kind = WorldChangeKind::ObjectGenerated;
        change.target_instance_id = target;
        change.definition_key = target;
        change.quantity_delta = std::abs(operation.quantity_value);
    } else if (operation.op_kind == EffectExecutionOpKind::SetObjectQuantity) {
        change.kind = WorldChangeKind::ObjectQuantityChanged;
        change.target_instance_id = target;
        change.quantity_delta = operation.quantity_value;
    } else if (operation.op_kind == EffectExecutionOpKind::AddObjectStateNumber) {
        change.kind = WorldChangeKind::ObjectStateChanged;
        change.target_instance_id = target;
        change.state_key = operation.state_key;
        change.numeric_delta = operation.number_value;
    } else if (operation.op_kind == EffectExecutionOpKind::SetObjectStateNumber) {
        change.kind = WorldChangeKind::ObjectStateChanged;
        change.target_instance_id = target;
        change.state_key = operation.state_key;
        change.numeric_set_value = operation.number_value;
    } else if (operation.op_kind == EffectExecutionOpKind::AddObjectTag) {
        change.kind = WorldChangeKind::ObjectStateChanged;
        change.target_instance_id = target;
        change.state_key = operation.state_key.empty() ? std::optional<std::string>("tag") : std::optional<std::string>(operation.state_key);
        change.tag_add = {operation.runtime_key};
    } else if (operation.op_kind == EffectExecutionOpKind::RemoveObjectTag) {
        change.kind = WorldChangeKind::ObjectStateChanged;
        change.target_instance_id = target;
        change.state_key = operation.state_key.empty() ? std::optional<std::string>("tag") : std::optional<std::string>(operation.state_key);
        change.tag_remove = {operation.runtime_key};
    } else if (operation.op_kind == EffectExecutionOpKind::ChangeActorNeed) {
        change.kind = WorldChangeKind::ActorNeedChanged;
        change.target_actor_key = target;
        change.state_key = operation.state_key;
        change.numeric_delta = operation.number_value;
    } else if (operation.op_kind == EffectExecutionOpKind::ChangeThreatLevel) {
        change.kind = WorldChangeKind::ThreatLevelChanged;
        change.target_threat_key = target;
        change.numeric_delta = operation.number_value;
        if (!operation.state_key.empty()) change.state_key = operation.state_key;
    } else if (operation.op_kind == EffectExecutionOpKind::ResolveThreat) {
        change.kind = WorldChangeKind::ThreatResolved;
        change.target_threat_key = target;
    } else if (operation.op_kind == EffectExecutionOpKind::EmitWorldEvent) {
        change.kind = WorldChangeKind::StoryOutcomeReached;
        change.target_actor_key = request.actor_key;
    } else if (operation.op_kind == EffectExecutionOpKind::QueueFollowupGoal) {
        change.kind = WorldChangeKind::AgentActionQueued;
        change.target_actor_key = request.actor_key;
    }
    return change;
}

ExecutionFailureKind validateOperationAgainstSnapshot(const EffectExecutionOperation& operation, const WorldExecutionRequest& request, const WorldSnapshot& snapshot) {
    const auto target = targetKey(operation, request);
    if (operation.target_kind == EffectExecutionTargetKind::ObjectTarget && target.empty()) return ExecutionFailureKind::MissingTarget;
    if (operation.target_kind == EffectExecutionTargetKind::ThreatTarget && target.empty()) return ExecutionFailureKind::MissingTarget;
    if (operation.op_kind == EffectExecutionOpKind::ConsumeObjectQuantity) {
        const auto* object = objectByKey(snapshot, target);
        if (!object) return ExecutionFailureKind::MissingObject;
        if (object->quantity < std::abs(operation.quantity_value)) return ExecutionFailureKind::InsufficientQuantity;
    }
    if (operation.op_kind == EffectExecutionOpKind::AddObjectStateNumber || operation.op_kind == EffectExecutionOpKind::SetObjectStateNumber) {
        const auto* object = objectByKey(snapshot, target);
        if (!object) return ExecutionFailureKind::MissingObject;
        if (operation.op_kind == EffectExecutionOpKind::AddObjectStateNumber && numericState(*object, operation.state_key) + operation.number_value < 0.0) return ExecutionFailureKind::InsufficientQuantity;
    }
    if (operation.op_kind == EffectExecutionOpKind::ResolveThreat || operation.op_kind == EffectExecutionOpKind::ChangeThreatLevel) {
        if (snapshot.threats_by_key.find(target) == snapshot.threats_by_key.end()) return ExecutionFailureKind::MissingTarget;
    }
    return ExecutionFailureKind::Unknown;
}

std::string summaryForFailure(ExecutionFailureKind kind) {
    switch (kind) {
        case ExecutionFailureKind::SpecNotFound: return "这个效果还没有登记执行规格。";
        case ExecutionFailureKind::ConditionNotMet: return "执行前置条件还不满足。";
        case ExecutionFailureKind::MissingObject: return "缺少执行需要的对象。";
        case ExecutionFailureKind::MissingTarget: return "缺少执行需要的目标。";
        case ExecutionFailureKind::InsufficientQuantity: return "材料数量不足，不能完成这一步。";
        case ExecutionFailureKind::ExpressionFailed: return "前置条件表达式无法安全计算。";
        case ExecutionFailureKind::InvalidOperation: return "执行规格包含非法操作。";
        default: return "执行被安全阻止。";
    }
}

WorldExecutionResult failedExecution(ExecutionFailureKind kind, std::string summary, std::vector<std::string> trace = {}) {
    WorldExecutionResult result;
    result.ok = false;
    result.executed = false;
    result.failure_kind = kind;
    result.safe_summary_zh_cn = std::move(summary);
    result.trace_keys = std::move(trace);
    return result;
}

AgentAutonomyActionKind actionKindForStep(const AgentPlanStep& step) {
    if (step.kind == PlanStepKind::RestoreTool) return AgentAutonomyActionKind::MaintainTool;
    if (step.kind == PlanStepKind::PrepareObject || step.kind == PlanStepKind::BuildStructure) return AgentAutonomyActionKind::GatherMaterial;
    if (step.target_key.has_value()) return AgentAutonomyActionKind::HoldTorch;
    return AgentAutonomyActionKind::FollowActor;
}

} // namespace

#define PF_ENUM_TO_STRING(name, snake) case name: return snake

std::string toString(EffectExecutionOpKind kind) {
    switch (kind) {
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ConsumeObjectQuantity, "consume_object_quantity");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::AddObjectQuantity, "add_object_quantity");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::SetObjectQuantity, "set_object_quantity");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::AddObjectStateNumber, "add_object_state_number");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::SetObjectStateNumber, "set_object_state_number");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::AddObjectTag, "add_object_tag");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::RemoveObjectTag, "remove_object_tag");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ChangeActorNeed, "change_actor_need");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ChangeThreatLevel, "change_threat_level");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ResolveThreat, "resolve_threat");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::EmitWorldEvent, "emit_world_event");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::QueueFollowupGoal, "queue_followup_goal");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ApplyStatusEffect, "apply_status_effect");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::RemoveStatusEffect, "remove_status_effect");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ChangeRelationship, "change_relationship");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::CreateAgreement, "create_agreement");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::BreakAgreement, "break_agreement");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ChangeReputation, "change_reputation");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::UnlockCapability, "unlock_capability");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ChangePopulation, "change_population");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ChangeSystemPressure, "change_system_pressure");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ChangeWorldRule, "change_world_rule");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::PropagateKnowledge, "propagate_knowledge");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::CorruptKnowledge, "corrupt_knowledge");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ChangeKnowledgeTrust, "change_knowledge_trust");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::ScheduleDelayedEffect, "schedule_delayed_effect");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::StartPeriodicEffect, "start_periodic_effect");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::StopPeriodicEffect, "stop_periodic_effect");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::TestOnly, "test_only");
        PF_ENUM_TO_STRING(EffectExecutionOpKind::Unknown, "unknown");
    }
    return "unknown";
}

std::string toString(EffectExecutionTargetKind kind) {
    switch (kind) {
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::ActorSelf, "actor_self");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::ActorTarget, "actor_target");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::ObjectSelf, "object_self");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::ObjectTarget, "object_target");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::Location, "location");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::ThreatTarget, "threat_target");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::GroupOrTribe, "group_or_tribe");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::RuntimeKey, "runtime_key");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::InventoryScope, "inventory_scope");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::Region, "region");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::WorldSystem, "world_system");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::Faction, "faction");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::Civilization, "civilization");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::Megastructure, "megastructure");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::KnowledgeGraph, "knowledge_graph");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::Agreement, "agreement");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::Timeline, "timeline");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::TestOnly, "test_only");
        PF_ENUM_TO_STRING(EffectExecutionTargetKind::Unknown, "unknown");
    }
    return "unknown";
}

std::string toString(WorldScopeKind kind) {
    switch (kind) {
        PF_ENUM_TO_STRING(WorldScopeKind::Actor, "actor");
        PF_ENUM_TO_STRING(WorldScopeKind::Inventory, "inventory");
        PF_ENUM_TO_STRING(WorldScopeKind::Object, "object");
        PF_ENUM_TO_STRING(WorldScopeKind::Location, "location");
        PF_ENUM_TO_STRING(WorldScopeKind::Region, "region");
        PF_ENUM_TO_STRING(WorldScopeKind::Threat, "threat");
        PF_ENUM_TO_STRING(WorldScopeKind::GroupOrTribe, "group_or_tribe");
        PF_ENUM_TO_STRING(WorldScopeKind::Faction, "faction");
        PF_ENUM_TO_STRING(WorldScopeKind::Civilization, "civilization");
        PF_ENUM_TO_STRING(WorldScopeKind::WorldSystem, "world_system");
        PF_ENUM_TO_STRING(WorldScopeKind::Megastructure, "megastructure");
        PF_ENUM_TO_STRING(WorldScopeKind::KnowledgeGraph, "knowledge_graph");
        PF_ENUM_TO_STRING(WorldScopeKind::Agreement, "agreement");
        PF_ENUM_TO_STRING(WorldScopeKind::Timeline, "timeline");
        PF_ENUM_TO_STRING(WorldScopeKind::TestOnly, "test_only");
        PF_ENUM_TO_STRING(WorldScopeKind::Unknown, "unknown");
    }
    return "unknown";
}

std::string toString(EffectOperationGroupKind kind) {
    switch (kind) {
        PF_ENUM_TO_STRING(EffectOperationGroupKind::Primary, "primary");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::Cost, "cost");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::SideEffect, "side_effect");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::Risk, "risk");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::Trigger, "trigger");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::Knowledge, "knowledge");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::Social, "social");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::Systemic, "systemic");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::Scheduled, "scheduled");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::TestOnly, "test_only");
        PF_ENUM_TO_STRING(EffectOperationGroupKind::Unknown, "unknown");
    }
    return "unknown";
}

std::string toString(OperationFailurePolicy policy) {
    switch (policy) {
        PF_ENUM_TO_STRING(OperationFailurePolicy::FailSpec, "fail_spec");
        PF_ENUM_TO_STRING(OperationFailurePolicy::SkipOperation, "skip_operation");
        PF_ENUM_TO_STRING(OperationFailurePolicy::ContinueWithWarning, "continue_with_warning");
        PF_ENUM_TO_STRING(OperationFailurePolicy::QueueForRetry, "queue_for_retry");
        PF_ENUM_TO_STRING(OperationFailurePolicy::TestOnly, "test_only");
        PF_ENUM_TO_STRING(OperationFailurePolicy::Unknown, "unknown");
    }
    return "unknown";
}

std::string toString(TemporalEffectKind kind) {
    switch (kind) {
        PF_ENUM_TO_STRING(TemporalEffectKind::Instant, "instant");
        PF_ENUM_TO_STRING(TemporalEffectKind::Delayed, "delayed");
        PF_ENUM_TO_STRING(TemporalEffectKind::Duration, "duration");
        PF_ENUM_TO_STRING(TemporalEffectKind::Periodic, "periodic");
        PF_ENUM_TO_STRING(TemporalEffectKind::UntilConditionMet, "until_condition_met");
        PF_ENUM_TO_STRING(TemporalEffectKind::TestOnly, "test_only");
        PF_ENUM_TO_STRING(TemporalEffectKind::Unknown, "unknown");
    }
    return "unknown";
}

std::string toString(TargetSelectionKind kind) {
    switch (kind) {
        PF_ENUM_TO_STRING(TargetSelectionKind::ExplicitRequest, "explicit_request");
        PF_ENUM_TO_STRING(TargetSelectionKind::ActorSelf, "actor_self");
        PF_ENUM_TO_STRING(TargetSelectionKind::NearestMatching, "nearest_matching");
        PF_ENUM_TO_STRING(TargetSelectionKind::AllInScope, "all_in_scope");
        PF_ENUM_TO_STRING(TargetSelectionKind::RandomInScope, "random_in_scope");
        PF_ENUM_TO_STRING(TargetSelectionKind::HighestThreat, "highest_threat");
        PF_ENUM_TO_STRING(TargetSelectionKind::LowestNeed, "lowest_need");
        PF_ENUM_TO_STRING(TargetSelectionKind::LinkedKnowledgeTarget, "linked_knowledge_target");
        PF_ENUM_TO_STRING(TargetSelectionKind::TestOnly, "test_only");
        PF_ENUM_TO_STRING(TargetSelectionKind::Unknown, "unknown");
    }
    return "unknown";
}

std::string toString(ExecutionValueSourceKind kind) {
    switch (kind) {
        PF_ENUM_TO_STRING(ExecutionValueSourceKind::Constant, "constant");
        PF_ENUM_TO_STRING(ExecutionValueSourceKind::RequestObjectKey, "request_object_key");
        PF_ENUM_TO_STRING(ExecutionValueSourceKind::RequestTargetKey, "request_target_key");
        PF_ENUM_TO_STRING(ExecutionValueSourceKind::RequestActorKey, "request_actor_key");
        PF_ENUM_TO_STRING(ExecutionValueSourceKind::EffectOutputKey, "effect_output_key");
        PF_ENUM_TO_STRING(ExecutionValueSourceKind::RuntimeExpression, "runtime_expression");
        PF_ENUM_TO_STRING(ExecutionValueSourceKind::TestOnly, "test_only");
        PF_ENUM_TO_STRING(ExecutionValueSourceKind::Unknown, "unknown");
    }
    return "unknown";
}

std::string toString(ExecutionFailureKind kind) {
    switch (kind) {
        PF_ENUM_TO_STRING(ExecutionFailureKind::SpecNotFound, "spec_not_found");
        PF_ENUM_TO_STRING(ExecutionFailureKind::ConditionNotMet, "condition_not_met");
        PF_ENUM_TO_STRING(ExecutionFailureKind::MissingObject, "missing_object");
        PF_ENUM_TO_STRING(ExecutionFailureKind::MissingTarget, "missing_target");
        PF_ENUM_TO_STRING(ExecutionFailureKind::InsufficientQuantity, "insufficient_quantity");
        PF_ENUM_TO_STRING(ExecutionFailureKind::StateNotMutable, "state_not_mutable");
        PF_ENUM_TO_STRING(ExecutionFailureKind::ForbiddenBySafety, "forbidden_by_safety");
        PF_ENUM_TO_STRING(ExecutionFailureKind::InvalidOperation, "invalid_operation");
        PF_ENUM_TO_STRING(ExecutionFailureKind::ExpressionFailed, "expression_failed");
        PF_ENUM_TO_STRING(ExecutionFailureKind::PartialExecutionRejected, "partial_execution_rejected");
        PF_ENUM_TO_STRING(ExecutionFailureKind::TestOnly, "test_only");
        PF_ENUM_TO_STRING(ExecutionFailureKind::Unknown, "unknown");
    }
    return "unknown";
}

std::string toString(AgentStepExecutionMode mode) {
    switch (mode) {
        PF_ENUM_TO_STRING(AgentStepExecutionMode::DryRun, "dry_run");
        PF_ENUM_TO_STRING(AgentStepExecutionMode::ExecuteOneStep, "execute_one_step");
        PF_ENUM_TO_STRING(AgentStepExecutionMode::ExecuteUntilBlocked, "execute_until_blocked");
        PF_ENUM_TO_STRING(AgentStepExecutionMode::ExecuteUntilGoalResolved, "execute_until_goal_resolved");
        PF_ENUM_TO_STRING(AgentStepExecutionMode::TestOnly, "test_only");
        PF_ENUM_TO_STRING(AgentStepExecutionMode::Unknown, "unknown");
    }
    return "unknown";
}

#undef PF_ENUM_TO_STRING

template <typename EnumT>
Result<EnumT> enumFromString(const std::string& value, const std::vector<std::pair<std::string, EnumT>>& values) {
    for (const auto& [key, item] : values) if (value == key) return Result<EnumT>::ok(item);
    return failValue<EnumT>("unknown enum string");
}

Result<EffectExecutionOpKind> effectExecutionOpKindFromString(const std::string& value) {
    return enumFromString<EffectExecutionOpKind>(value, {
        {"consume_object_quantity", EffectExecutionOpKind::ConsumeObjectQuantity}, {"add_object_quantity", EffectExecutionOpKind::AddObjectQuantity}, {"set_object_quantity", EffectExecutionOpKind::SetObjectQuantity},
        {"add_object_state_number", EffectExecutionOpKind::AddObjectStateNumber}, {"set_object_state_number", EffectExecutionOpKind::SetObjectStateNumber}, {"add_object_tag", EffectExecutionOpKind::AddObjectTag},
        {"remove_object_tag", EffectExecutionOpKind::RemoveObjectTag}, {"change_actor_need", EffectExecutionOpKind::ChangeActorNeed}, {"change_threat_level", EffectExecutionOpKind::ChangeThreatLevel},
        {"resolve_threat", EffectExecutionOpKind::ResolveThreat}, {"emit_world_event", EffectExecutionOpKind::EmitWorldEvent}, {"queue_followup_goal", EffectExecutionOpKind::QueueFollowupGoal},
        {"apply_status_effect", EffectExecutionOpKind::ApplyStatusEffect}, {"remove_status_effect", EffectExecutionOpKind::RemoveStatusEffect}, {"change_relationship", EffectExecutionOpKind::ChangeRelationship},
        {"create_agreement", EffectExecutionOpKind::CreateAgreement}, {"break_agreement", EffectExecutionOpKind::BreakAgreement}, {"change_reputation", EffectExecutionOpKind::ChangeReputation},
        {"unlock_capability", EffectExecutionOpKind::UnlockCapability}, {"change_population", EffectExecutionOpKind::ChangePopulation}, {"change_system_pressure", EffectExecutionOpKind::ChangeSystemPressure},
        {"change_world_rule", EffectExecutionOpKind::ChangeWorldRule}, {"propagate_knowledge", EffectExecutionOpKind::PropagateKnowledge}, {"corrupt_knowledge", EffectExecutionOpKind::CorruptKnowledge},
        {"change_knowledge_trust", EffectExecutionOpKind::ChangeKnowledgeTrust}, {"schedule_delayed_effect", EffectExecutionOpKind::ScheduleDelayedEffect}, {"start_periodic_effect", EffectExecutionOpKind::StartPeriodicEffect},
        {"stop_periodic_effect", EffectExecutionOpKind::StopPeriodicEffect}, {"test_only", EffectExecutionOpKind::TestOnly}});
}

Result<EffectExecutionTargetKind> effectExecutionTargetKindFromString(const std::string& value) {
    return enumFromString<EffectExecutionTargetKind>(value, {
        {"actor_self", EffectExecutionTargetKind::ActorSelf}, {"actor_target", EffectExecutionTargetKind::ActorTarget}, {"object_self", EffectExecutionTargetKind::ObjectSelf},
        {"object_target", EffectExecutionTargetKind::ObjectTarget}, {"location", EffectExecutionTargetKind::Location}, {"threat_target", EffectExecutionTargetKind::ThreatTarget},
        {"group_or_tribe", EffectExecutionTargetKind::GroupOrTribe}, {"runtime_key", EffectExecutionTargetKind::RuntimeKey}, {"inventory_scope", EffectExecutionTargetKind::InventoryScope},
        {"region", EffectExecutionTargetKind::Region}, {"world_system", EffectExecutionTargetKind::WorldSystem}, {"faction", EffectExecutionTargetKind::Faction},
        {"civilization", EffectExecutionTargetKind::Civilization}, {"megastructure", EffectExecutionTargetKind::Megastructure}, {"knowledge_graph", EffectExecutionTargetKind::KnowledgeGraph},
        {"agreement", EffectExecutionTargetKind::Agreement}, {"timeline", EffectExecutionTargetKind::Timeline}, {"test_only", EffectExecutionTargetKind::TestOnly}});
}

Result<ExecutionValueSourceKind> executionValueSourceKindFromString(const std::string& value) {
    return enumFromString<ExecutionValueSourceKind>(value, {{"constant", ExecutionValueSourceKind::Constant}, {"request_object_key", ExecutionValueSourceKind::RequestObjectKey}, {"request_target_key", ExecutionValueSourceKind::RequestTargetKey}, {"request_actor_key", ExecutionValueSourceKind::RequestActorKey}, {"effect_output_key", ExecutionValueSourceKind::EffectOutputKey}, {"runtime_expression", ExecutionValueSourceKind::RuntimeExpression}, {"test_only", ExecutionValueSourceKind::TestOnly}});
}

Result<ExecutionFailureKind> executionFailureKindFromString(const std::string& value) {
    return enumFromString<ExecutionFailureKind>(value, {{"spec_not_found", ExecutionFailureKind::SpecNotFound}, {"condition_not_met", ExecutionFailureKind::ConditionNotMet}, {"missing_object", ExecutionFailureKind::MissingObject}, {"missing_target", ExecutionFailureKind::MissingTarget}, {"insufficient_quantity", ExecutionFailureKind::InsufficientQuantity}, {"state_not_mutable", ExecutionFailureKind::StateNotMutable}, {"forbidden_by_safety", ExecutionFailureKind::ForbiddenBySafety}, {"invalid_operation", ExecutionFailureKind::InvalidOperation}, {"expression_failed", ExecutionFailureKind::ExpressionFailed}, {"partial_execution_rejected", ExecutionFailureKind::PartialExecutionRejected}, {"test_only", ExecutionFailureKind::TestOnly}});
}

Result<AgentStepExecutionMode> agentStepExecutionModeFromString(const std::string& value) {
    return enumFromString<AgentStepExecutionMode>(value, {{"dry_run", AgentStepExecutionMode::DryRun}, {"execute_one_step", AgentStepExecutionMode::ExecuteOneStep}, {"execute_until_blocked", AgentStepExecutionMode::ExecuteUntilBlocked}, {"execute_until_goal_resolved", AgentStepExecutionMode::ExecuteUntilGoalResolved}, {"test_only", AgentStepExecutionMode::TestOnly}});
}

Result<WorldScopeKind> worldScopeKindFromString(const std::string& value) {
    return enumFromString<WorldScopeKind>(value, {{"actor", WorldScopeKind::Actor}, {"inventory", WorldScopeKind::Inventory}, {"object", WorldScopeKind::Object}, {"location", WorldScopeKind::Location}, {"region", WorldScopeKind::Region}, {"threat", WorldScopeKind::Threat}, {"group_or_tribe", WorldScopeKind::GroupOrTribe}, {"faction", WorldScopeKind::Faction}, {"civilization", WorldScopeKind::Civilization}, {"world_system", WorldScopeKind::WorldSystem}, {"megastructure", WorldScopeKind::Megastructure}, {"knowledge_graph", WorldScopeKind::KnowledgeGraph}, {"agreement", WorldScopeKind::Agreement}, {"timeline", WorldScopeKind::Timeline}, {"test_only", WorldScopeKind::TestOnly}});
}

Result<EffectOperationGroupKind> effectOperationGroupKindFromString(const std::string& value) {
    return enumFromString<EffectOperationGroupKind>(value, {{"primary", EffectOperationGroupKind::Primary}, {"cost", EffectOperationGroupKind::Cost}, {"side_effect", EffectOperationGroupKind::SideEffect}, {"risk", EffectOperationGroupKind::Risk}, {"trigger", EffectOperationGroupKind::Trigger}, {"knowledge", EffectOperationGroupKind::Knowledge}, {"social", EffectOperationGroupKind::Social}, {"systemic", EffectOperationGroupKind::Systemic}, {"scheduled", EffectOperationGroupKind::Scheduled}, {"test_only", EffectOperationGroupKind::TestOnly}});
}

Result<OperationFailurePolicy> operationFailurePolicyFromString(const std::string& value) {
    return enumFromString<OperationFailurePolicy>(value, {{"fail_spec", OperationFailurePolicy::FailSpec}, {"skip_operation", OperationFailurePolicy::SkipOperation}, {"continue_with_warning", OperationFailurePolicy::ContinueWithWarning}, {"queue_for_retry", OperationFailurePolicy::QueueForRetry}, {"test_only", OperationFailurePolicy::TestOnly}});
}

Result<TemporalEffectKind> temporalEffectKindFromString(const std::string& value) {
    return enumFromString<TemporalEffectKind>(value, {{"instant", TemporalEffectKind::Instant}, {"delayed", TemporalEffectKind::Delayed}, {"duration", TemporalEffectKind::Duration}, {"periodic", TemporalEffectKind::Periodic}, {"until_condition_met", TemporalEffectKind::UntilConditionMet}, {"test_only", TemporalEffectKind::TestOnly}});
}

Result<TargetSelectionKind> targetSelectionKindFromString(const std::string& value) {
    return enumFromString<TargetSelectionKind>(value, {{"explicit_request", TargetSelectionKind::ExplicitRequest}, {"actor_self", TargetSelectionKind::ActorSelf}, {"nearest_matching", TargetSelectionKind::NearestMatching}, {"all_in_scope", TargetSelectionKind::AllInScope}, {"random_in_scope", TargetSelectionKind::RandomInScope}, {"highest_threat", TargetSelectionKind::HighestThreat}, {"lowest_need", TargetSelectionKind::LowestNeed}, {"linked_knowledge_target", TargetSelectionKind::LinkedKnowledgeTarget}, {"test_only", TargetSelectionKind::TestOnly}});
}

bool WorldScopeRef::empty() const {
    return kind == WorldScopeKind::Unknown && scope_key.empty() && parent_scope_key.empty() && safe_tags.empty();
}

Result<void> WorldScopeRef::validateBasic() const {
    if (empty()) return Result<void>::ok();
    if (kind == WorldScopeKind::Unknown) return fail("world scope kind unknown");
    if (forbiddenText(scope_key) || forbiddenText(parent_scope_key) || forbiddenText(safe_tags)) return fail("world scope contains forbidden text");
    return Result<void>::ok();
}

Result<void> TemporalEffectPolicy::validateBasic() const {
    if (kind == TemporalEffectKind::Unknown) return fail("temporal effect kind unknown");
    if (kind == TemporalEffectKind::Instant && (delay_ticks != 0 || duration_ticks != 0 || period_ticks != 0)) return fail("instant temporal effect has tick values");
    if (kind == TemporalEffectKind::Delayed && delay_ticks == 0) return fail("delayed temporal effect missing delay");
    if (kind == TemporalEffectKind::Duration && duration_ticks == 0) return fail("duration temporal effect missing duration");
    if (kind == TemporalEffectKind::Periodic && period_ticks == 0) return fail("periodic temporal effect missing period");
    if (kind == TemporalEffectKind::UntilConditionMet && until_condition_key.empty()) return fail("temporal effect missing until condition");
    if (forbiddenText(until_condition_key)) return fail("temporal effect contains forbidden text");
    return Result<void>::ok();
}

Result<void> TargetSelectionSpec::validateBasic() const {
    if (kind == TargetSelectionKind::Unknown) return fail("target selection kind unknown");
    if (max_targets == 0) return fail("target selection max targets zero");
    auto scope_valid = scope.validateBasic();
    if (scope_valid.is_error()) return scope_valid;
    if (forbiddenText(explicit_target_key) || forbiddenText(required_tags)) return fail("target selection contains forbidden text");
    return Result<void>::ok();
}

bool KnowledgeEffectPayload::empty() const {
    return knowledge_key.empty() && subject_key.empty() && relation_key.empty() && trust_delta == 0.0 && propagation_tags.empty();
}

Result<void> KnowledgeEffectPayload::validateBasic() const {
    if (empty()) return Result<void>::ok();
    if (!std::isfinite(trust_delta)) return fail("knowledge payload trust delta invalid");
    if (forbiddenText(knowledge_key) || forbiddenText(subject_key) || forbiddenText(relation_key) || forbiddenText(propagation_tags)) return fail("knowledge payload contains forbidden text");
    return Result<void>::ok();
}

bool RelationshipEffectPayload::empty() const {
    return source_actor_key.empty() && target_actor_key.empty() && relationship_key.empty() && relationship_delta == 0.0 && reputation_delta == 0.0 && agreement_key.empty();
}

Result<void> RelationshipEffectPayload::validateBasic() const {
    if (empty()) return Result<void>::ok();
    if (!std::isfinite(relationship_delta) || !std::isfinite(reputation_delta)) return fail("relationship payload delta invalid");
    if (forbiddenText(source_actor_key) || forbiddenText(target_actor_key) || forbiddenText(relationship_key) || forbiddenText(agreement_key)) return fail("relationship payload contains forbidden text");
    return Result<void>::ok();
}

bool WorldRuleEffectPayload::empty() const {
    return rule_key.empty() && system_key.empty() && pressure_delta == 0.0 && capability_key.empty() && safe_summary_zh_cn.empty();
}

Result<void> WorldRuleEffectPayload::validateBasic() const {
    if (empty()) return Result<void>::ok();
    if (!std::isfinite(pressure_delta)) return fail("world rule payload pressure invalid");
    if (forbiddenText(rule_key) || forbiddenText(system_key) || forbiddenText(capability_key) || forbiddenText(safe_summary_zh_cn)) return fail("world rule payload contains forbidden text");
    return Result<void>::ok();
}

Result<void> RandomExecutionPolicy::validateBasic() const {
    if (!std::isfinite(chance) || chance < 0.0 || chance > 1.0) return fail("random execution chance invalid");
    if (enabled && random_stream_key.empty()) return fail("random execution stream key empty");
    if (forbiddenText(random_stream_key) || forbiddenText(outcome_keys)) return fail("random execution contains forbidden text");
    return Result<void>::ok();
}

bool ScheduledEffectRef::empty() const {
    return schedule_id.empty() && effect_key.empty() && scope.empty() && due_tick == 0 && repeat_period_ticks == 0;
}

Result<void> ScheduledEffectRef::validateBasic() const {
    if (empty()) return Result<void>::ok();
    if (schedule_id.empty() || effect_key.empty()) return fail("scheduled effect required field empty");
    auto scope_valid = scope.validateBasic();
    if (scope_valid.is_error()) return scope_valid;
    if (forbiddenText(schedule_id) || forbiddenText(effect_key)) return fail("scheduled effect contains forbidden text");
    return Result<void>::ok();
}

Result<void> EffectExecutionOperation::validateBasic() const {
    if (operation_id.empty() || safe_summary_zh_cn.empty()) return fail("execution operation required field empty");
    if (op_kind == EffectExecutionOpKind::Unknown || target_kind == EffectExecutionTargetKind::Unknown || key_source == ExecutionValueSourceKind::Unknown || group_kind == EffectOperationGroupKind::Unknown || failure_policy == OperationFailurePolicy::Unknown) return fail("execution operation enum unknown");
    if (forbiddenText(operation_id) || forbiddenText(runtime_key) || forbiddenText(effect_output_key) || forbiddenText(state_key) || forbiddenText(safe_summary_zh_cn) || forbiddenText(trace_keys)) return fail("execution operation contains forbidden text");
    if (!std::isfinite(number_value)) return fail("execution operation number invalid");
    if ((op_kind == EffectExecutionOpKind::ConsumeObjectQuantity || op_kind == EffectExecutionOpKind::AddObjectQuantity) && quantity_value == 0) return fail("execution operation quantity zero");
    if ((op_kind == EffectExecutionOpKind::AddObjectStateNumber || op_kind == EffectExecutionOpKind::SetObjectStateNumber || op_kind == EffectExecutionOpKind::ChangeActorNeed) && state_key.empty()) return fail("execution operation state key empty");
    auto scope_valid = world_scope.validateBasic();
    if (scope_valid.is_error()) return scope_valid;
    auto target_valid = target_selection.validateBasic();
    if (target_valid.is_error()) return target_valid;
    auto temporal_valid = temporal_policy.validateBasic();
    if (temporal_valid.is_error()) return temporal_valid;
    auto knowledge_valid = knowledge_payload.validateBasic();
    if (knowledge_valid.is_error()) return knowledge_valid;
    auto relationship_valid = relationship_payload.validateBasic();
    if (relationship_valid.is_error()) return relationship_valid;
    auto rule_valid = world_rule_payload.validateBasic();
    if (rule_valid.is_error()) return rule_valid;
    auto random_valid = random_policy.validateBasic();
    if (random_valid.is_error()) return random_valid;
    auto scheduled_valid = scheduled_effect.validateBasic();
    if (scheduled_valid.is_error()) return scheduled_valid;
    return Result<void>::ok();
}

Result<void> EffectExecutionSpec::validateBasic() const {
    if (spec_id.empty() || effect_key.empty() || safe_summary_zh_cn.empty() || source_config_id.empty() || version.empty()) return fail("execution spec required field empty");
    if (!isSnakeKey(effect_key)) return fail("execution spec effect key invalid");
    if (operations.empty()) return fail("execution spec operations empty");
    if (forbiddenText(spec_id) || forbiddenText(effect_key) || forbiddenText(safe_summary_zh_cn) || forbiddenText(source_config_id) || forbiddenText(version) || forbiddenText(trace_keys) || forbiddenText(unsupported_features)) return fail("execution spec contains forbidden text");
    auto scope_valid = default_scope.validateBasic();
    if (scope_valid.is_error()) return scope_valid;
    auto temporal_valid = temporal_policy.validateBasic();
    if (temporal_valid.is_error()) return temporal_valid;
    auto random_valid = random_policy.validateBasic();
    if (random_valid.is_error()) return random_valid;
    for (const auto& scheduled : scheduled_effects) {
        auto valid = scheduled.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& ref : condition_refs) {
        auto valid = ref.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& operation : operations) {
        auto valid = operation.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> WorldExecutionRequest::validateBasic() const {
    if (request_id.empty() || actor_key.empty() || effect_key.empty()) return fail("world execution request required field empty");
    if (forbiddenText(request_id) || forbiddenText(actor_key) || forbiddenText(effect_key) || forbiddenText(object_key) || forbiddenText(target_object_key) || forbiddenText(target_threat_key) || forbiddenText(trace_keys)) return fail("world execution request contains forbidden text");
    return Result<void>::ok();
}

Result<void> WorldExecutionResult::validateBasic() const {
    if (safe_summary_zh_cn.empty()) return fail("world execution result summary empty");
    if (ok && failure_kind != ExecutionFailureKind::Unknown) return fail("world execution success has failure");
    if (!ok && failure_kind == ExecutionFailureKind::Unknown) return fail("world execution failure missing kind");
    if (!ok && !changes.empty()) return fail("world execution failure has changes");
    if (forbiddenText(safe_summary_zh_cn) || forbiddenText(trace_keys) || forbiddenText(spec_id)) return fail("world execution result contains forbidden text");
    for (const auto& change : changes) {
        auto valid = change.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> AgentStepExecutionRequest::validateBasic() const {
    if (request_id.empty() || actor_key.empty() || plan_id.empty()) return fail("agent step execution request required field empty");
    if (mode == AgentStepExecutionMode::Unknown) return fail("agent step execution mode unknown");
    if (mode != AgentStepExecutionMode::DryRun && mode != AgentStepExecutionMode::ExecuteOneStep && mode != AgentStepExecutionMode::TestOnly) return fail("agent step execution mode not implemented in p41");
    if (forbiddenText(request_id) || forbiddenText(actor_key) || forbiddenText(plan_id) || forbiddenText(source_knowledge_id) || forbiddenText(trace_keys)) return fail("agent step execution request contains forbidden text");
    auto valid = step.validateBasic();
    if (valid.is_error()) return valid;
    return Result<void>::ok();
}

Result<void> AgentStepExecutionResult::validateBasic() const {
    if (consumed_step_id.empty()) return fail("agent step result step id empty");
    if (!ok && blocked_reason_zh_cn.empty()) return fail("agent step blocked reason empty");
    if (forbiddenText(blocked_reason_zh_cn) || forbiddenText(consumed_step_id) || forbiddenText(trace_keys)) return fail("agent step result contains forbidden text");
    auto valid = world_result.validateBasic();
    if (valid.is_error()) return valid;
    return Result<void>::ok();
}

std::vector<EffectExecutionSpec> builtInEffectExecutionSpecs() {
    return {
        spec("spec.restore_hunger", "restore_hunger", {}, {
            op("consume", EffectExecutionOpKind::ConsumeObjectQuantity, EffectExecutionTargetKind::ObjectSelf, "你消耗了一个可食用物，它在这个地方少了。", 1, 0.0, {}, ExecutionValueSourceKind::RequestObjectKey),
            op("hunger", EffectExecutionOpKind::ChangeActorNeed, EffectExecutionTargetKind::ActorSelf, "饥饿感缓解了一些。", 0, -30.0, "hunger")}, "食物缓解了饥饿。"),
        spec("spec.poison", "poison", {}, {
            op("consume", EffectExecutionOpKind::ConsumeObjectQuantity, EffectExecutionTargetKind::ObjectSelf, "你消耗了一个可疑食物，它在这个地方少了。", 1, 0.0, {}, ExecutionValueSourceKind::RequestObjectKey),
            op("poison", EffectExecutionOpKind::ChangeActorNeed, EffectExecutionTargetKind::ActorSelf, "身体状态变差了，这次经验会影响之后的判断。", 0, -20.0, "health")}, "这次尝试让身体变差。"),
        spec("spec.cut_wood", "cut_wood", {condition("condition:test:eq:object.quantity.wood.gte.1"), condition("condition:test:eq:object.quantity.axe.gte.1"), condition("condition:test:eq:object.state.axe.sharpness.gte.1")}, {
            op("wood", EffectExecutionOpKind::ConsumeObjectQuantity, EffectExecutionTargetKind::RuntimeKey, "木头被斧头砍开，变成了可以继续加工的材料。", 1, 0.0, {}, ExecutionValueSourceKind::Constant, "wood"),
            op("processed", EffectExecutionOpKind::AddObjectStateNumber, EffectExecutionTargetKind::RuntimeKey, "处理好的木材增加了。当前可用于制作火把。", 0, 1.0, "processed", ExecutionValueSourceKind::Constant, "wood"),
            op("sharpness", EffectExecutionOpKind::AddObjectStateNumber, EffectExecutionTargetKind::RuntimeKey, "斧头的锋利度下降了。", 0, -1.0, "sharpness", ExecutionValueSourceKind::Constant, "axe")}, "木头被处理成可用材料。"),
        spec("spec.restore_sharpness", "restore_sharpness", {condition("condition:test:eq:object.quantity.whetstone.gte.1"), condition("condition:test:eq:object.quantity.axe.gte.1")}, {
            op("sharpness", EffectExecutionOpKind::SetObjectStateNumber, EffectExecutionTargetKind::RuntimeKey, "斧头被磨石重新打磨，锋利度恢复了。", 0, 3.0, "sharpness", ExecutionValueSourceKind::Constant, "axe")}, "工具状态被恢复。"),
        spec("spec.tool_dull", "tool_dull", {}, {
            op("dull", EffectExecutionOpKind::SetObjectStateNumber, EffectExecutionTargetKind::RuntimeKey, "斧头太钝了，砍不动木头。你需要先打磨它。", 0, 0.0, "sharpness", ExecutionValueSourceKind::Constant, "axe")}, "工具状态变钝。"),
        spec("spec.ignite_fire", "ignite_fire", {condition("condition:test:eq:object.quantity.dry_grass.gte.1")}, {
            op("grass", EffectExecutionOpKind::ConsumeObjectQuantity, EffectExecutionTargetKind::RuntimeKey, "干草被火种点燃，一处小火堆出现了。", 1, 0.0, {}, ExecutionValueSourceKind::Constant, "dry_grass"),
            op("camp_fire", EffectExecutionOpKind::AddObjectQuantity, EffectExecutionTargetKind::RuntimeKey, "火光照亮了周围，火堆现在可以被其他生物观察到。", 1, 0.0, {}, ExecutionValueSourceKind::Constant, "camp_fire"),
            op("beast_observed_fire", EffectExecutionOpKind::SetObjectStateNumber, EffectExecutionTargetKind::RuntimeKey, "火光让树林里的影子变得犹豫。野生生物已经观察到火。", 0, 1.0, "observed_fire", ExecutionValueSourceKind::Constant, "beast_shadow")}, "火源被点燃。"),
        spec("spec.make_torch", "make_torch", {condition("condition:test:eq:object.quantity.wood_processed.gte.1"), condition("condition:test:eq:object.quantity.camp_fire.gte.1")}, {
            op("processed", EffectExecutionOpKind::AddObjectStateNumber, EffectExecutionTargetKind::RuntimeKey, "你消耗了一份处理好的木材。", 0, -1.0, "processed", ExecutionValueSourceKind::Constant, "wood"),
            op("torch", EffectExecutionOpKind::AddObjectQuantity, EffectExecutionTargetKind::RuntimeKey, "你把处理过的木头和火源组合起来，做出了一支火把。", 1, 0.0, {}, ExecutionValueSourceKind::Constant, "torch")}, "火把被制作出来。"),
        spec("spec.repel_beast", "repel_beast", {condition("condition:test:eq:object.quantity.torch.gte.1")}, {
            op("repel", EffectExecutionOpKind::ResolveThreat, EffectExecutionTargetKind::ThreatTarget, "火把的光和热逼退了靠近的野兽。树林里的低吼远去了。", 0, 0.0, {}, ExecutionValueSourceKind::Constant, "beast_shadow")}, "威胁被火把驱退。"),
        spec("spec.build_house", "build_house", {condition("condition:test:eq:object.quantity.wood_processed.gte.3")}, {
            op("wood_processed", EffectExecutionOpKind::ConsumeObjectQuantity, EffectExecutionTargetKind::RuntimeKey, "处理好的木材被用于建造庇护所。", 3, 0.0, {}, ExecutionValueSourceKind::Constant, "wood_processed"),
            op("house", EffectExecutionOpKind::AddObjectQuantity, EffectExecutionTargetKind::RuntimeKey, "新的房子提供了更多庇护容量。", 1, 0.0, {}, ExecutionValueSourceKind::Constant, "house")}, "房子被建造出来。"),
        spec("spec.provide_warmth", "provide_warmth", {}, {
            op("warmth", EffectExecutionOpKind::EmitWorldEvent, EffectExecutionTargetKind::ActorSelf, "温暖来源改善了当前处境。")}, "温暖来源产生了作用。"),
        spec("spec.no_visible_effect", "no_visible_effect", {}, {
            op("consume", EffectExecutionOpKind::ConsumeObjectQuantity, EffectExecutionTargetKind::ObjectSelf, "你消耗了一个对象，它在这个地方少了。", 1, 0.0, {}, ExecutionValueSourceKind::RequestObjectKey),
            op("no_visible", EffectExecutionOpKind::EmitWorldEvent, EffectExecutionTargetKind::ActorSelf, "暂时没有明显变化，但这次尝试仍然进入经验记录。")}, "暂时没有明显变化。"),
        spec("spec.use_hint", "use_hint", {}, {
            op("inspect_use", EffectExecutionOpKind::EmitWorldEvent, EffectExecutionTargetKind::ActorSelf, "你摸索了对象的用途，它没有立刻改变世界，但这次尝试被记录为可学习经验。")}, "这次摸索被记录为经验。")};
}

EffectExecutionSpecRegistry::EffectExecutionSpecRegistry() : EffectExecutionSpecRegistry(true) {}

EffectExecutionSpecRegistry::EffectExecutionSpecRegistry(bool load_builtins) {
    if (load_builtins) {
        for (const auto& item : builtInEffectExecutionSpecs()) {
            specs_by_effect_key_[item.effect_key] = item;
        }
    }
}

Result<void> EffectExecutionSpecRegistry::registerSpec(const EffectExecutionSpec& spec_value) {
    auto valid = spec_value.validateBasic();
    if (valid.is_error()) return valid;
    if (specs_by_effect_key_.find(spec_value.effect_key) != specs_by_effect_key_.end()) return fail("execution spec duplicate effect key");
    specs_by_effect_key_[spec_value.effect_key] = spec_value;
    return Result<void>::ok();
}

Result<EffectExecutionSpec> EffectExecutionSpecRegistry::findByEffectKey(const std::string& effect_key) const {
    auto it = specs_by_effect_key_.find(effect_key);
    if (it == specs_by_effect_key_.end()) return Result<EffectExecutionSpec>::fail(makeError(ErrorCode::knowledge_not_found, "execution spec not found"));
    return Result<EffectExecutionSpec>::ok(it->second);
}

Result<std::vector<EffectExecutionSpec>> EffectExecutionSpecRegistry::allSpecs() const {
    std::vector<EffectExecutionSpec> specs;
    for (const auto& [_, item] : specs_by_effect_key_) specs.push_back(item);
    std::sort(specs.begin(), specs.end(), [](const auto& lhs, const auto& rhs) { return lhs.effect_key < rhs.effect_key; });
    return Result<std::vector<EffectExecutionSpec>>::ok(std::move(specs));
}

Result<void> EffectExecutionSpecRegistry::validateAgainst(const EffectSemanticsRegistry& semantics) const {
    std::set<std::string> seen;
    for (const auto& [effect_key, item] : specs_by_effect_key_) {
        if (!seen.insert(effect_key).second) return fail("execution spec duplicate effect key");
        auto valid = item.validateBasic();
        if (valid.is_error()) return valid;
        auto semantic = semantics.findByEffectKey(effect_key);
        if (semantic.is_error()) return Result<void>::fail(semantic.errors());
    }
    return Result<void>::ok();
}

Result<std::vector<PlanPrecondition>> ExecutionConditionResolver::preconditionsForEffect(
    const std::string& effect_key,
    const EffectExecutionSpecRegistry& execution_specs,
    const std::vector<pathfinder::reaction::ObjectReactionRule>& reaction_rules) const {
    auto spec_result = execution_specs.findByEffectKey(effect_key);
    if (spec_result.is_error()) return Result<std::vector<PlanPrecondition>>::fail(spec_result.errors());
    std::vector<PlanPrecondition> resolved;
    for (const auto& ref : spec_result.value().condition_refs) {
        const auto& key = ref.inline_canonical_key;
        const std::string prefix = "condition:test:eq:";
        if (key.rfind(prefix + "object.quantity.", 0) == 0) {
            auto tail = key.substr((prefix + "object.quantity.").size());
            auto pos = tail.rfind(".gte.");
            if (pos != std::string::npos) resolved.push_back(quantityPrecondition(tail.substr(0, pos), std::stoi(tail.substr(pos + 5)), true));
        } else if (key.rfind(prefix + "object.state.", 0) == 0) {
            auto tail = key.substr((prefix + "object.state.").size());
            auto pos = tail.rfind(".gte.");
            auto object_state = pos == std::string::npos ? tail : tail.substr(0, pos);
            auto dot = object_state.find('.');
            if (dot != std::string::npos && pos != std::string::npos) resolved.push_back(statePrecondition(object_state.substr(0, dot), object_state.substr(dot + 1), std::stoi(tail.substr(pos + 5)), true));
        }
    }
    ReactionPlanningAdapter adapter;
    auto reaction_resolved = adapter.preconditionsForEffect(effect_key, reaction_rules);
    if (reaction_resolved.is_error()) return reaction_resolved;
    resolved.insert(resolved.end(), reaction_resolved.value().begin(), reaction_resolved.value().end());
    std::vector<PlanPrecondition> deduped;
    for (const auto& value : resolved) {
        const auto duplicate = std::find_if(deduped.begin(), deduped.end(), [&](const auto& item) {
            return item.missing_domain == value.missing_domain && item.missing_key == value.missing_key && item.required_value == value.required_value;
        });
        if (duplicate == deduped.end()) deduped.push_back(value);
    }
    return Result<std::vector<PlanPrecondition>>::ok(std::move(deduped));
}

Result<WorldExecutionResult> WorldEffectExecutor::execute(
    const WorldSnapshot& snapshot,
    const WorldExecutionRequest& request,
    const EffectExecutionSpecRegistry& specs,
    const ConditionExpressionEvaluator& evaluator) const {
    auto snapshot_valid = snapshot.validateBasic();
    if (snapshot_valid.is_error()) return Result<WorldExecutionResult>::fail(snapshot_valid.errors());
    auto request_valid = request.validateBasic();
    if (request_valid.is_error()) return Result<WorldExecutionResult>::fail(request_valid.errors());
    auto spec_result = specs.findByEffectKey(request.effect_key);
    if (spec_result.is_error()) return Result<WorldExecutionResult>::ok(failedExecution(ExecutionFailureKind::SpecNotFound, summaryForFailure(ExecutionFailureKind::SpecNotFound), {"p41.spec_missing:" + request.effect_key}));
    const auto spec_value = spec_result.value();
    auto spec_valid = spec_value.validateBasic();
    if (spec_valid.is_error()) return Result<WorldExecutionResult>::fail(spec_valid.errors());

    ConditionEvaluationContext context;
    context.context_type = "p41_world_execution";
    context.safe_context_keys = safeFactsFromSnapshot(snapshot);
    for (const auto& ref : spec_value.condition_refs) {
        auto evaluated = evaluator.evaluate(ref, context);
        if (evaluated.is_error()) return Result<WorldExecutionResult>::ok(failedExecution(ExecutionFailureKind::ExpressionFailed, summaryForFailure(ExecutionFailureKind::ExpressionFailed), spec_value.trace_keys));
        if (!evaluated.value().matched) return Result<WorldExecutionResult>::ok(failedExecution(ExecutionFailureKind::ConditionNotMet, summaryForFailure(ExecutionFailureKind::ConditionNotMet), spec_value.trace_keys));
    }
    std::vector<WorldChange> changes;
    const auto prefix = "world.change." + std::to_string(snapshot.turn_index) + "." + request.effect_key + ".";
    for (const auto& operation : spec_value.operations) {
        const auto failure = validateOperationAgainstSnapshot(operation, request, snapshot);
        if (failure != ExecutionFailureKind::Unknown) return Result<WorldExecutionResult>::ok(failedExecution(failure, summaryForFailure(failure), operation.trace_keys));
        if (!isExecutableOperationKind(operation.op_kind)) {
            if (!request.dry_run) return Result<WorldExecutionResult>::ok(failedExecution(ExecutionFailureKind::InvalidOperation, "这个长期效果目前只支持 dry-run 验证，尚未接入真实世界结算。", operation.trace_keys));
            continue;
        }
        if (!request.dry_run) changes.push_back(changeForOperation(operation, request, prefix));
    }
    WorldExecutionResult result;
    result.ok = true;
    result.executed = !request.dry_run;
    result.failure_kind = ExecutionFailureKind::Unknown;
    result.changes = std::move(changes);
    result.safe_summary_zh_cn = request.dry_run ? "执行条件已满足，可以执行。" : spec_value.safe_summary_zh_cn;
    result.trace_keys = spec_value.trace_keys;
    result.spec_id = spec_value.spec_id;
    auto valid = result.validateBasic();
    if (valid.is_error()) return Result<WorldExecutionResult>::fail(valid.errors());
    return Result<WorldExecutionResult>::ok(std::move(result));
}

Result<AgentStepExecutionResult> AgentPlanStepExecutor::executeStep(
    const WorldSnapshot& snapshot,
    const AgentStepExecutionRequest& request,
    const EffectExecutionSpecRegistry& specs) const {
    auto valid = request.validateBasic();
    if (valid.is_error()) return Result<AgentStepExecutionResult>::fail(valid.errors());
    WorldExecutionRequest world_request;
    world_request.request_id = request.request_id + ".world";
    world_request.actor_key = request.actor_key;
    world_request.effect_key = request.step.effect_key;
    world_request.object_key = request.step.object_key;
    world_request.target_object_key = request.step.target_key.value_or("");
    world_request.target_threat_key = request.step.target_key.value_or("");
    world_request.dry_run = request.mode == AgentStepExecutionMode::DryRun;
    world_request.trace_keys = request.trace_keys;
    ConditionExpressionEvaluator evaluator;
    WorldEffectExecutor executor;
    auto world_result = executor.execute(snapshot, world_request, specs, evaluator);
    if (world_result.is_error()) return Result<AgentStepExecutionResult>::fail(world_result.errors());
    AgentStepExecutionResult result;
    result.ok = world_result.value().ok;
    result.executed = world_result.value().executed;
    result.world_result = world_result.value();
    result.blocked_reason_zh_cn = world_result.value().ok ? "" : world_result.value().safe_summary_zh_cn;
    result.should_replan = !world_result.value().ok;
    result.consumed_step_id = request.step.step_id;
    result.trace_keys = world_result.value().trace_keys;
    auto result_valid = result.validateBasic();
    if (result_valid.is_error()) return Result<AgentStepExecutionResult>::fail(result_valid.errors());
    return Result<AgentStepExecutionResult>::ok(std::move(result));
}

Result<AgentAutonomyResult> AgentExecutionCoordinator::executePlan(
    const WorldSnapshot& snapshot,
    const AgentPlan& plan,
    AgentStepExecutionMode mode,
    const EffectExecutionSpecRegistry& specs) const {
    auto plan_valid = plan.validateBasic();
    if (plan_valid.is_error()) return Result<AgentAutonomyResult>::fail(plan_valid.errors());
    AgentAutonomyResult result;
    result.agent_actor_key = plan.actor_key;
    result.required_knowledge_effect_key = plan.steps.empty() ? "" : plan.steps.front().effect_key;
    result.skip_reason = InteractionFailureKind::ConditionNotMet;
    if (plan.blocked || plan.steps.empty()) {
        result.action_kind = AgentAutonomyActionKind::None;
        result.executed = false;
        result.summary_zh_cn = plan.blocking_reason_zh_cn.value_or("计划被阻塞。");
        return Result<AgentAutonomyResult>::ok(std::move(result));
    }
    const auto& step = plan.steps.front();
    AgentStepExecutionRequest step_request;
    step_request.request_id = "agent.step." + step.step_id;
    step_request.actor_key = plan.actor_key;
    step_request.plan_id = plan.plan_id;
    step_request.step = step;
    step_request.mode = mode;
    step_request.trace_keys = plan.trace_keys;
    AgentPlanStepExecutor step_executor;
    auto step_result = step_executor.executeStep(snapshot, step_request, specs);
    if (step_result.is_error()) return Result<AgentAutonomyResult>::fail(step_result.errors());
    result.action_kind = step_result.value().ok ? actionKindForStep(step) : AgentAutonomyActionKind::None;
    result.executed = step_result.value().executed;
    result.used_object_key = step.object_key;
    result.target_key = step.target_key.value_or(plan.goal.target_key.value_or(""));
    result.summary_zh_cn = step_result.value().ok ? step.explanation_zh_cn : step_result.value().blocked_reason_zh_cn;
    if (step_result.value().ok) result.skip_reason.reset();
    result.changes = step_result.value().world_result.changes;
    result.trace_keys = step_result.value().trace_keys;
    return Result<AgentAutonomyResult>::ok(std::move(result));
}

} // namespace pathfinder::agent_reasoning
