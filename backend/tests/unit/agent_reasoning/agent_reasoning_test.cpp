#include "pathfinder/agent_reasoning/agent_reasoner.h"
#include "pathfinder/agent_reasoning/effect_execution.h"
#include "pathfinder/agent_reasoning/reaction_planning_adapter.h"
#include "pathfinder/condition/condition_expression_evaluator.h"
#include <algorithm>
#include <cassert>
#include <iostream>

using namespace pathfinder::agent_reasoning;
using namespace pathfinder::foundation;
using namespace pathfinder::knowledge;
using namespace pathfinder::world_interaction;

static WorldObjectInstance object(std::string key, int quantity) {
    WorldObjectInstance item;
    item.instance_id = key + ".instance";
    item.definition_key = key;
    item.display_name_zh_cn = key;
    item.kind = WorldObjectInstanceKind::ResourceStack;
    item.quantity = quantity;
    return item;
}

static WorldSnapshot snapshot() {
    WorldSnapshot value;
    value.snapshot_id = "snapshot.p39";
    value.scenario_key = "p39";
    value.turn_index = 1;
    WorldActorRuntimeState companion;
    companion.actor_key = "companion";
    companion.display_name_zh_cn = "同伴";
    companion.trust = 0.8;
    value.actors_by_key[companion.actor_key] = companion;
    return value;
}

static KnowledgeClaim claim(std::string owner, std::string object_key, std::string action_key, std::string effect_key, double confidence = 0.9) {
    KnowledgeClaim value;
    value.knowledge_id = KnowledgeId("knowledge." + owner + "." + effect_key + "." + object_key);
    value.owner.kind = KnowledgeOwnerKind::Agent;
    value.owner.entity_id = EntityId(owner);
    value.subject.kind = KnowledgeSubjectKind::ObjectInstance;
    value.subject.subject_id = object_key;
    value.subject.subject_type_key = "world_object";
    value.predicate.relation_type = KnowledgeRelationType::HasEffect;
    value.predicate.action_key = action_key;
    value.predicate.effect_key = effect_key;
    value.confidence.confidence = confidence;
    value.confidence.band = KnowledgeConfidenceBand::Reliable;
    value.status = KnowledgeStatus::Active;
    value.projection_flags.usable_by_ai = true;
    value.projection_flags.usable_for_action = true;
    return value;
}

static ReasoningRequest request(WorldSnapshot world, AgentNeedState needs, std::vector<KnowledgeClaim> claims) {
    ReasoningRequest req;
    req.request_id = "request.p39";
    req.actor_key = needs.actor_key;
    req.world_snapshot = std::move(world);
    req.need_state = needs;
    req.agent_knowledge = std::move(claims);
    req.trigger_key = "test";
    return req;
}

static void effect_semantics_registry_validation() {
    EffectSemanticsRegistry registry;
    assert(registry.validateAll().is_ok());
    assert(registry.findByEffectKey("restore_hunger").is_ok());
    assert(registry.findByEffectKey("build_house").is_ok());
    EffectSemantics duplicate = registry.findByEffectKey("restore_hunger").value();
    assert(registry.registerDefinition(duplicate).is_error());
    std::cout << "effect_semantics_registry_validation passed\n";
}

