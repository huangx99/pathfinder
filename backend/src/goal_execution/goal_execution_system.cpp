#include "pathfinder/goal_execution/goal_execution_system.h"

#include "pathfinder/foundation/error.h"
#include "pathfinder/h5_dialog/dialog_types.h"
#include "pathfinder/reaction/reaction_fixtures.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace pathfinder::goal_execution {
namespace {
using pathfinder::agent_reasoning::AgentGoal;
using pathfinder::agent_reasoning::AgentGoalKind;
using pathfinder::agent_reasoning::AgentPlan;
using pathfinder::agent_reasoning::AgentPlanStep;
using pathfinder::agent_reasoning::PlanStepKind;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::h5_dialog::DialogActionKind;
using pathfinder::reaction::ObjectReactionRule;
using pathfinder::reaction::ReactionObjectRole;
using pathfinder::reaction::ReactionOutputKind;
using pathfinder::world_interaction::WorldInteractionInput;
using pathfinder::world_interaction::WorldObjectInstance;
using pathfinder::world_interaction::WorldObjectInstanceKind;
using pathfinder::world_interaction::WorldSnapshot;

Result<void> okVoid() { return Result<void>::ok(); }

Result<void> fail(const std::string& message) {
    return Result<void>::fail(makeError(ErrorCode::validation_failed, message));
}

template <typename T>
Result<T> failValue(const std::string& message) {
    return Result<T>::fail(makeError(ErrorCode::validation_failed, message));
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

GoalFrame* findFrame(ExecutionContext& context, const std::string& frame_id) {
    auto it = std::find_if(context.goal_stack.begin(), context.goal_stack.end(), [&](const auto& frame) { return frame.frame_id == frame_id; });
    return it == context.goal_stack.end() ? nullptr : &*it;
}

const GoalFrame* findFrame(const ExecutionContext& context, const std::string& frame_id) {
    auto it = std::find_if(context.goal_stack.begin(), context.goal_stack.end(), [&](const auto& frame) { return frame.frame_id == frame_id; });
    return it == context.goal_stack.end() ? nullptr : &*it;
}

const WorldObjectInstance* findObject(const WorldSnapshot& snapshot, const std::string& key) {
    auto it = snapshot.objects_by_key.find(key);
    return it == snapshot.objects_by_key.end() ? nullptr : &it->second;
}

int quantityOf(const WorldSnapshot& snapshot, const std::string& key) {
    const auto* object = findObject(snapshot, key);
    return object == nullptr ? 0 : object->quantity;
}

double numericState(const WorldObjectInstance* object, const std::string& key) {
    if (object == nullptr) return 0.0;
    auto it = object->numeric_states.find(key);
    return it == object->numeric_states.end() ? 0.0 : it->second;
}

// TODO(P41-config-catalog): This is a P40 minimum bridge for P28 fixture definitions.
// Replace it with catalog/config-backed object definition lookup before large-scale content expansion.
std::string productKey(const std::string& definition_id) {
    if (definition_id == "def_torch") return "torch";
    if (definition_id == "def_wood_processed") return "wood_processed";
    if (definition_id == "def_fire_source") return "camp_fire";
    if (definition_id == "def_raw_wood") return "wood";
    if (definition_id == "def_axe") return "axe";
    if (definition_id == "def_whetstone") return "whetstone";
    return definition_id;
}

// TODO(P41-config-catalog): This maps P28 fixture input patterns for the P40 bridge only.
// New content should eventually resolve required objects from reaction/object config packs, not this hardcoded list.
std::string patternObjectKey(const pathfinder::reaction::ReactionObjectPattern& pattern) {
    if (pattern.definition_id) return productKey(pattern.definition_id->value());
    if (contains(pattern.required_tag_keys, "fire_source")) return "camp_fire";
    if (contains(pattern.required_tag_keys, "branch_like") || contains(pattern.required_tag_keys, "combustible")) return "wood_processed";
    if (contains(pattern.required_tag_keys, "wood_material")) return "wood";
    if (contains(pattern.required_tag_keys, "cutting_tool")) return "axe";
    if (contains(pattern.required_tag_keys, "sharpening_tool")) return "whetstone";
    if (contains(pattern.required_tag_keys, "water_source")) return "water";
    return "unknown_material";
}

ActionDriverKind driverKindForStep(const AgentPlanStep& step) {
    const auto has_semantic = [&](pathfinder::agent_reasoning::EffectSemanticKind kind) {
        return std::any_of(step.expected_semantics.begin(), step.expected_semantics.end(), [&](const auto& semantics) { return semantics.semantic_kind == kind; });
    };
    const auto targets_kind = [&](const std::string& target_kind) {
        return std::any_of(step.expected_semantics.begin(), step.expected_semantics.end(), [&](const auto& semantics) {
            return semantics.required_target_kind.has_value() && *semantics.required_target_kind == target_kind;
        });
    };
    if (step.kind == PlanStepKind::BuildStructure || has_semantic(pathfinder::agent_reasoning::EffectSemanticKind::CapabilityDelta)) return ActionDriverKind::BuildStructure;
    if (step.kind == PlanStepKind::RestoreTool || has_semantic(pathfinder::agent_reasoning::EffectSemanticKind::ObjectStateDelta)) return ActionDriverKind::SharpenTool;
    if (has_semantic(pathfinder::agent_reasoning::EffectSemanticKind::ThreatDelta) || targets_kind("threat")) return ActionDriverKind::CounterThreat;
    if (step.action_key == "eat") return ActionDriverKind::Eat;
    if (step.action_key == "move") return ActionDriverKind::MoveTo;
    if (step.action_key == "chop") return ActionDriverKind::ChopWood;
    if (step.kind == PlanStepKind::PrepareObject) return ActionDriverKind::UseObject;
    return ActionDriverKind::Gather;
}

InternalBlocker blocker(InternalBlockerKind kind, const DriverTickInput& input, std::string missing_key, std::string required_value, std::string summary, std::optional<AgentGoalKind> goal_kind = std::nullopt) {
    InternalBlocker value;
    value.blocker_id = "blocker." + input.driver_state.driver_state_id + "." + toString(kind) + "." + missing_key;
    value.kind = kind;
    value.actor_key = input.actor_key;
    value.source_driver_state_id = input.driver_state.driver_state_id;
    value.missing_key = std::move(missing_key);
    value.required_value = std::move(required_value);
    value.can_generate_subgoal = goal_kind.has_value();
    value.suggested_goal_kind = goal_kind;
    value.safe_summary_zh_cn = std::move(summary);
    return value;
}

DriverCommand command(const DriverTickInput& input, ActionDriverKind kind, std::string action_key, std::string effect_key, std::string summary) {
    DriverCommand value;
    value.command_id = "command." + input.driver_state.driver_state_id + "." + effect_key;
    value.actor_key = input.actor_key;
    value.driver_kind = kind;
    value.object_key = input.driver_state.object_key;
    value.target_key = input.driver_state.target_key;
    value.action_key = std::move(action_key);
    value.effect_key = std::move(effect_key);
    value.estimated_ticks = 1;
    value.requires_world_resolution = true;
    value.public_summary_zh_cn = std::move(summary);
    return value;
}

DriverTickResult runningSummary(std::string summary) {
    DriverTickResult result;
    result.status = ExecutionFrameStatus::Running;
    result.progress_delta = 0.25;
    result.safe_summary_zh_cn = std::move(summary);
    return result;
}

class BasicDriver final : public ActionDriver {
public:
    BasicDriver(ActionDriverKind kind, std::string action_key, std::string effect_key, std::string summary)
        : kind_(kind), action_key_(std::move(action_key)), effect_key_(std::move(effect_key)), summary_(std::move(summary)) {}

    ActionDriverKind kind() const override { return kind_; }
    bool canHandle(const DriverCommand& value) const override { return value.driver_kind == kind_; }

    Result<DriverCheckResult> checkPreconditions(const DriverTickInput& input) const override {
        DriverCheckResult check;
        if (input.driver_state.object_key && quantityOf(input.world_snapshot, *input.driver_state.object_key) <= 0) {
            check.ok = false;
            check.blockers.push_back(blocker(InternalBlockerKind::MissingObject, input, *input.driver_state.object_key, "1", "缺少执行动作需要的对象。", AgentGoalKind::AcquireObject));
        }
        return Result<DriverCheckResult>::ok(check);
    }

    Result<DriverTickResult> tick(const DriverTickInput& input) const override {
        auto check = checkPreconditions(input);
        if (check.is_error()) return Result<DriverTickResult>::fail(check.errors());
        if (!check.value().ok) {
            DriverTickResult result;
            result.status = ExecutionFrameStatus::WaitingForReasoning;
            result.internal_blockers = check.value().blockers;
            result.should_replan = true;
            result.safe_summary_zh_cn = "当前动作缺少前置对象，需要先处理阻塞。";
            return Result<DriverTickResult>::ok(result);
        }
        auto result = runningSummary(summary_);
        result.commands.push_back(command(input, kind_, action_key_, effect_key_, summary_));
        return Result<DriverTickResult>::ok(result);
    }

private:
    ActionDriverKind kind_;
    std::string action_key_;
    std::string effect_key_;
    std::string summary_;
};

class ChopWoodDriver final : public ActionDriver {
public:
    ActionDriverKind kind() const override { return ActionDriverKind::ChopWood; }
    bool canHandle(const DriverCommand& value) const override { return value.driver_kind == ActionDriverKind::ChopWood; }

    Result<DriverCheckResult> checkPreconditions(const DriverTickInput& input) const override {
        DriverCheckResult check;
        if (input.driver_state.required_location_key && input.driver_state.current_location_key && *input.driver_state.required_location_key != *input.driver_state.current_location_key) {
            check.ok = false;
            check.blockers.push_back(blocker(InternalBlockerKind::LocationMismatch, input, *input.driver_state.required_location_key, "same_location", "伐木地点不对，需要先移动。", AgentGoalKind::AcquireObject));
        }
        MaterialRequirementSet set;
        set.set_id = "requirements.chop_wood";
        set.owner_step_id = input.driver_state.driver_state_id;
        set.requirements.push_back(MaterialRequirement{"req.axe", "axe", 1});
        set.requirements.push_back(MaterialRequirement{"req.wood", "wood", 1});
        MaterialRequirementEvaluator evaluator;
        auto evaluated = evaluator.evaluate({input.world_snapshot, input.execution_context.reserved_resources, set});
        if (evaluated.is_error()) return Result<DriverCheckResult>::fail(evaluated.errors());
        if (!evaluated.value().satisfied) {
            auto blockers = evaluator.toBlockers(evaluated.value(), input.actor_key, input.driver_state.driver_state_id);
            if (blockers.is_error()) return Result<DriverCheckResult>::fail(blockers.errors());
            check.ok = false;
            check.blockers.insert(check.blockers.end(), blockers.value().begin(), blockers.value().end());
        }
        const auto* axe = findObject(input.world_snapshot, "axe");
        if (numericState(axe, "sharpness") <= 0.0) {
            check.ok = false;
            check.blockers.push_back(blocker(InternalBlockerKind::ToolStateInsufficient, input, "axe", "sharpness>0", "斧头变钝，需要先打磨。", AgentGoalKind::RestoreToolState));
        }
        return Result<DriverCheckResult>::ok(check);
    }

    Result<DriverTickResult> tick(const DriverTickInput& input) const override {
        auto check = checkPreconditions(input);
        if (check.is_error()) return Result<DriverTickResult>::fail(check.errors());
        if (!check.value().ok) {
            DriverTickResult result;
            result.status = ExecutionFrameStatus::WaitingForReasoning;
            result.internal_blockers = check.value().blockers;
            result.should_replan = true;
            result.safe_summary_zh_cn = "伐木被内部条件阻塞。";
            return Result<DriverTickResult>::ok(result);
        }
        auto result = runningSummary("同伴正在伐木，为后续目标准备木材。");
        result.commands.push_back(command(input, ActionDriverKind::ChopWood, "use", "cut_wood", "同伴挥动斧头砍木头。"));
        return Result<DriverTickResult>::ok(result);
    }
};

class SharpenToolDriver final : public ActionDriver {
public:
    ActionDriverKind kind() const override { return ActionDriverKind::SharpenTool; }
    bool canHandle(const DriverCommand& value) const override { return value.driver_kind == ActionDriverKind::SharpenTool; }

    Result<DriverCheckResult> checkPreconditions(const DriverTickInput& input) const override {
        DriverCheckResult check;
        MaterialRequirementSet set;
        set.set_id = "requirements.sharpen_tool";
        set.owner_step_id = input.driver_state.driver_state_id;
        set.requirements.push_back(MaterialRequirement{"req.axe", "axe", 1});
        set.requirements.push_back(MaterialRequirement{"req.whetstone", "whetstone", 1});
        MaterialRequirementEvaluator evaluator;
        auto evaluated = evaluator.evaluate({input.world_snapshot, input.execution_context.reserved_resources, set});
        if (evaluated.is_error()) return Result<DriverCheckResult>::fail(evaluated.errors());
        if (!evaluated.value().satisfied) {
            auto blockers = evaluator.toBlockers(evaluated.value(), input.actor_key, input.driver_state.driver_state_id);
            if (blockers.is_error()) return Result<DriverCheckResult>::fail(blockers.errors());
            check.ok = false;
            check.blockers = blockers.value();
        }
        return Result<DriverCheckResult>::ok(check);
    }

    Result<DriverTickResult> tick(const DriverTickInput& input) const override {
        auto check = checkPreconditions(input);
        if (check.is_error()) return Result<DriverTickResult>::fail(check.errors());
        if (!check.value().ok) {
            DriverTickResult result;
            result.status = ExecutionFrameStatus::WaitingForReasoning;
            result.internal_blockers = check.value().blockers;
            result.should_replan = true;
            result.safe_summary_zh_cn = "打磨工具缺少材料。";
            return Result<DriverTickResult>::ok(result);
        }
        auto result = runningSummary("同伴正在打磨斧头。");
        result.commands.push_back(command(input, ActionDriverKind::SharpenTool, "use", "restore_sharpness", "同伴用磨石打磨斧头。"));
        return Result<DriverTickResult>::ok(result);
    }
};

// TODO(P41-building-recipes): P40 accepts a MaterialRequirementSet as the minimal build contract.
// Formal building recipes should be loaded from content/config packs in a later production-system phase.
class BuildStructureDriver final : public ActionDriver {
public:
    ActionDriverKind kind() const override { return ActionDriverKind::BuildStructure; }
    bool canHandle(const DriverCommand& value) const override { return value.driver_kind == ActionDriverKind::BuildStructure; }

    Result<DriverCheckResult> checkPreconditions(const DriverTickInput& input) const override {
        DriverCheckResult check;
        if (input.driver_state.material_requirements.requirements.empty()) {
            check.ok = false;
            check.blockers.push_back(blocker(InternalBlockerKind::MissingCondition, input, "building_recipe", "material_requirement_set", "建造目标缺少材料需求配置，不能依赖隐藏 fallback。", AgentGoalKind::AcquireObject));
            return Result<DriverCheckResult>::ok(check);
        }
        MaterialRequirementEvaluator evaluator;
        auto evaluated = evaluator.evaluate({input.world_snapshot, input.execution_context.reserved_resources, input.driver_state.material_requirements});
        if (evaluated.is_error()) return Result<DriverCheckResult>::fail(evaluated.errors());
        if (!evaluated.value().satisfied) {
            auto blockers = evaluator.toBlockers(evaluated.value(), input.actor_key, input.driver_state.driver_state_id);
            if (blockers.is_error()) return Result<DriverCheckResult>::fail(blockers.errors());
            check.ok = false;
            check.blockers = blockers.value();
        }
        return Result<DriverCheckResult>::ok(check);
    }

    Result<DriverTickResult> tick(const DriverTickInput& input) const override {
        auto check = checkPreconditions(input);
        if (check.is_error()) return Result<DriverTickResult>::fail(check.errors());
        if (!check.value().ok) {
            DriverTickResult result;
            result.status = ExecutionFrameStatus::WaitingForReasoning;
            result.internal_blockers = check.value().blockers;
            result.should_replan = true;
            result.safe_summary_zh_cn = "建造目标缺少材料或配置，需要先补齐。";
            return Result<DriverTickResult>::ok(result);
        }
        auto result = runningSummary("同伴正在按材料需求推进建造。");
        result.commands.push_back(command(input, ActionDriverKind::BuildStructure, "use", input.driver_state.action_key.empty() ? "build_structure" : input.driver_state.action_key, "同伴提交建造结算请求。"));
        return Result<DriverTickResult>::ok(result);
    }
};

class MoveToDriver final : public ActionDriver {
public:
    ActionDriverKind kind() const override { return ActionDriverKind::MoveTo; }
    bool canHandle(const DriverCommand& value) const override { return value.driver_kind == ActionDriverKind::MoveTo; }
    Result<DriverCheckResult> checkPreconditions(const DriverTickInput&) const override { return Result<DriverCheckResult>::ok({}); }
    Result<DriverTickResult> tick(const DriverTickInput& input) const override {
        auto result = runningSummary("同伴正在移动到目标地点。完整寻路会由后续地图系统接入。");
        result.commands.push_back(command(input, ActionDriverKind::MoveTo, "move", "move_to", "同伴请求移动到目标地点。"));
        return Result<DriverTickResult>::ok(result);
    }
};

class CounterThreatDriver final : public ActionDriver {
public:
    ActionDriverKind kind() const override { return ActionDriverKind::CounterThreat; }
    bool canHandle(const DriverCommand& value) const override { return value.driver_kind == ActionDriverKind::CounterThreat; }
    Result<DriverCheckResult> checkPreconditions(const DriverTickInput& input) const override {
        DriverCheckResult check;
        MaterialRequirementSet set;
        set.set_id = "requirements.counter_threat";
        set.owner_step_id = input.driver_state.driver_state_id;
        set.requirements.push_back(MaterialRequirement{"req.torch", "torch", 1});
        MaterialRequirementEvaluator evaluator;
        auto evaluated = evaluator.evaluate({input.world_snapshot, input.execution_context.reserved_resources, set});
        if (evaluated.is_error()) return Result<DriverCheckResult>::fail(evaluated.errors());
        if (!evaluated.value().satisfied) {
            auto blockers = evaluator.toBlockers(evaluated.value(), input.actor_key, input.driver_state.driver_state_id);
            if (blockers.is_error()) return Result<DriverCheckResult>::fail(blockers.errors());
            check.ok = false;
            check.blockers = blockers.value();
        }
        return Result<DriverCheckResult>::ok(check);
    }
    Result<DriverTickResult> tick(const DriverTickInput& input) const override {
        auto check = checkPreconditions(input);
        if (check.is_error()) return Result<DriverTickResult>::fail(check.errors());
        if (!check.value().ok) {
            DriverTickResult result;
            result.status = ExecutionFrameStatus::WaitingForReasoning;
            result.internal_blockers = check.value().blockers;
            result.should_replan = true;
            result.safe_summary_zh_cn = "处理威胁缺少可靠反制物，需要请求推理。";
            return Result<DriverTickResult>::ok(result);
        }
        auto result = runningSummary("同伴暂停原任务，正在处理靠近的威胁。");
        result.commands.push_back(command(input, ActionDriverKind::CounterThreat, "use", "repel_beast", "同伴准备用火把驱赶野兽。"));
        return Result<DriverTickResult>::ok(result);
    }
};

} // namespace

GoalStackManager::GoalStackManager(uint32_t max_depth) : max_depth_(max_depth) {}

Result<void> GoalStackManager::pushGoal(ExecutionContext& context, GoalFrame frame) const {
    if (context.goal_stack.size() >= max_depth_) return fail("goal stack depth exceeded");
    if (frame.frame_id.empty()) frame.frame_id = "frame." + frame.goal_id;
    frame.status = ExecutionFrameStatus::Running;
    context.active_frame_id = frame.frame_id;
    context.goal_stack.push_back(std::move(frame));
    return okVoid();
}

Result<void> GoalStackManager::pauseFrame(ExecutionContext& context, const std::string& frame_id, const std::string& reason_key, uint64_t tick) const {
    auto* frame = findFrame(context, frame_id);
    if (frame == nullptr) return fail("goal frame not found for pause");
    frame->status = ExecutionFrameStatus::Paused;
    if (context.active_driver_state) {
        PausedExecutionContext paused;
        paused.paused_context_id = "paused." + frame_id + "." + std::to_string(tick);
        paused.frame_id = frame_id;
        paused.driver_state = *context.active_driver_state;
        paused.pause_reason_key = reason_key;
        paused.paused_tick = tick;
        paused.resume_deadline_tick = tick + 32;
        paused.resume_validator_keys = {"goal_valid", "resource_available", "location_safe"};
        context.paused_contexts.push_back(std::move(paused));
    }
    context.active_frame_id.reset();
    context.active_driver_state.reset();
    return okVoid();
}

Result<void> GoalStackManager::resumeFrame(ExecutionContext& context, const std::string& frame_id) const {
    auto* frame = findFrame(context, frame_id);
    if (frame == nullptr) return fail("goal frame not found for resume");
    auto paused = std::find_if(context.paused_contexts.begin(), context.paused_contexts.end(), [&](const auto& value) { return value.frame_id == frame_id; });
    if (paused == context.paused_contexts.end()) return fail("paused context not found for resume");
    frame->status = ExecutionFrameStatus::Running;
    context.active_frame_id = frame_id;
    context.active_driver_state = paused->driver_state;
    context.paused_contexts.erase(paused);
    return okVoid();
}

Result<void> GoalStackManager::completeFrame(ExecutionContext& context, const std::string& frame_id) const {
    auto* frame = findFrame(context, frame_id);
    if (frame == nullptr) return fail("goal frame not found for complete");
    frame->status = ExecutionFrameStatus::Completed;
    if (context.active_frame_id == frame_id) {
        context.active_frame_id.reset();
        context.active_driver_state.reset();
    }
    return okVoid();
}

Result<void> GoalStackManager::cancelFrame(ExecutionContext& context, const std::string& frame_id) const {
    auto* frame = findFrame(context, frame_id);
    if (frame == nullptr) return fail("goal frame not found for cancel");
    frame->status = ExecutionFrameStatus::Cancelled;
    MaterialReservationManager reservations;
    auto released = reservations.releaseByGoal(context, frame->goal_id);
    if (released.is_error()) return released;
    if (context.active_frame_id == frame_id) {
        context.active_frame_id.reset();
        context.active_driver_state.reset();
    }
    return okVoid();
}

Result<GoalFrame> GoalStackManager::activeFrame(const ExecutionContext& context) const {
    if (!context.active_frame_id) return failValue<GoalFrame>("active frame missing");
    const auto* frame = findFrame(context, *context.active_frame_id);
    if (frame == nullptr) return failValue<GoalFrame>("active frame not found");
    return Result<GoalFrame>::ok(*frame);
}

Result<void> ActionDriverRegistry::registerDriver(std::unique_ptr<ActionDriver> driver) {
    if (!driver) return fail("driver is null");
    drivers_[driver->kind()] = std::move(driver);
    return okVoid();
}

Result<const ActionDriver*> ActionDriverRegistry::find(ActionDriverKind kind, AgentCapabilityTier tier) const {
    if (tier == AgentCapabilityTier::InstinctAgent && !(kind == ActionDriverKind::CounterThreat || kind == ActionDriverKind::MoveTo || kind == ActionDriverKind::UseObject)) {
        return failValue<const ActionDriver*>("driver filtered by instinct capability");
    }
    if (tier == AgentCapabilityTier::SimpleAgent && kind == ActionDriverKind::AdvancedConstruct) {
        return failValue<const ActionDriver*>("driver filtered by simple capability");
    }
    auto it = drivers_.find(kind);
    if (it == drivers_.end()) return failValue<const ActionDriver*>("driver not registered");
    return Result<const ActionDriver*>::ok(it->second.get());
}

Result<void> ActionDriverRegistry::validateAll() const {
    const std::vector<ActionDriverKind> required{ActionDriverKind::Eat, ActionDriverKind::Gather, ActionDriverKind::ChopWood, ActionDriverKind::SharpenTool, ActionDriverKind::BuildStructure, ActionDriverKind::UseObject, ActionDriverKind::MoveTo, ActionDriverKind::CounterThreat};
    for (const auto kind : required) {
        if (!drivers_.contains(kind)) return fail("default driver missing: " + toString(kind));
    }
    return okVoid();
}

Result<void> ActionDriverRegistry::registerDefaultDrivers() {
    auto eat = registerDriver(std::make_unique<BasicDriver>(ActionDriverKind::Eat, "eat", "restore_hunger", "同伴正在吃东西。"));
    if (eat.is_error()) return eat;
    auto gather = registerDriver(std::make_unique<BasicDriver>(ActionDriverKind::Gather, "gather", "gather_object", "同伴正在采集资源。"));
    if (gather.is_error()) return gather;
    auto use = registerDriver(std::make_unique<BasicDriver>(ActionDriverKind::UseObject, "use", "use_object", "同伴正在使用物品。"));
    if (use.is_error()) return use;
    auto chop = registerDriver(std::make_unique<ChopWoodDriver>());
    if (chop.is_error()) return chop;
    auto sharpen = registerDriver(std::make_unique<SharpenToolDriver>());
    if (sharpen.is_error()) return sharpen;
    auto build = registerDriver(std::make_unique<BuildStructureDriver>());
    if (build.is_error()) return build;
    auto move = registerDriver(std::make_unique<MoveToDriver>());
    if (move.is_error()) return move;
    return registerDriver(std::make_unique<CounterThreatDriver>());
}

Result<MaterialEvaluationResult> MaterialRequirementEvaluator::evaluate(const MaterialEvaluationInput& input) const {
    auto valid = input.requirements.validateBasic();
    if (valid.is_error()) return Result<MaterialEvaluationResult>::fail(valid.errors());
    MaterialEvaluationResult result;
    result.evaluation_id = "material_eval." + input.requirements.set_id;
    result.satisfied = true;
    for (const auto& requirement : input.requirements.requirements) {
        MaterialAvailability item;
        item.object_key = requirement.object_key;
        const auto* object = findObject(input.world_snapshot, requirement.object_key);
        item.total_quantity = object == nullptr ? 0 : object->quantity;
        for (const auto& reservation : input.reserved_resources) {
            if (reservation.status == ReservationStatus::Active && reservation.resource_key == requirement.object_key) item.reserved_quantity += reservation.quantity;
        }
        item.available_quantity = std::max(0, item.total_quantity - item.reserved_quantity);
        bool state_ok = object != nullptr;
        if (object != nullptr) {
            for (const auto& tag : requirement.acceptable_state_tags) state_ok = state_ok && contains(object->state_tags, tag);
            for (const auto& tag : requirement.forbidden_state_tags) state_ok = state_ok && !contains(object->state_tags, tag);
            if (requirement.quality_key && requirement.minimum_quality_value) state_ok = state_ok && numericState(object, *requirement.quality_key) >= *requirement.minimum_quality_value;
        }
        item.matching_quantity = state_ok ? item.available_quantity : 0;
        item.missing_quantity = std::max(0, requirement.required_quantity - item.matching_quantity);
        item.blocked_by_state = object != nullptr && item.available_quantity > 0 && !state_ok;
        item.blocked_by_reservation = item.total_quantity >= requirement.required_quantity && item.available_quantity < requirement.required_quantity;
        item.safe_summary_zh_cn = item.missing_quantity == 0 ? "材料可用。" : "材料不足或状态不符。";
        if (item.missing_quantity > 0) result.satisfied = false;
        result.availability.push_back(std::move(item));
    }
    result.safe_summary_zh_cn = result.satisfied ? "材料需求已满足。" : "材料需求未满足。";
    return Result<MaterialEvaluationResult>::ok(std::move(result));
}

Result<std::vector<InternalBlocker>> MaterialRequirementEvaluator::toBlockers(const MaterialEvaluationResult& result, const std::string& actor_key, const std::string& driver_state_id) const {
    std::vector<InternalBlocker> blockers;
    for (const auto& availability : result.availability) {
        if (availability.missing_quantity <= 0 && !availability.blocked_by_state && !availability.blocked_by_reservation) continue;
        InternalBlocker blocker;
        blocker.blocker_id = "blocker." + driver_state_id + ".material." + availability.object_key;
        blocker.kind = availability.blocked_by_state ? InternalBlockerKind::ObjectStateInvalid : (availability.blocked_by_reservation ? InternalBlockerKind::ResourceReserved : InternalBlockerKind::InsufficientQuantity);
        blocker.actor_key = actor_key;
        blocker.source_driver_state_id = driver_state_id;
        blocker.missing_key = availability.object_key;
        blocker.required_value = std::to_string(std::max(1, availability.missing_quantity));
        blocker.can_generate_subgoal = blocker.kind != InternalBlockerKind::ObjectStateInvalid;
        blocker.suggested_goal_kind = blocker.can_generate_subgoal ? std::optional<AgentGoalKind>(AgentGoalKind::AcquireObject) : std::nullopt;
        blocker.safe_summary_zh_cn = availability.blocked_by_state ? "材料状态不符合当前动作要求。" : (availability.blocked_by_reservation ? "材料已被其他目标预定。" : "材料数量不足，需要先获取。");
        blockers.push_back(std::move(blocker));
    }
    return Result<std::vector<InternalBlocker>>::ok(std::move(blockers));
}

Result<ResourceReservation> MaterialReservationManager::reserve(ExecutionContext& context, const MaterialReserveRequest& request) const {
    if (request.quantity <= 0) return Result<ResourceReservation>::fail(makeError(ErrorCode::validation_value_out_of_range, "reservation quantity invalid"));
    ResourceReservation reservation;
    reservation.reservation_id = "reservation." + request.purpose_goal_id + "." + request.resource_key;
    reservation.actor_key = request.actor_key;
    reservation.resource_key = request.resource_key;
    reservation.quantity = request.quantity;
    reservation.purpose_goal_id = request.purpose_goal_id;
    reservation.expires_tick = request.expires_tick;
    reservation.status = ReservationStatus::Active;
    context.reserved_resources.push_back(reservation);
    return Result<ResourceReservation>::ok(std::move(reservation));
}

Result<void> MaterialReservationManager::release(ExecutionContext& context, const std::string& reservation_id) const {
    for (auto& reservation : context.reserved_resources) {
        if (reservation.reservation_id == reservation_id) reservation.status = ReservationStatus::Released;
    }
    return okVoid();
}

Result<void> MaterialReservationManager::releaseByGoal(ExecutionContext& context, const std::string& goal_id) const {
    for (auto& reservation : context.reserved_resources) {
        if (reservation.purpose_goal_id == goal_id && reservation.status == ReservationStatus::Active) reservation.status = ReservationStatus::Released;
    }
    return okVoid();
}

Result<int> MaterialReservationManager::reservedQuantity(const ExecutionContext& context, const std::string& object_key) const {
    int total = 0;
    for (const auto& reservation : context.reserved_resources) {
        if (reservation.status == ReservationStatus::Active && reservation.resource_key == object_key) total += reservation.quantity;
    }
    return Result<int>::ok(total);
}

Result<std::vector<ReactionMaterialLink>> ReactionMaterialResolver::findProducers(const MaterialRequirement& requirement, const std::vector<ObjectReactionRule>& rules) const {
    auto valid = requirement.validateBasic();
    if (valid.is_error()) return Result<std::vector<ReactionMaterialLink>>::fail(valid.errors());
    std::vector<ReactionMaterialLink> links;
    for (const auto& rule : rules) {
        for (const auto& output : rule.output_templates) {
            std::string output_key;
            if ((output.output_kind == ReactionOutputKind::ProduceObject || output.output_kind == ReactionOutputKind::TransformObject) && !output.product_definition_id.empty()) output_key = productKey(output.product_definition_id.value());
            if (output.output_kind == ReactionOutputKind::ResourceDelta && output.resource_key == "sharpness" && output.resource_delta > 0.0) output_key = "axe";
            if (output_key != requirement.object_key && requirement.source_effect_key.value_or("") != rule.knowledge_effect_key) continue;
            ReactionMaterialLink link;
            link.link_id = "reaction_material." + requirement.requirement_id + "." + rule.rule_key;
            link.requirement_id = requirement.requirement_id;
            link.reaction_rule_id = rule.rule_key;
            link.output_object_key = output_key.empty() ? requirement.object_key : output_key;
            link.expected_output_quantity = std::max(1, output.quantity_delta);
            for (const auto& pattern : rule.object_patterns) link.input_object_keys.push_back(patternObjectKey(pattern));
            for (const auto& condition : rule.condition_refs) {
                if (!link.condition_expression.empty()) link.condition_expression += ";";
                link.condition_expression += condition.inline_canonical_key;
            }
            links.push_back(std::move(link));
        }
    }
    return Result<std::vector<ReactionMaterialLink>>::ok(std::move(links));
}

Result<std::vector<ReactionMaterialLink>> ReactionMaterialResolver::findProducers(const MaterialRequirement& requirement) const {
    return findProducers(requirement, pathfinder::reaction::fixtures::coreP28Rules());
}

Result<std::vector<AgentGoal>> InternalBlockerResolver::resolve(const InternalBlockerResolveInput& input) const {
    std::vector<AgentGoal> goals;
    for (const auto& blocker : input.blockers) {
        if (!blocker.can_generate_subgoal || !blocker.suggested_goal_kind) continue;
        AgentGoal goal;
        goal.goal_id = "goal." + blocker.actor_key + "." + blocker.missing_key + ".from_blocker";
        goal.actor_key = blocker.actor_key;
        goal.kind = *blocker.suggested_goal_kind;
        goal.target_key = blocker.missing_key;
        goal.urgency = blocker.kind == InternalBlockerKind::ToolStateInsufficient ? 75.0 : 55.0;
        goal.desired_delta = 1.0;
        goal.source_keys = {blocker.blocker_id};
        goals.push_back(std::move(goal));
    }
    return Result<std::vector<AgentGoal>>::ok(std::move(goals));
}

double InterruptPriorityEvaluator::score(const ExternalInterruptSignal& signal, const ExecutionContext&, const GoalFrame*) const {
    double priority = signal.severity * 0.45 + signal.urgency * 0.35;
    if (signal.kind == ExternalInterruptKind::ThreatAppeared || signal.kind == ExternalInterruptKind::ThreatEscalated || signal.kind == ExternalInterruptKind::DependentInDanger || signal.kind == ExternalInterruptKind::FireSpread) priority += 20.0;
    if (signal.kind == ExternalInterruptKind::CommandOverride) priority += 10.0;
    return std::min(100.0, priority);
}

Result<std::vector<InterruptDecision>> InterruptSystem::evaluate(const InterruptEvaluationInput& input) const {
    std::vector<InterruptDecision> decisions;
    const GoalFrame* active = input.context.active_frame_id ? findFrame(input.context, *input.context.active_frame_id) : nullptr;
    InterruptPriorityEvaluator evaluator;
    for (const auto& signal : input.signals) {
        const bool targeted = signal.target_actor_keys.empty() || contains(signal.target_actor_keys, input.context.actor_key);
        const bool same_location = !signal.location_key || *signal.location_key == input.world_snapshot.location_key;
        if (!targeted || !same_location) continue;
        const double score = evaluator.score(signal, input.context, active);
        InterruptDecision decision;
        decision.decision_id = "interrupt_decision." + signal.interrupt_id;
        decision.actor_key = input.context.actor_key;
        decision.interrupt_id = signal.interrupt_id;
        if (active != nullptr) decision.affected_frame_id = active->frame_id;
        decision.trace_keys = {"interrupt:" + toString(signal.kind)};
        if (signal.kind == ExternalInterruptKind::CommandOverride && score < 70.0) {
            decision.kind = InterruptDecisionKind::ObserveOnly;
            decision.safe_explanation_zh_cn = "玩家命令已记录，但当前任务暂不被打断。";
        } else if (score >= 70.0 || !signal.can_be_ignored) {
            decision.kind = InterruptDecisionKind::PauseAndInsertEmergencyGoal;
            decision.pause_current = active != nullptr;
            decision.requires_replan = true;
            AgentGoal goal;
            goal.goal_id = "goal." + input.context.actor_key + ".emergency." + signal.interrupt_id;
            goal.actor_key = input.context.actor_key;
            goal.kind = signal.kind == ExternalInterruptKind::DependentInDanger ? AgentGoalKind::ProtectDependent : AgentGoalKind::ReduceThreat;
            goal.target_key = signal.threat_key.value_or(signal.source_event_id);
            goal.urgency = score;
            goal.source_keys = {signal.interrupt_id};
            decision.inserted_goal_request = goal;
            decision.safe_explanation_zh_cn = signal.kind == ExternalInterruptKind::CommandOverride ? "同伴暂时延迟玩家命令：更高优先级危险正在发生，危险解除后会重新评估。" : "外界危险优先级更高，同伴暂停当前任务先处理危险。";
        } else if (score >= 40.0) {
            decision.kind = InterruptDecisionKind::ObserveOnly;
            decision.safe_explanation_zh_cn = "同伴注意到外界变化，但当前任务继续执行。";
        } else {
            decision.kind = InterruptDecisionKind::Ignore;
            decision.safe_explanation_zh_cn = "外界变化未影响当前任务。";
        }
        decisions.push_back(std::move(decision));
    }
    return Result<std::vector<InterruptDecision>>::ok(std::move(decisions));
}

Result<ResumeDecision> ExecutionResumeValidator::validate(const ExecutionContext&, const PausedExecutionContext& paused, const WorldSnapshot& snapshot, uint64_t tick) const {
    ResumeDecision decision;
    if (tick > paused.resume_deadline_tick) {
        decision.kind = ResumeDecisionKind::Replan;
        decision.safe_summary_zh_cn = "原任务暂停过久，需要重新规划。";
        return Result<ResumeDecision>::ok(decision);
    }
    if (paused.driver_state.object_key && quantityOf(snapshot, *paused.driver_state.object_key) <= 0) {
        decision.kind = ResumeDecisionKind::Replan;
        decision.safe_summary_zh_cn = "原任务依赖的对象已经不可用，需要重新规划。";
        return Result<ResumeDecision>::ok(decision);
    }
    decision.kind = ResumeDecisionKind::Resume;
    decision.safe_summary_zh_cn = "危险解除后，原任务仍然有效，可以恢复。";
    return Result<ResumeDecision>::ok(decision);
}

Result<std::vector<WorldInteractionInput>> DriverCommandAdapter::adapt(const std::vector<DriverCommand>& commands) const {
    std::vector<WorldInteractionInput> inputs;
    for (const auto& command : commands) {
        if (!command.requires_world_resolution) continue;
        WorldInteractionInput input;
        input.interaction_id = command.command_id;
        input.actor_key = command.actor_key;
        input.object_key = command.object_key.value_or("");
        input.target_object_key = command.target_key.value_or("");
        input.action = command.action_key == "eat" ? DialogActionKind::Eat : (command.action_key == "wait" ? DialogActionKind::Wait : DialogActionKind::Use);
        input.effect_key = command.effect_key.value_or(command.action_key);
        input.feedback_key = command.command_id + ".feedback";
        input.reason_keys = {"p40_driver_command", toString(command.driver_kind)};
        inputs.push_back(std::move(input));
    }
    return Result<std::vector<WorldInteractionInput>>::ok(std::move(inputs));
}

ExecutionStatusProjection ExecutionProjectionMapper::map(const ExecutionResult& result) const {
    ExecutionStatusProjection projection;
    if (result.updated_context.active_frame_id) {
        const auto* frame = findFrame(result.updated_context, *result.updated_context.active_frame_id);
        if (frame != nullptr) projection.current_goal = {frame->public_reason_zh_cn, frame->trace_keys};
    }
    if (result.updated_context.active_driver_state) projection.active_step = {"当前步骤：" + toString(result.updated_context.active_driver_state->driver_kind), result.updated_context.trace_keys};
    if (!result.internal_blockers.empty()) projection.blocked_by = {result.internal_blockers.front().safe_summary_zh_cn, {result.internal_blockers.front().blocker_id}};
    if (!result.interrupt_decisions.empty()) {
        projection.interrupt_reason = {result.interrupt_decisions.front().safe_explanation_zh_cn, result.interrupt_decisions.front().trace_keys};
        projection.response_plan = {result.interrupt_decisions.front().requires_replan ? "先处理紧急目标，再重新评估原任务。" : "记录外界变化，当前任务继续。", result.interrupt_decisions.front().trace_keys};
    }
    projection.resume_hint = {result.safe_summary_zh_cn, result.trace};
    for (const auto& trace : result.trace) projection.trace_lines.push_back({trace, {trace}});
    return projection;
}

GoalExecutionSystem::GoalExecutionSystem() {
    auto registered = registry_.registerDefaultDrivers();
    (void)registered;
}

Result<ExecutionResult> GoalExecutionSystem::tick(const ExecutionTickRequest& request) const {
    ExecutionResult result;
    result.ok = true;
    result.updated_context = request.context;
    result.updated_context.last_update_tick = request.tick;
    if (request.incoming_plan) {
        auto frame = goalFrameFromPlan(*request.incoming_plan, request.tick);
        auto pushed = stack_manager_.pushGoal(result.updated_context, frame);
        if (pushed.is_error()) return Result<ExecutionResult>::fail(pushed.errors());
        if (!request.incoming_plan->steps.empty()) result.updated_context.active_driver_state = driverStateFromPlanStep(request.incoming_plan->steps.front());
    }

    auto interrupts = interrupt_system_.evaluate({request.visible_events, result.updated_context, request.world_snapshot});
    if (interrupts.is_error()) return Result<ExecutionResult>::fail(interrupts.errors());
    result.interrupt_decisions = interrupts.value();
    auto preempt = std::find_if(result.interrupt_decisions.begin(), result.interrupt_decisions.end(), [](const auto& decision) { return decision.kind == InterruptDecisionKind::PauseAndInsertEmergencyGoal; });
    if (preempt != result.interrupt_decisions.end()) {
        if (preempt->pause_current && result.updated_context.active_frame_id) {
            auto paused = stack_manager_.pauseFrame(result.updated_context, *result.updated_context.active_frame_id, preempt->interrupt_id, request.tick);
            if (paused.is_error()) return Result<ExecutionResult>::fail(paused.errors());
        }
        if (preempt->inserted_goal_request) {
            GoalFrame emergency;
            emergency.frame_id = "frame." + preempt->inserted_goal_request->goal_id;
            emergency.actor_key = preempt->inserted_goal_request->actor_key;
            emergency.goal_id = preempt->inserted_goal_request->goal_id;
            emergency.plan_id = "plan.emergency.pending";
            emergency.priority = preempt->inserted_goal_request->urgency;
            emergency.created_tick = request.tick;
            emergency.public_reason_zh_cn = "处理外界紧急事件";
            emergency.trace_keys = {preempt->interrupt_id};
            auto pushed = stack_manager_.pushGoal(result.updated_context, emergency);
            if (pushed.is_error()) return Result<ExecutionResult>::fail(pushed.errors());
            DriverExecutionState state;
            state.driver_state_id = "driver.counter_threat." + preempt->interrupt_id;
            state.driver_kind = ActionDriverKind::CounterThreat;
            state.action_key = "repel_beast";
            state.object_key = "torch";
            state.target_key = preempt->inserted_goal_request->target_key;
            result.updated_context.active_driver_state = state;
        }
        result.requires_reasoning = true;
        result.reasoning_request.request_id = "reasoning." + preempt->interrupt_id;
        result.reasoning_request.actor_key = result.updated_context.actor_key;
        result.reasoning_request.world_snapshot = request.world_snapshot;
        result.reasoning_request.agent_knowledge = result.updated_context.known_claims;
        result.reasoning_request.trigger_key = preempt->interrupt_id;
        result.safe_summary_zh_cn = preempt->safe_explanation_zh_cn;
        result.trace = {"tick_order:interrupt_before_driver", "interrupt:" + preempt->interrupt_id};
        result.projection = projection_mapper_.map(result);
        return Result<ExecutionResult>::ok(std::move(result));
    }

    if (!result.updated_context.active_frame_id || !result.updated_context.active_driver_state) {
        if (!result.updated_context.paused_contexts.empty()) {
            auto resume = resume_validator_.validate(result.updated_context, result.updated_context.paused_contexts.back(), request.world_snapshot, request.tick);
            if (resume.is_error()) return Result<ExecutionResult>::fail(resume.errors());
            if (resume.value().kind == ResumeDecisionKind::Resume) {
                auto resumed = stack_manager_.resumeFrame(result.updated_context, result.updated_context.paused_contexts.back().frame_id);
                if (resumed.is_error()) return Result<ExecutionResult>::fail(resumed.errors());
                result.safe_summary_zh_cn = resume.value().safe_summary_zh_cn;
            } else {
                result.requires_reasoning = true;
                result.safe_summary_zh_cn = resume.value().safe_summary_zh_cn;
                result.projection = projection_mapper_.map(result);
                return Result<ExecutionResult>::ok(std::move(result));
            }
        } else {
            result.safe_summary_zh_cn = "没有正在执行的目标。";
            result.projection = projection_mapper_.map(result);
            return Result<ExecutionResult>::ok(std::move(result));
        }
    }

    auto active = stack_manager_.activeFrame(result.updated_context);
    if (active.is_error()) return Result<ExecutionResult>::fail(active.errors());
    auto driver = registry_.find(result.updated_context.active_driver_state->driver_kind, result.updated_context.capability_tier);
    if (driver.is_error()) return Result<ExecutionResult>::fail(driver.errors());
    DriverTickInput input;
    input.request_id = request.request_id;
    input.actor_key = result.updated_context.actor_key;
    input.world_snapshot = request.world_snapshot;
    input.execution_context = result.updated_context;
    input.active_frame = active.value();
    input.driver_state = *result.updated_context.active_driver_state;
    input.elapsed_ticks = request.elapsed_ticks;
    input.visible_events = request.visible_events;
    auto ticked = driver.value()->tick(input);
    if (ticked.is_error()) return Result<ExecutionResult>::fail(ticked.errors());
    result.commands_to_resolve = ticked.value().commands;
    result.internal_blockers = ticked.value().internal_blockers;
    result.execution_feedback = ticked.value().generated_feedback;
    result.requires_reasoning = ticked.value().should_replan;
    if (result.requires_reasoning) {
        result.reasoning_request.request_id = "reasoning.blocker." + request.request_id;
        result.reasoning_request.actor_key = result.updated_context.actor_key;
        result.reasoning_request.world_snapshot = request.world_snapshot;
        result.reasoning_request.agent_knowledge = result.updated_context.known_claims;
        result.reasoning_request.trigger_key = result.internal_blockers.empty() ? "driver_replan" : result.internal_blockers.front().blocker_id;
        auto goals = blocker_resolver_.resolve({result.updated_context, result.internal_blockers});
        if (goals.is_error()) return Result<ExecutionResult>::fail(goals.errors());
        result.generated_subgoals = goals.value();
        if (!result.generated_subgoals.empty()) {
            // TODO(P41-multi-blocker-scheduling): P40 only pushes the first generated subgoal.
            // Later multi-agent/resource scheduling should rank and queue multiple blockers explicitly.
            const auto parent_frame_id = result.updated_context.active_frame_id;
            if (parent_frame_id) {
                auto paused = stack_manager_.pauseFrame(result.updated_context, *parent_frame_id, result.reasoning_request.trigger_key, request.tick);
                if (paused.is_error()) return Result<ExecutionResult>::fail(paused.errors());
            }
            const auto& subgoal = result.generated_subgoals.front();
            GoalFrame frame;
            frame.frame_id = "frame." + subgoal.goal_id;
            frame.actor_key = subgoal.actor_key;
            frame.goal_id = subgoal.goal_id;
            frame.plan_id = "plan.pending.reasoning";
            frame.parent_frame_id = parent_frame_id;
            frame.priority = subgoal.urgency;
            frame.created_tick = request.tick;
            frame.public_reason_zh_cn = "先处理阻塞：" + subgoal.target_key.value_or("未知目标");
            frame.trace_keys = subgoal.source_keys;
            auto pushed = stack_manager_.pushGoal(result.updated_context, std::move(frame));
            if (pushed.is_error()) return Result<ExecutionResult>::fail(pushed.errors());
            result.updated_context.active_driver_state.reset();
        }
    }
    auto adapted = command_adapter_.adapt(result.commands_to_resolve);
    if (adapted.is_error()) return Result<ExecutionResult>::fail(adapted.errors());
    result.world_changes = adapted.value();
    if (result.updated_context.active_driver_state) {
        result.updated_context.active_driver_state->progress = std::min(1.0, result.updated_context.active_driver_state->progress + ticked.value().progress_delta);
        result.updated_context.active_driver_state->remaining_ticks = result.updated_context.active_driver_state->remaining_ticks > 0 ? result.updated_context.active_driver_state->remaining_ticks - 1 : 0;
    }
    result.safe_summary_zh_cn = ticked.value().safe_summary_zh_cn;
    result.trace = {"tick_order:interrupt_before_driver", "driver:" + toString(input.driver_state.driver_kind)};
    result.projection = projection_mapper_.map(result);
    return Result<ExecutionResult>::ok(std::move(result));
}

DriverExecutionState driverStateFromPlanStep(const AgentPlanStep& step) {
    DriverExecutionState state;
    state.driver_state_id = "driver_state." + step.step_id;
    state.driver_kind = driverKindForStep(step);
    state.action_key = step.effect_key.empty() ? step.action_key : step.effect_key;
    if (!step.object_key.empty()) state.object_key = step.object_key;
    state.target_key = step.target_key;
    state.remaining_ticks = std::max<uint32_t>(1, step.estimated_ticks);
    if (state.driver_kind == ActionDriverKind::BuildStructure) {
        state.material_requirements.set_id = "requirements." + step.step_id;
        state.material_requirements.owner_step_id = step.step_id;
    }
    return state;
}

GoalFrame goalFrameFromPlan(const AgentPlan& plan, uint64_t tick) {
    GoalFrame frame;
    frame.frame_id = "frame." + plan.goal.goal_id;
    frame.actor_key = plan.actor_key;
    frame.goal_id = plan.goal.goal_id;
    frame.plan_id = plan.plan_id;
    frame.priority = plan.goal.urgency;
    frame.created_tick = tick;
    if (plan.goal.expires_after_ticks) frame.expires_tick = tick + *plan.goal.expires_after_ticks;
    frame.public_reason_zh_cn = plan.steps.empty() ? "执行当前目标" : plan.steps.front().explanation_zh_cn;
    frame.trace_keys = plan.trace_keys;
    return frame;
}

} // namespace pathfinder::goal_execution
