#include "pathfinder/goal_execution/goal_execution_system.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include "pathfinder/world_interaction/world_services.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>

using namespace pathfinder::agent_reasoning;
using namespace pathfinder::foundation;
using namespace pathfinder::goal_execution;
using namespace pathfinder::knowledge;
using namespace pathfinder::world_interaction;

static WorldObjectInstance object(std::string key, int quantity) {
    WorldObjectInstance item;
    item.instance_id = key + ".instance";
    item.definition_key = key;
    item.display_name_zh_cn = key;
    item.kind = WorldObjectInstanceKind::ResourceStack;
    item.quantity = quantity;
    item.location_key = "forest";
    return item;
}

static WorldSnapshot world() {
    WorldSnapshot snapshot;
    snapshot.snapshot_id = "snapshot.p40";
    snapshot.scenario_key = "p40";
    snapshot.turn_index = 1;
    snapshot.location_key = "forest";
    WorldActorRuntimeState actor;
    actor.actor_key = "companion";
    actor.display_name_zh_cn = "同伴";
    snapshot.actors_by_key[actor.actor_key] = actor;
    return snapshot;
}

static KnowledgeClaim claim(std::string effect_key) {
    KnowledgeClaim value;
    value.knowledge_id = KnowledgeId("knowledge.companion." + effect_key);
    value.owner.kind = KnowledgeOwnerKind::Agent;
    value.owner.entity_id = EntityId("companion");
    value.subject.kind = KnowledgeSubjectKind::ObjectInstance;
    value.subject.subject_id = "torch";
    value.subject.subject_type_key = "world_object";
    value.predicate.relation_type = KnowledgeRelationType::HasEffect;
    value.predicate.action_key = "use";
    value.predicate.effect_key = effect_key;
    value.confidence.confidence = 0.9;
    value.confidence.band = KnowledgeConfidenceBand::Reliable;
    value.status = KnowledgeStatus::Active;
    value.projection_flags.usable_by_ai = true;
    value.projection_flags.usable_for_action = true;
    return value;
}

static PlanPrecondition precondition(std::string domain, std::string key, std::string required, bool can_plan = false) {
    PlanPrecondition value;
    value.condition_id = "condition." + domain + "." + key;
    value.condition_expression = "condition:test:eq:" + domain + "." + key + "." + required;
    value.missing_domain = std::move(domain);
    value.missing_key = std::move(key);
    value.required_value = std::move(required);
    value.can_be_planned = can_plan;
    return value;
}

static ExecutionContext context(ActionDriverKind kind = ActionDriverKind::ChopWood) {
    ExecutionContext ctx;
    ctx.context_id = "execution.companion";
    ctx.actor_key = "companion";
    ctx.capability_tier = AgentCapabilityTier::CognitiveAgent;
    GoalFrame frame;
    frame.frame_id = "frame.chop";
    frame.actor_key = "companion";
    frame.goal_id = "goal.chop";
    frame.plan_id = "plan.chop";
    frame.status = ExecutionFrameStatus::Running;
    frame.priority = 50;
    frame.created_tick = 1;
    frame.public_reason_zh_cn = "为建房准备木头";
    ctx.goal_stack.push_back(frame);
    ctx.active_frame_id = frame.frame_id;
    DriverExecutionState state;
    state.driver_state_id = "driver.chop";
    state.driver_kind = kind;
    state.action_key = "use";
    if (kind == ActionDriverKind::BuildStructure) {
        state.effect_key = "build_house";
    } else if (kind == ActionDriverKind::CounterThreat) {
        state.effect_key = "repel_beast";
        state.object_key = "torch";
        state.preconditions = {precondition("object.quantity", "torch", "gte.1")};
    } else {
        state.effect_key = "cut_wood";
        state.object_key = "axe";
        state.preconditions = {
            precondition("object.quantity", "wood", "gte.1"),
            precondition("object.quantity", "axe", "gte.1"),
            precondition("object.state", "axe.sharpness", "gte.1", true)
        };
    }
    state.target_key = kind == ActionDriverKind::CounterThreat ? std::optional<std::string>("wolf") : std::optional<std::string>("wood");
    state.current_location_key = "forest";
    state.required_location_key = "forest";
    ctx.active_driver_state = state;
    return ctx;
}