static void p41_execution_enums_and_dto_validation() {
    assert(toString(EffectExecutionOpKind::ResolveThreat) == "resolve_threat");
    assert(toString(EffectExecutionOpKind::ApplyStatusEffect) == "apply_status_effect");
    assert(toString(EffectExecutionTargetKind::KnowledgeGraph) == "knowledge_graph");
    assert(toString(WorldScopeKind::Region) == "region");
    assert(toString(EffectOperationGroupKind::SideEffect) == "side_effect");
    assert(toString(OperationFailurePolicy::ContinueWithWarning) == "continue_with_warning");
    assert(toString(TemporalEffectKind::Periodic) == "periodic");
    assert(toString(TargetSelectionKind::AllInScope) == "all_in_scope");
    assert(effectExecutionOpKindFromString("resolve_threat").is_ok());
    assert(effectExecutionOpKindFromString("apply_status_effect").is_ok());
    assert(effectExecutionTargetKindFromString("civilization").is_ok());
    assert(worldScopeKindFromString("world_system").is_ok());
    assert(effectOperationGroupKindFromString("risk").is_ok());
    assert(operationFailurePolicyFromString("queue_for_retry").is_ok());
    assert(temporalEffectKindFromString("delayed").is_ok());
    assert(targetSelectionKindFromString("random_in_scope").is_ok());
    assert(effectExecutionOpKindFromString("bad_value").is_error());
    EffectExecutionOperation invalid;
    assert(invalid.validateBasic().is_error());
    EffectExecutionOperation operation;
    operation.operation_id = "op.test";
    operation.op_kind = EffectExecutionOpKind::EmitWorldEvent;
    operation.target_kind = EffectExecutionTargetKind::ActorSelf;
    operation.key_source = ExecutionValueSourceKind::Constant;
    operation.safe_summary_zh_cn = "安全摘要。";
    assert(operation.validateBasic().is_ok());
    operation.safe_summary_zh_cn = "raw_state 泄漏";
    assert(operation.validateBasic().is_error());
    std::cout << "p41_execution_enums_and_dto_validation passed\n";
}

static void p41_execution_registry_validation() {
    EffectSemanticsRegistry semantics;
    EffectExecutionSpecRegistry specs;
    assert(specs.validateAgainst(semantics).is_ok());
    assert(specs.findByEffectKey("make_torch").is_ok());
    assert(specs.findByEffectKey("repel_beast").is_ok());
    auto duplicate = specs.findByEffectKey("make_torch").value();
    assert(specs.registerSpec(duplicate).is_error());
    EffectExecutionSpecRegistry empty(false);
    assert(empty.findByEffectKey("make_torch").is_error());
    std::cout << "p41_execution_registry_validation passed\n";
}

static void p41_world_effect_executor_make_torch_atomic() {
    auto world = snapshot();
    world.objects_by_key["wood"] = object("wood", 1);
    world.objects_by_key["wood"].numeric_states["processed"] = 1;
    world.objects_by_key["camp_fire"] = object("camp_fire", 1);
    world.objects_by_key["torch"] = object("torch", 0);
    EffectExecutionSpecRegistry specs;
    WorldEffectExecutor executor;
    pathfinder::condition::ConditionExpressionEvaluator evaluator;
    WorldExecutionRequest req;
    req.request_id = "p41.make_torch";
    req.actor_key = "companion";
    req.effect_key = "make_torch";
    req.object_key = "wood";
    auto result = executor.execute(world, req, specs, evaluator);
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().changes.size() == 2);

    world.objects_by_key["wood"].numeric_states["processed"] = 0;
    auto failed = executor.execute(world, req, specs, evaluator);
    assert(failed.is_ok());
    assert(!failed.value().ok);
    assert(failed.value().failure_kind == ExecutionFailureKind::ConditionNotMet);
    assert(failed.value().changes.empty());
    std::cout << "p41_world_effect_executor_make_torch_atomic passed\n";
}

static void p41_agent_step_executor_blocks_without_torch() {
    auto world = snapshot();
    world.objects_by_key["torch"] = object("torch", 0);
    WorldThreatRuntimeState threat;
    threat.threat_key = "beast_shadow";
    threat.display_name_zh_cn = "野兽影子";
    threat.phase = ThreatEventPhase::Approaching;
    threat.level = 75;
    threat.active = true;
    world.threats_by_key[threat.threat_key] = threat;
    EffectSemanticsRegistry semantics;
    AgentPlanStep step;
    step.step_id = "step.p41.repel";
    step.kind = PlanStepKind::DirectAction;
    step.actor_key = "companion";
    step.object_key = "torch";
    step.target_key = "beast_shadow";
    step.action_key = "use";
    step.effect_key = "repel_beast";
    step.expected_semantics = {semantics.findByEffectKey("repel_beast").value()};
    step.explanation_zh_cn = "举起火把驱赶正在靠近的野兽。";
    EffectExecutionSpecRegistry specs;
    AgentPlanStepExecutor executor;
    AgentStepExecutionRequest req;
    req.request_id = "p41.agent.step";
    req.actor_key = "companion";
    req.plan_id = "plan.p41";
    req.step = step;
    req.mode = AgentStepExecutionMode::ExecuteOneStep;
    auto result = executor.executeStep(world, req, specs);
    assert(result.is_ok());
    assert(!result.value().ok);
    assert(result.value().should_replan);
    assert(result.value().world_result.changes.empty());
    std::cout << "p41_agent_step_executor_blocks_without_torch passed\n";
}


