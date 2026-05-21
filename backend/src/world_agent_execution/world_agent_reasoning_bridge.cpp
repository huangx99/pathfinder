#include "pathfinder/world_agent_execution/world_agent_reasoning_bridge.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_agent_execution {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::agent_reasoning::ReasoningRequest;
using pathfinder::agent_reasoning::ReasoningResult;
using pathfinder::agent_reasoning::AgentPlan;
using pathfinder::agent_reasoning::AgentPlanStep;
using pathfinder::goal_execution::InternalBlocker;
using pathfinder::goal_execution::InternalBlockerKind;

namespace {

pathfinder::world_interaction::WorldSnapshot buildReasoningSnapshot(const WorldAgentDecisionContext& context) {
    pathfinder::world_interaction::WorldSnapshot snapshot;
    snapshot.snapshot_id = context.context_id + ".world";
    snapshot.scenario_key = "v2_grid_runtime";
    snapshot.turn_index = context.tick;
    snapshot.location_key = context.actor.coord.layer_key.empty() ? "surface" : context.actor.coord.layer_key;
    snapshot.trace_keys.push_back("p51.v2_grid_snapshot_adapter");

    pathfinder::world_interaction::WorldActorRuntimeState actor;
    actor.actor_key = context.actor_key;
    actor.display_name_zh_cn = context.actor_key;
    actor.is_agent_controlled = !context.actor.is_player_controlled;
    actor.can_act = true;
    actor.known_claims = context.actor_claims;

    for (const auto& inventory_pair : context.inventory_snapshot.inventories) {
        const auto& inventory = inventory_pair.second;
        if (inventory.owner.owner_key != context.actor_key) {
            continue;
        }
        for (const auto& entry : inventory.entries) {
            actor.held_object_keys.push_back(entry.entity_key);

            auto& object = snapshot.objects_by_key[entry.entity_key];
            if (object.instance_id.empty()) {
                object.instance_id = entry.entity_id.empty() ? entry.entity_key : entry.entity_id;
                object.definition_key = entry.entity_key;
                object.display_name_zh_cn = entry.entity_key;
                object.kind = pathfinder::world_interaction::WorldObjectInstanceKind::GeneratedInstance;
                object.owner_actor_key = context.actor_key;
                object.visible = true;
                object.usable = true;
                object.state_tags = entry.state_keys;
                object.numeric_states.clear();
                for (const auto& numeric_state : entry.numeric_states) {
                    object.numeric_states[numeric_state.first] = numeric_state.second;
                }
            }
            object.quantity += entry.quantity;
            object.actor_quantities[context.actor_key] += entry.quantity;
        }
    }

    snapshot.actors_by_key[actor.actor_key] = std::move(actor);

    for (const auto& resource : context.visible_resources) {
        auto& object = snapshot.objects_by_key[resource.resource_key];
        if (object.instance_id.empty()) {
            object.instance_id = resource.node_id.empty() ? resource.resource_key : resource.node_id;
            object.definition_key = resource.resource_key;
            object.display_name_zh_cn = resource.resource_key;
            object.kind = pathfinder::world_interaction::WorldObjectInstanceKind::ResourceStack;
            object.visible = true;
            object.usable = true;
            object.state_tags = resource.tag_keys;
        }
        object.quantity += std::max(0, resource.remaining_charges);
    }

    for (const auto& entity : context.visible_entities) {
        auto& object = snapshot.objects_by_key[entity.entity_key];
        if (object.instance_id.empty()) {
            object.instance_id = entity.entity_id.empty() ? entity.entity_key : entity.entity_id;
            object.definition_key = entity.entity_key;
            object.display_name_zh_cn = entity.entity_key;
            object.kind = pathfinder::world_interaction::WorldObjectInstanceKind::GeneratedInstance;
            object.owner_actor_key = entity.owner_ref;
            object.visible = true;
            object.usable = true;
            object.state_tags = entity.tag_keys;
        }
        object.quantity = std::max(1, object.quantity);
    }

    for (const auto& interrupt : context.external_interrupts) {
        if (!interrupt.threat_key.has_value() || interrupt.threat_key->empty()) {
            continue;
        }
        pathfinder::world_interaction::WorldThreatRuntimeState threat;
        threat.threat_key = *interrupt.threat_key;
        threat.display_name_zh_cn = *interrupt.threat_key;
        threat.phase = pathfinder::world_interaction::ThreatEventPhase::Approaching;
        threat.level = std::clamp(std::max(interrupt.severity, interrupt.urgency), 0.0, 100.0);
        threat.active = true;
        threat.resolved = false;
        threat.last_change_reason = interrupt.interrupt_id;
        snapshot.threats_by_key[threat.threat_key] = std::move(threat);
    }

    return snapshot;
}

pathfinder::agent_reasoning::AgentNeedState buildNeedState(
    const std::string& actor_key,
    const pathfinder::agent_reasoning::AgentGoal& goal) {
    pathfinder::agent_reasoning::AgentNeedState need;
    need.actor_key = actor_key;
    switch (goal.kind) {
        case pathfinder::agent_reasoning::AgentGoalKind::ReduceHunger:
            need.hunger = std::max(60.0, goal.urgency);
            break;
        case pathfinder::agent_reasoning::AgentGoalKind::ReduceCold:
        case pathfinder::agent_reasoning::AgentGoalKind::IncreaseWarmth:
        case pathfinder::agent_reasoning::AgentGoalKind::IncreaseShelterCapacity:
            need.cold = std::max(60.0, goal.urgency);
            break;
        case pathfinder::agent_reasoning::AgentGoalKind::ReduceThreat:
        case pathfinder::agent_reasoning::AgentGoalKind::ProtectDependent:
            need.fear = std::max(60.0, goal.urgency);
            break;
        case pathfinder::agent_reasoning::AgentGoalKind::RestoreHealth:
            need.health = std::max(0.0, 100.0 - std::max(1.0, goal.urgency));
            break;
        default:
            break;
    }
    return need;
}

} // namespace