static ExecutionTickRequest request(ExecutionContext ctx, WorldSnapshot snapshot, std::vector<ExternalInterruptSignal> signals = {}) {
    ExecutionTickRequest req;
    req.request_id = "tick.p40";
    req.context = std::move(ctx);
    req.world_snapshot = std::move(snapshot);
    req.visible_events = std::move(signals);
    req.elapsed_ticks = 1;
    req.tick = 2;
    return req;
}

static ExternalInterruptSignal wolf() {
    ExternalInterruptSignal signal;
    signal.interrupt_id = "interrupt.wolf";
    signal.kind = ExternalInterruptKind::ThreatAppeared;
    signal.source_event_id = "event.wolf";
    signal.location_key = "forest";
    signal.target_actor_keys = {"companion"};
    signal.threat_key = "wolf";
    signal.severity = 95;
    signal.urgency = 95;
    signal.can_be_ignored = false;
    signal.public_summary_zh_cn = "狼靠近树林。";
    return signal;
}

static void stack() {
    GoalStackManager manager;
    ExecutionContext ctx;
    ctx.actor_key = "companion";
    GoalFrame frame;
    frame.frame_id = "frame.a";
    frame.actor_key = "companion";
    frame.goal_id = "goal.a";
    frame.plan_id = "plan.a";
    assert(manager.pushGoal(ctx, frame).is_ok());
    assert(ctx.active_frame_id == "frame.a");
    DriverExecutionState state;
    state.driver_state_id = "driver.a";
    state.driver_kind = ActionDriverKind::Gather;
    ctx.active_driver_state = state;
    assert(manager.pauseFrame(ctx, "frame.a", "test_pause", 3).is_ok());
    assert(ctx.paused_contexts.size() == 1);
    assert(manager.resumeFrame(ctx, "frame.a").is_ok());
    assert(ctx.active_driver_state->driver_kind == ActionDriverKind::Gather);
    assert(manager.cancelFrame(ctx, "frame.a").is_ok());
}

static void registry() {
    ActionDriverRegistry registry;
    assert(registry.registerDefaultDrivers().is_ok());
    assert(registry.validateAll().is_ok());
    assert(registry.find(ActionDriverKind::ChopWood).is_ok());
}

static void dull_axe() {
    auto snapshot = world();
    snapshot.objects_by_key["axe"] = object("axe", 1);
    snapshot.objects_by_key["axe"].numeric_states["sharpness"] = 0;
    snapshot.objects_by_key["wood"] = object("wood", 2);
    GoalExecutionSystem system;
    auto result = system.tick(request(context(), snapshot));
    assert(result.is_ok());
    assert(result.value().requires_reasoning);
    assert(!result.value().internal_blockers.empty());
    assert(result.value().internal_blockers.front().kind == InternalBlockerKind::ToolStateInsufficient);
    assert(!result.value().generated_subgoals.empty());
    assert(result.value().generated_subgoals.front().kind == AgentGoalKind::RestoreToolState);
    assert(result.value().updated_context.goal_stack.size() == 2);
    assert(result.value().updated_context.goal_stack.back().parent_frame_id == "frame.chop");
}

static void blocker_subgoal() {
    InternalBlocker blocker;
    blocker.blocker_id = "blocker.axe";
    blocker.actor_key = "companion";
    blocker.kind = InternalBlockerKind::ToolStateInsufficient;
    blocker.missing_key = "axe";
    blocker.can_generate_subgoal = true;
    blocker.suggested_goal_kind = AgentGoalKind::RestoreToolState;
    InternalBlockerResolver resolver;
    auto goals = resolver.resolve({context(), {blocker}});
    assert(goals.is_ok());
    assert(goals.value().size() == 1);
    assert(goals.value().front().kind == AgentGoalKind::RestoreToolState);
}

static void wolf_interrupt() {
    auto snapshot = world();
    snapshot.objects_by_key["axe"] = object("axe", 1);
    snapshot.objects_by_key["axe"].numeric_states["sharpness"] = 3;
    snapshot.objects_by_key["wood"] = object("wood", 2);
    GoalExecutionSystem system;
    auto result = system.tick(request(context(), snapshot, {wolf()}));
    assert(result.is_ok());
    assert(result.value().interrupt_decisions.front().kind == InterruptDecisionKind::PauseAndInsertEmergencyGoal);
    assert(result.value().updated_context.paused_contexts.size() == 1);
    assert(result.value().updated_context.active_driver_state->driver_kind == ActionDriverKind::CounterThreat);
    assert(result.value().commands_to_resolve.empty());
}