static WorldExecutionResult dryRunFutureSpec(EffectExecutionSpec spec_value) {
    EffectExecutionSpecRegistry specs(false);
    assert(specs.registerSpec(spec_value).is_ok());
    WorldEffectExecutor executor;
    pathfinder::condition::ConditionExpressionEvaluator evaluator;
    WorldExecutionRequest req;
    req.request_id = "p41.future." + spec_value.effect_key;
    req.actor_key = "companion";
    req.effect_key = spec_value.effect_key;
    req.dry_run = true;
    auto result = executor.execute(snapshot(), req, specs, evaluator);
    assert(result.is_ok());
    return result.value();
}

static EffectExecutionOperation futureOperation(std::string id, EffectExecutionOpKind op_kind, EffectExecutionTargetKind target_kind, EffectOperationGroupKind group_kind) {
    EffectExecutionOperation operation;
    operation.operation_id = std::move(id);
    operation.op_kind = op_kind;
    operation.target_kind = target_kind;
    operation.group_kind = group_kind;
    operation.failure_policy = OperationFailurePolicy::ContinueWithWarning;
    operation.safe_summary_zh_cn = "长期效果 dry-run 校验。";
    operation.trace_keys = {"p41.future:" + operation.operation_id};
    return operation;
}

static EffectExecutionSpec futureSpec(std::string effect_key, std::vector<EffectExecutionOperation> operations) {
    EffectExecutionSpec spec_value;
    spec_value.spec_id = "spec." + effect_key;
    spec_value.effect_key = std::move(effect_key);
    spec_value.operations = std::move(operations);
    spec_value.safe_summary_zh_cn = "长期世界效果 dry-run 通过。";
    spec_value.source_config_id = "test.p41.future";
    spec_value.unsupported_features = {"dry_run_only_until_world_systems_are_connected"};
    return spec_value;
}

static void p41_magic_fireball_area_side_effect_dry_run() {
    auto burn = futureOperation("burn_status", EffectExecutionOpKind::ApplyStatusEffect, EffectExecutionTargetKind::Region, EffectOperationGroupKind::Primary);
    burn.world_scope.kind = WorldScopeKind::Region;
    burn.world_scope.scope_key = "region.old_forest";
    burn.target_selection.kind = TargetSelectionKind::AllInScope;
    burn.target_selection.scope = burn.world_scope;
    burn.target_selection.required_tags = {"flammable"};
    burn.target_selection.max_targets = 12;
    burn.world_rule_payload.rule_key = "regional_burning";
    burn.world_rule_payload.safe_summary_zh_cn = "区域目标被附加燃烧状态。";
    auto pressure = futureOperation("wildfire_pressure", EffectExecutionOpKind::ChangeSystemPressure, EffectExecutionTargetKind::WorldSystem, EffectOperationGroupKind::SideEffect);
    pressure.world_scope.kind = WorldScopeKind::WorldSystem;
    pressure.world_scope.scope_key = "system.ecology";
    pressure.world_rule_payload.system_key = "ecology";
    pressure.world_rule_payload.pressure_delta = 15.0;
    auto result = dryRunFutureSpec(futureSpec("magic_fireball_area", {burn, pressure}));
    assert(result.ok && !result.executed && result.changes.empty());
    std::cout << "p41_magic_fireball_area_side_effect_dry_run passed\n";
}