WorldAgentReasoningBridge::WorldAgentReasoningBridge(const pathfinder::agent_reasoning::AgentReasoner& reasoner)
    : reasoner_(reasoner) {
}

Result<WorldAgentReasoningBridge::ReasoningResult> WorldAgentReasoningBridge::reasonForGoal(
    const WorldAgentDecisionContext& context,
    const pathfinder::agent_reasoning::AgentGoal& goal) {

    ReasoningRequest req;
    req.request_id = context.context_id + ".reason";
    req.actor_key = context.actor_key;
    req.world_snapshot = buildReasoningSnapshot(context);
    req.need_state = buildNeedState(context.actor_key, goal);
    req.agent_knowledge = context.actor_claims;
    req.trigger_key = goal.goal_id;

    // First ask the existing P39 reasoner. Generic fallback below is only used when
    // the selected V2 grid goal cannot yet be represented by P39's need snapshot.
    auto reasoner_result = reasoner_.reason(req);
    if (reasoner_result.is_ok()) {
        auto rr = reasoner_result.value();
        if (rr.ok && rr.selected_plan.has_value()) {
            WorldAgentReasoningBridge::ReasoningResult result;
            result.ok = true;
            result.plan = rr.selected_plan.value();
            return Result<WorldAgentReasoningBridge::ReasoningResult>::ok(result);
        }
    }

    // Fallback minimal plan construction for MVP when reasoner fails or is stubbed
    WorldAgentReasoningBridge::ReasoningResult fallback;
    fallback.ok = true;

    AgentPlan plan;
    plan.plan_id = context.context_id + ".plan";
    plan.actor_key = context.actor_key;
    plan.goal = goal;

    AgentPlanStep step;
    step.step_id = plan.plan_id + ".step0";
    step.actor_key = context.actor_key;

    if (goal.kind == pathfinder::agent_reasoning::AgentGoalKind::AcquireObject && goal.target_key.has_value()) {
        step.action_key = "gather";
        step.object_key = goal.target_key.value();
        step.target_key = goal.target_key;
    } else if (goal.kind == pathfinder::agent_reasoning::AgentGoalKind::ReduceThreat) {
        step.action_key = "flee";
        step.object_key = "self";
    } else {
        step.action_key = "wait";
        step.object_key = "";
    }

    plan.steps.push_back(step);
    fallback.plan = plan;
    fallback.reason_keys.push_back("reasoning_bridge_fallback");

    return Result<WorldAgentReasoningBridge::ReasoningResult>::ok(fallback);
}

} // namespace pathfinder::world_agent_execution