static void resume() {
    auto ctx = context();
    GoalStackManager manager;
    assert(manager.pauseFrame(ctx, "frame.chop", "wolf", 2).is_ok());
    auto snapshot = world();
    snapshot.objects_by_key["axe"] = object("axe", 1);
    snapshot.objects_by_key["axe"].numeric_states["sharpness"] = 2;
    snapshot.objects_by_key["wood"] = object("wood", 2);
    GoalExecutionSystem system;
    auto result = system.tick(request(ctx, snapshot));
    assert(result.is_ok());
    assert(result.value().updated_context.active_frame_id == "frame.chop");
    assert(result.value().updated_context.active_driver_state.has_value());
    assert(result.value().updated_context.active_driver_state->driver_kind == ActionDriverKind::ChopWood);
    assert(result.value().updated_context.paused_contexts.empty());
}

static void command_adapter() {
    DriverCommand command;
    command.command_id = "command.cut";
    command.actor_key = "companion";
    command.driver_kind = ActionDriverKind::ChopWood;
    command.object_key = "axe";
    command.target_key = "wood";
    command.action_key = "use";
    command.effect_key = "cut_wood";
    DriverCommandAdapter adapter;
    auto adapted = adapter.adapt({command});
    assert(adapted.is_ok());
    assert(adapted.value().front().effect_key == "cut_wood");
    assert(adapted.value().front().object_key == "axe");
}

static void projection_safe() {
    auto snapshot = world();
    GoalExecutionSystem system;
    auto result = system.tick(request(context(), snapshot, {wolf()}));
    assert(result.is_ok());
    const auto text = result.value().projection.interrupt_reason.text_zh_cn + result.value().projection.response_plan.text_zh_cn;
    assert(text.find("score") == std::string::npos);
    assert(text.find("95") == std::string::npos);
}

static void materials() {
    auto snapshot = world();
    snapshot.objects_by_key["wood"] = object("wood", 3);
    MaterialRequirementSet set;
    set.set_id = "set.build";
    set.owner_step_id = "step.build";
    set.requirements.push_back(MaterialRequirement{"req.wood", "wood", 5});
    ResourceReservation reservation;
    reservation.reservation_id = "reservation.fire.wood";
    reservation.resource_key = "wood";
    reservation.quantity = 2;
    MaterialRequirementEvaluator evaluator;
    auto evaluated = evaluator.evaluate({snapshot, {reservation}, set});
    assert(evaluated.is_ok());
    assert(!evaluated.value().satisfied);
    assert(evaluated.value().availability.front().available_quantity == 1);
    assert(evaluated.value().availability.front().missing_quantity == 4);

    snapshot.objects_by_key["dry_branch"] = object("dry_branch", 1);
    snapshot.objects_by_key["dry_branch"].state_tags = {"wet"};
    MaterialRequirementSet dry_set;
    dry_set.set_id = "set.dry";
    dry_set.owner_step_id = "step.fire";
    dry_set.requirements.push_back(MaterialRequirement{"req.dry", "dry_branch", 1, {"dry"}});
    auto dry = evaluator.evaluate({snapshot, {}, dry_set});
    assert(dry.is_ok());
    assert(dry.value().availability.front().blocked_by_state);
}

static void reaction_links() {
    ReactionMaterialResolver resolver;
    auto torch = resolver.findProducers(MaterialRequirement{"req.torch", "torch", 1});
    assert(torch.is_ok());
    assert(std::any_of(torch.value().begin(), torch.value().end(), [](const auto& link) { return link.output_object_key == "torch"; }));
    auto wood = resolver.findProducers(MaterialRequirement{"req.wood_processed", "wood_processed", 1});
    assert(wood.is_ok());
    assert(std::any_of(wood.value().begin(), wood.value().end(), [](const auto& link) { return link.output_object_key == "wood_processed"; }));
    auto axe = resolver.findProducers(MaterialRequirement{"req.axe", "axe", 1});
    assert(axe.is_ok());
    assert(std::any_of(axe.value().begin(), axe.value().end(), [](const auto& link) { return link.condition_expression.find("sharp") != std::string::npos || link.reaction_rule_id.find("sharpen") != std::string::npos; }));
}