static void p41_mechanical_replication_system_pressure_dry_run() {
    auto replicate = futureOperation("replication_loop", EffectExecutionOpKind::StartPeriodicEffect, EffectExecutionTargetKind::WorldSystem, EffectOperationGroupKind::Scheduled);
    replicate.temporal_policy.kind = TemporalEffectKind::Periodic;
    replicate.temporal_policy.period_ticks = 6;
    replicate.random_policy.enabled = true;
    replicate.random_policy.random_stream_key = "stream.machine_replication";
    replicate.random_policy.chance = 0.35;
    replicate.random_policy.outcome_keys = {"replicate", "stall"};
    replicate.world_rule_payload.system_key = "machine_swarm";
    replicate.world_rule_payload.pressure_delta = 8.0;
    auto result = dryRunFutureSpec(futureSpec("mechanical_replication_pressure", {replicate}));
    assert(result.ok && !result.executed && result.changes.empty());
    std::cout << "p41_mechanical_replication_system_pressure_dry_run passed\n";
}

static void p41_tribe_capability_unlock_dry_run() {
    auto unlock = futureOperation("unlock_smelting", EffectExecutionOpKind::UnlockCapability, EffectExecutionTargetKind::Civilization, EffectOperationGroupKind::Primary);
    unlock.world_scope.kind = WorldScopeKind::Civilization;
    unlock.world_scope.scope_key = "civilization.river_tribe";
    unlock.world_rule_payload.capability_key = "capability.smelting";
    unlock.world_rule_payload.safe_summary_zh_cn = "族群解锁冶炼能力。";
    auto result = dryRunFutureSpec(futureSpec("tribe_unlock_smelting", {unlock}));
    assert(result.ok && !result.executed && result.changes.empty());
    std::cout << "p41_tribe_capability_unlock_dry_run passed\n";
}

static void p41_diplomacy_agreement_dto_validation() {
    auto agreement = futureOperation("create_trade_pact", EffectExecutionOpKind::CreateAgreement, EffectExecutionTargetKind::Agreement, EffectOperationGroupKind::Social);
    agreement.relationship_payload.source_actor_key = "faction.river_tribe";
    agreement.relationship_payload.target_actor_key = "faction.hill_tribe";
    agreement.relationship_payload.relationship_key = "trade_trust";
    agreement.relationship_payload.relationship_delta = 12.0;
    agreement.relationship_payload.reputation_delta = 3.0;
    agreement.relationship_payload.agreement_key = "agreement.trade_pact";
    auto spec_value = futureSpec("diplomacy_trade_agreement", {agreement});
    assert(spec_value.validateBasic().is_ok());
    auto result = dryRunFutureSpec(spec_value);
    assert(result.ok && !result.executed && result.changes.empty());
    std::cout << "p41_diplomacy_agreement_dto_validation passed\n";
}

static void p41_plague_delayed_spread_dto_validation() {
    auto spread = futureOperation("schedule_spread", EffectExecutionOpKind::ScheduleDelayedEffect, EffectExecutionTargetKind::Timeline, EffectOperationGroupKind::Scheduled);
    spread.temporal_policy.kind = TemporalEffectKind::Delayed;
    spread.temporal_policy.delay_ticks = 24;
    spread.scheduled_effect.schedule_id = "schedule.plague.spread";
    spread.scheduled_effect.effect_key = "plague_spread_tick";
    spread.scheduled_effect.due_tick = 240;
    spread.scheduled_effect.scope.kind = WorldScopeKind::Region;
    spread.scheduled_effect.scope.scope_key = "region.delta";
    auto spec_value = futureSpec("plague_delayed_spread", {spread});
    spec_value.temporal_policy.kind = TemporalEffectKind::Delayed;
    spec_value.temporal_policy.delay_ticks = 24;
    spec_value.scheduled_effects = {spread.scheduled_effect};
    assert(spec_value.validateBasic().is_ok());
    auto result = dryRunFutureSpec(spec_value);
    assert(result.ok && !result.executed && result.changes.empty());
    std::cout << "p41_plague_delayed_spread_dto_validation passed\n";
}

static void goal_generation_hunger() {
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.hunger = 80;
    AgentGoalGenerator generator;
    auto result = generator.generateGoals({needs, snapshot()});
    assert(result.is_ok());
    assert(!result.value().empty());
    assert(result.value().front().kind == AgentGoalKind::ReduceHunger);
    std::cout << "goal_generation_hunger passed\n";
}

static void candidate_from_owned_knowledge_only() {
    auto world = snapshot();
    world.objects_by_key["red_berry"] = object("red_berry", 1);
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.hunger = 80;
    AgentGoal goal;
    goal.goal_id = "goal.companion.reduce_hunger.1";
    goal.actor_key = "companion";
    goal.kind = AgentGoalKind::ReduceHunger;
    goal.urgency = 80;
    goal.desired_delta = -30;
    goal.source_keys = {"need:hunger"};
    KnowledgeActionCandidateBuilder builder;
    auto built = builder.buildCandidates({"companion", goal, world, {claim("other", "red_berry", "eat", "restore_hunger")}, {}});
    assert(built.is_ok());
    assert(built.value().empty());
    std::cout << "candidate_from_owned_knowledge_only passed\n";
}

static void hungry_agent_selects_known_safe_food() {
    auto world = snapshot();
    world.objects_by_key["red_berry"] = object("red_berry", 2);
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.hunger = 80;
    AgentReasoner reasoner;
    auto result = reasoner.reason(request(world, needs, {claim("companion", "red_berry", "eat", "restore_hunger")}));
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().selected_plan->steps.front().effect_key == "restore_hunger");
    assert(result.value().selected_plan->steps.front().object_key == "red_berry");
    std::cout << "hungry_agent_selects_known_safe_food passed\n";
}

static void poison_food_rejected() {
    auto world = snapshot();
    world.objects_by_key["rotten_berry"] = object("rotten_berry", 1);
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.hunger = 80;
    AgentReasoner reasoner;
    auto result = reasoner.reason(request(world, needs, {claim("companion", "rotten_berry", "eat", "poison")}));
    assert(result.is_ok());
    assert(!result.value().ok);
    std::cout << "poison_food_rejected passed\n";
}

static void cold_agent_prefers_fire_when_urgent() {
    auto world = snapshot();
    world.objects_by_key["dry_grass"] = object("dry_grass", 1);
    world.objects_by_key["wood_processed"] = object("wood_processed", 3);
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.cold = 85;
    AgentReasoner reasoner;
    auto result = reasoner.reason(request(world, needs, {
        claim("companion", "dry_grass", "use", "ignite_fire"),
        claim("companion", "wood_processed", "use", "build_house")
    }));
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().selected_plan->steps.front().effect_key == "ignite_fire");
    assert(result.value().safe_explanation_zh_cn.find("最快改善寒冷") != std::string::npos);
    std::cout << "cold_agent_prefers_fire_when_urgent passed\n";
}

static void shelter_shortage_build_house_plan() {
    auto world = snapshot();
    world.objects_by_key["tribe_status"] = object("tribe_status", 1);
    world.objects_by_key["tribe_status"].numeric_states["population"] = 4;
    world.objects_by_key["tribe_status"].numeric_states["shelter_capacity"] = 2;
    world.objects_by_key["wood_processed"] = object("wood_processed", 3);
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.cold = 40;
    AgentReasoner reasoner;
    auto result = reasoner.reason(request(world, needs, {claim("companion", "wood_processed", "use", "build_house")}));
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().selected_plan->goal.kind == AgentGoalKind::IncreaseShelterCapacity);
    assert(result.value().selected_plan->steps.back().effect_key == "build_house");
    std::cout << "shelter_shortage_build_house_plan passed\n";
}