static void material_tags_from_dialog_runtime_snapshot() {
    pathfinder::h5_dialog::DialogScenarioCatalog catalog;
    auto scenario = catalog.defaultScenario();
    assert(scenario.is_ok());
    pathfinder::h5_dialog::DialogSessionState state;
    state.session_id = "s_p40_material_tags";
    state.scenario_key = scenario.value().scenario_key;
    state.turn_index = 1;
    pathfinder::h5_dialog::DialogObjectRuntimeState grass;
    grass.object_key = "dry_grass";
    grass.numeric_states["quantity"] = 1.0;
    grass.tag_states = {"fuel", "wet"};
    state.object_runtime_states["dry_grass"] = grass;
    pathfinder::h5_dialog::DialogObjectRuntimeState axe;
    axe.object_key = "axe";
    axe.numeric_states["quantity"] = 1.0;
    axe.numeric_states["sharpness"] = 0.0;
    axe.tag_states = {"tool", "axe"};
    state.object_runtime_states["axe"] = axe;

    WorldSnapshotAdapter adapter;
    auto snapshot = adapter.fromDialogSession(scenario.value(), state);
    assert(snapshot.is_ok());
    assert(std::find(snapshot.value().objects_by_key["axe"].state_tags.begin(), snapshot.value().objects_by_key["axe"].state_tags.end(), "dull") != snapshot.value().objects_by_key["axe"].state_tags.end());

    MaterialRequirementSet set;
    set.set_id = "set.real_adapter_state";
    set.owner_step_id = "step.fire";
    set.requirements.push_back(MaterialRequirement{"req.dry_grass", "dry_grass", 1, {"dry"}});
    MaterialRequirementEvaluator evaluator;
    auto evaluated = evaluator.evaluate({snapshot.value(), {}, set});
    assert(evaluated.is_ok());
    assert(!evaluated.value().satisfied);
    assert(evaluated.value().availability.front().blocked_by_state);
}

static void reaction_material_resolver_no_unknown_material_for_core_rules() {
    ReactionMaterialResolver resolver;
    std::vector<MaterialRequirement> requirements = {
        MaterialRequirement{"req.torch", "torch", 1},
        MaterialRequirement{"req.wood_processed", "wood_processed", 1},
        MaterialRequirement{"req.axe", "axe", 1}
    };
    for (const auto& requirement : requirements) {
        auto links = resolver.findProducers(requirement);
        assert(links.is_ok());
        assert(!links.value().empty());
        for (const auto& link : links.value()) {
            assert(std::find(link.input_object_keys.begin(), link.input_object_keys.end(), "unknown_material") == link.input_object_keys.end());
        }
    }
}

static void known_claims() {
    auto snapshot = world();
    auto ctx = context();
    ctx.known_claims.push_back(claim("repel_beast"));
    snapshot.objects_by_key["axe"] = object("axe", 1);
    snapshot.objects_by_key["wood"] = object("wood", 2);
    GoalExecutionSystem system;
    auto result = system.tick(request(ctx, snapshot));
    assert(result.is_ok());
    assert(result.value().requires_reasoning);
    assert(result.value().reasoning_request.agent_knowledge.size() == 1);
    assert(result.value().reasoning_request.agent_knowledge.front().knowledge_id.value() == "knowledge.companion.repel_beast");
}

static void build_recipe() {
    auto snapshot = world();
    snapshot.objects_by_key["wood_processed"] = object("wood_processed", 5);
    auto ctx = context(ActionDriverKind::BuildStructure);
    GoalExecutionSystem system;
    auto missing = system.tick(request(ctx, snapshot));
    assert(missing.is_ok());
    assert(!missing.value().internal_blockers.empty());
    assert(missing.value().internal_blockers.front().kind == InternalBlockerKind::MissingCondition);

    ctx = context(ActionDriverKind::BuildStructure);
    ctx.active_driver_state->material_requirements.set_id = "recipe.house";
    ctx.active_driver_state->material_requirements.owner_step_id = "step.build";
    ctx.active_driver_state->material_requirements.requirements.push_back(MaterialRequirement{"req.processed", "wood_processed", 3});
    auto built = system.tick(request(ctx, snapshot));
    assert(built.is_ok());
    assert(!built.value().commands_to_resolve.empty());
}