static void build_house_requires_wood_chain() {
    auto world = snapshot();
    world.objects_by_key["tribe_status"] = object("tribe_status", 1);
    world.objects_by_key["tribe_status"].numeric_states["population"] = 4;
    world.objects_by_key["tribe_status"].numeric_states["shelter_capacity"] = 2;
    world.objects_by_key["wood"] = object("wood", 3);
    world.objects_by_key["wood_processed"] = object("wood_processed", 0);
    world.objects_by_key["axe"] = object("axe", 1);
    world.objects_by_key["axe"].numeric_states["sharpness"] = 2;
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.cold = 40;
    AgentReasoner reasoner;
    auto result = reasoner.reason(request(world, needs, {
        claim("companion", "wood_processed", "use", "build_house"),
        claim("companion", "wood", "use", "cut_wood")
    }));
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().selected_plan->steps.front().effect_key == "cut_wood");
    assert(result.value().selected_plan->steps.back().effect_key == "build_house");
    std::cout << "build_house_requires_wood_chain passed\n";
}

static void dull_axe_requires_sharpen_chain() {
    auto world = snapshot();
    world.objects_by_key["tribe_status"] = object("tribe_status", 1);
    world.objects_by_key["tribe_status"].numeric_states["population"] = 4;
    world.objects_by_key["tribe_status"].numeric_states["shelter_capacity"] = 2;
    world.objects_by_key["wood"] = object("wood", 3);
    world.objects_by_key["wood_processed"] = object("wood_processed", 0);
    world.objects_by_key["axe"] = object("axe", 1);
    world.objects_by_key["axe"].numeric_states["sharpness"] = 0;
    world.objects_by_key["whetstone"] = object("whetstone", 1);
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.cold = 40;
    AgentReasoner reasoner;
    auto result = reasoner.reason(request(world, needs, {
        claim("companion", "wood_processed", "use", "build_house"),
        claim("companion", "wood", "use", "cut_wood"),
        claim("companion", "whetstone", "use", "restore_sharpness")
    }));
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().selected_plan->steps.front().effect_key == "restore_sharpness");
    std::cout << "dull_axe_requires_sharpen_chain passed\n";
}

static void beast_threat_selects_torch_repel() {
    auto world = snapshot();
    world.objects_by_key["torch"] = object("torch", 1);
    WorldThreatRuntimeState threat;
    threat.threat_key = "beast_shadow";
    threat.display_name_zh_cn = "野兽影子";
    threat.phase = ThreatEventPhase::Approaching;
    threat.level = 75;
    threat.active = true;
    world.threats_by_key[threat.threat_key] = threat;
    AgentNeedState needs;
    needs.actor_key = "companion";
    AgentReasoner reasoner;
    auto result = reasoner.reason(request(world, needs, {claim("companion", "torch", "use", "repel_beast")}));
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().selected_plan->steps.front().effect_key == "repel_beast");
    std::cout << "beast_threat_selects_torch_repel passed\n";
}

static void make_torch_uses_reaction_rule_preconditions() {
    auto world = snapshot();
    world.objects_by_key["wood_processed"] = object("wood_processed", 1);
    world.objects_by_key["camp_fire"] = object("camp_fire", 1);
    WorldThreatRuntimeState threat;
    threat.threat_key = "beast_shadow";
    threat.display_name_zh_cn = "野兽影子";
    threat.phase = ThreatEventPhase::Approaching;
    threat.level = 75;
    threat.active = true;
    world.threats_by_key[threat.threat_key] = threat;
    AgentNeedState needs;
    needs.actor_key = "companion";
    AgentReasoner reasoner;
    auto result = reasoner.reason(request(world, needs, {
        claim("companion", "torch", "use", "repel_beast"),
        claim("companion", "wood_processed", "use", "make_torch")
    }));
    assert(result.is_ok());
    assert(result.value().ok);
    assert(result.value().selected_plan->steps.front().effect_key == "make_torch");
    assert(result.value().selected_plan->steps.back().effect_key == "repel_beast");
    std::cout << "make_torch_uses_reaction_rule_preconditions passed\n";
}

static void reaction_adapter_reads_crafting_rules() {
    ReactionPlanningAdapter adapter;
    auto torch = adapter.preconditionsForEffect("make_torch");
    auto cut = adapter.preconditionsForEffect("cut_wood");
    auto sharpen = adapter.preconditionsForEffect("restore_sharpness");
    assert(torch.is_ok());
    assert(cut.is_ok());
    assert(sharpen.is_ok());
    assert(torch.value().size() == 2);
    assert(cut.value().size() == 3);
    assert(sharpen.value().size() == 2);
    auto has = [](const auto& values, const std::string& domain, const std::string& key) {
        return std::any_of(values.begin(), values.end(), [&](const auto& item) {
            return item.missing_domain == domain && item.missing_key == key;
        });
    };
    assert(has(torch.value(), "object.quantity", "wood_processed"));
    assert(has(torch.value(), "object.quantity", "camp_fire"));
    assert(has(cut.value(), "object.quantity", "wood"));
    assert(has(cut.value(), "object.quantity", "axe"));
    assert(has(cut.value(), "object.state", "axe.sharpness"));
    assert(has(sharpen.value(), "object.quantity", "whetstone"));
    assert(has(sharpen.value(), "object.quantity", "axe"));
    std::cout << "reaction_adapter_reads_crafting_rules passed\n";
}

static void cycle_detection_blocks_plan() {
    auto world = snapshot();
    AgentGoal goal;
    goal.goal_id = "goal.companion.reduce_threat.1";
    goal.actor_key = "companion";
    goal.kind = AgentGoalKind::ReduceThreat;
    goal.target_key = "beast_shadow";
    goal.urgency = 90;
    goal.desired_delta = -100;
    goal.source_keys = {"threat:beast_shadow"};

    EffectSemanticsRegistry registry;
    ActionCandidate cyclic;
    cyclic.candidate_id = "candidate.cycle";
    cyclic.actor_key = "companion";
    cyclic.source_knowledge_id = "knowledge.cycle";
    cyclic.object_key = "torch";
    cyclic.target_key = "beast_shadow";
    cyclic.action_key = "use";
    cyclic.effect_key = "repel_beast";
    cyclic.semantics = registry.findByEffectKey("make_torch").value();
    cyclic.knowledge_usability = KnowledgeUsability::Usable;
    cyclic.knowledge_confidence = 0.9;

    PlanPreconditionResolver resolver;
    auto plan = resolver.resolvePreconditions({goal, cyclic, {cyclic}, world, {}});
    assert(plan.is_ok());
    assert(plan.value().blocked);
    assert(plan.value().selection_reason == PlanSelectionReason::CycleDetected);
    std::cout << "cycle_detection_blocks_plan passed\n";
}

static void search_limit_returns_diagnostic() {
    auto world = snapshot();
    world.objects_by_key["tribe_status"] = object("tribe_status", 1);
    world.objects_by_key["tribe_status"].numeric_states["population"] = 4;
    world.objects_by_key["tribe_status"].numeric_states["shelter_capacity"] = 2;
    world.objects_by_key["wood"] = object("wood", 3);
    world.objects_by_key["wood_processed"] = object("wood_processed", 0);
    world.objects_by_key["axe"] = object("axe", 1);
    world.objects_by_key["axe"].numeric_states["sharpness"] = 2;
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.cold = 40;
    auto req = request(world, needs, {
        claim("companion", "wood_processed", "use", "build_house"),
        claim("companion", "wood", "use", "cut_wood")
    });
    req.options.max_total_expansions = 1;
    AgentReasoner reasoner;
    auto result = reasoner.reason(req);
    assert(result.is_ok());
    assert(!result.value().ok);
    assert(result.value().trace.limit_reached);
    std::cout << "search_limit_returns_diagnostic passed\n";
}