static void reservation_release() {
    ExecutionContext ctx;
    ctx.actor_key = "companion";
    GoalFrame frame;
    frame.frame_id = "frame.build";
    frame.actor_key = "companion";
    frame.goal_id = "goal.build";
    frame.plan_id = "plan.build";
    GoalStackManager stack;
    assert(stack.pushGoal(ctx, frame).is_ok());
    MaterialReservationManager reservations;
    assert(reservations.reserve(ctx, {"companion", "wood", 3, "goal.build", 99}).is_ok());
    assert(stack.cancelFrame(ctx, "frame.build").is_ok());
    assert(ctx.reserved_resources.front().status == ReservationStatus::Released);
}

static void tick_order() {
    auto snapshot = world();
    snapshot.objects_by_key["axe"] = object("axe", 1);
    snapshot.objects_by_key["axe"].numeric_states["sharpness"] = 0;
    snapshot.objects_by_key["wood"] = object("wood", 2);
    GoalExecutionSystem system;
    auto result = system.tick(request(context(), snapshot, {wolf()}));
    assert(result.is_ok());
    assert(result.value().trace.front() == "tick_order:interrupt_before_driver");
    assert(result.value().internal_blockers.empty());
}

static void player_delay() {
    auto signal = wolf();
    signal.kind = ExternalInterruptKind::CommandOverride;
    signal.interrupt_id = "interrupt.player.stop";
    signal.severity = 90;
    signal.urgency = 90;
    signal.can_be_ignored = false;
    GoalExecutionSystem system;
    auto result = system.tick(request(context(), world(), {signal}));
    assert(result.is_ok());
    assert(result.value().projection.interrupt_reason.text_zh_cn.find("玩家命令") != std::string::npos);
}

static void capability_filter() {
    ActionDriverRegistry registry;
    assert(registry.registerDefaultDrivers().is_ok());
    assert(registry.find(ActionDriverKind::BuildStructure, AgentCapabilityTier::InstinctAgent).is_error());
    assert(registry.find(ActionDriverKind::CounterThreat, AgentCapabilityTier::InstinctAgent).is_ok());
    assert(registry.find(ActionDriverKind::AdvancedConstruct, AgentCapabilityTier::SimpleAgent).is_error());
}

static void context_roundtrip() {
    auto ctx = context();
    ctx.reserved_resources.push_back(ResourceReservation{"reservation.wood", "companion", "wood", 2, "goal.chop", 10});
    ctx.known_claims.push_back(claim("cut_wood"));
    ExecutionContext copy = ctx;
    assert(copy.actor_key == ctx.actor_key);
    assert(copy.goal_stack.front().frame_id == "frame.chop");
    assert(copy.active_driver_state->driver_state_id == "driver.chop");
    assert(copy.reserved_resources.front().resource_key == "wood");
    assert(copy.known_claims.front().knowledge_id.value() == "knowledge.companion.cut_wood");
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    const std::string name = argv[1];
    if (name == "stack") stack();
    else if (name == "registry") registry();
    else if (name == "dull_axe") dull_axe();
    else if (name == "blocker_subgoal") blocker_subgoal();
    else if (name == "wolf_interrupt") wolf_interrupt();
    else if (name == "resume") resume();
    else if (name == "command_adapter") command_adapter();
    else if (name == "projection_safe") projection_safe();
    else if (name == "materials") materials();
    else if (name == "reaction_links") reaction_links();
    else if (name == "material_tags_from_dialog_runtime_snapshot") material_tags_from_dialog_runtime_snapshot();
    else if (name == "reaction_material_resolver_no_unknown_material_for_core_rules") reaction_material_resolver_no_unknown_material_for_core_rules();
    else if (name == "known_claims") known_claims();
    else if (name == "build_recipe") build_recipe();
    else if (name == "reservation_release") reservation_release();
    else if (name == "tick_order") tick_order();
    else if (name == "player_delay") player_delay();
    else if (name == "capability_filter") capability_filter();
    else if (name == "context_roundtrip") context_roundtrip();
    else return 2;
    std::cout << "goal_execution " << name << " passed\n";
    return 0;
}