static void h5_projection_safe() {
    auto world = snapshot();
    world.objects_by_key["dry_grass"] = object("dry_grass", 1);
    AgentNeedState needs;
    needs.actor_key = "companion";
    needs.cold = 85;
    AgentReasoner reasoner;
    auto result = reasoner.reason(request(world, needs, {claim("companion", "dry_grass", "use", "ignite_fire")}));
    assert(result.is_ok());
    ReasoningProjectionMapper mapper;
    auto projection = mapper.buildProjection(result.value());
    assert(projection.is_ok());
    assert(projection.value().public_reason_lines_zh_cn.front().find("utility_score") == std::string::npos);
    assert(projection.value().public_reason_lines_zh_cn.front().find("hidden_truth") == std::string::npos);
    assert(projection.value().public_reason_lines_zh_cn.front().find("raw_state") == std::string::npos);
    std::cout << "h5_projection_safe passed\n";
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "effect_semantics_registry_validation") effect_semantics_registry_validation();
    else if (mode == "p41_execution_enums_and_dto_validation") p41_execution_enums_and_dto_validation();
    else if (mode == "p41_execution_registry_validation") p41_execution_registry_validation();
    else if (mode == "p41_world_effect_executor_make_torch_atomic") p41_world_effect_executor_make_torch_atomic();
    else if (mode == "p41_agent_step_executor_blocks_without_torch") p41_agent_step_executor_blocks_without_torch();
    else if (mode == "p41_magic_fireball_area_side_effect_dry_run") p41_magic_fireball_area_side_effect_dry_run();
    else if (mode == "p41_mechanical_replication_system_pressure_dry_run") p41_mechanical_replication_system_pressure_dry_run();
    else if (mode == "p41_tribe_capability_unlock_dry_run") p41_tribe_capability_unlock_dry_run();
    else if (mode == "p41_diplomacy_agreement_dto_validation") p41_diplomacy_agreement_dto_validation();
    else if (mode == "p41_plague_delayed_spread_dto_validation") p41_plague_delayed_spread_dto_validation();
    else if (mode == "goal_generation_hunger") goal_generation_hunger();
    else if (mode == "candidate_from_owned_knowledge_only") candidate_from_owned_knowledge_only();
    else if (mode == "hungry_agent_selects_known_safe_food") hungry_agent_selects_known_safe_food();
    else if (mode == "poison_food_rejected") poison_food_rejected();
    else if (mode == "cold_agent_prefers_fire_when_urgent") cold_agent_prefers_fire_when_urgent();
    else if (mode == "shelter_shortage_build_house_plan") shelter_shortage_build_house_plan();
    else if (mode == "build_house_requires_wood_chain") build_house_requires_wood_chain();
    else if (mode == "dull_axe_requires_sharpen_chain") dull_axe_requires_sharpen_chain();
    else if (mode == "beast_threat_selects_torch_repel") beast_threat_selects_torch_repel();
    else if (mode == "make_torch_uses_reaction_rule_preconditions") make_torch_uses_reaction_rule_preconditions();
    else if (mode == "reaction_adapter_reads_crafting_rules") reaction_adapter_reads_crafting_rules();
    else if (mode == "cycle_detection_blocks_plan") cycle_detection_blocks_plan();
    else if (mode == "search_limit_returns_diagnostic") search_limit_returns_diagnostic();
    else if (mode == "h5_projection_safe") h5_projection_safe();
    else {
        effect_semantics_registry_validation();
        p41_execution_enums_and_dto_validation();
        p41_execution_registry_validation();
        p41_world_effect_executor_make_torch_atomic();
        p41_agent_step_executor_blocks_without_torch();
        p41_magic_fireball_area_side_effect_dry_run();
        p41_mechanical_replication_system_pressure_dry_run();
        p41_tribe_capability_unlock_dry_run();
        p41_diplomacy_agreement_dto_validation();
        p41_plague_delayed_spread_dto_validation();
        goal_generation_hunger();
        candidate_from_owned_knowledge_only();
        hungry_agent_selects_known_safe_food();
        poison_food_rejected();
        cold_agent_prefers_fire_when_urgent();
        shelter_shortage_build_house_plan();
        build_house_requires_wood_chain();
        dull_axe_requires_sharpen_chain();
        beast_threat_selects_torch_repel();
        make_torch_uses_reaction_rule_preconditions();
        reaction_adapter_reads_crafting_rules();
        cycle_detection_blocks_plan();
        search_limit_returns_diagnostic();
        h5_projection_safe();
    }
    return 0;
}
