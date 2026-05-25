#include "pathfinder/v3_sandbox/v3_agent_decision.h"

#include <algorithm>

namespace pathfinder::v3_sandbox {
namespace {
bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

std::string attemptKey(const std::string& object_key, const std::string& action_key) {
    return object_key + "|" + action_key;
}

bool knowsAction(const V3DecisionContext& context, const std::string& object_key, const std::string& action_key) {
    for (const auto& claim : context.knowledge) {
        if (claim.object_key == object_key && claim.action_key == action_key) return true;
    }
    return false;
}

const V3DecisionKnowledgeView* findUsefulKnowledge(const V3DecisionContext& context, const std::string& effect_key) {
    const V3DecisionKnowledgeView* best = nullptr;
    for (const auto& claim : context.knowledge) {
        if (claim.effect_key != effect_key) continue;
        if (claim.risk_delta > 0.15) continue;
        if (!best || claim.evidence_count > best->evidence_count) best = &claim;
    }
    return best;
}

std::vector<std::string> orderedUnknownActions(const V3DecisionContext& context, const V3DecisionObjectView& object) {
    std::vector<std::string> actions = object.allowed_actions;
    actions.erase(std::remove(actions.begin(), actions.end(), "inspect"), actions.end());
    if (context.hunger >= context.critical_hunger && contains(actions, "eat")) {
        actions.erase(std::remove(actions.begin(), actions.end(), "eat"), actions.end());
        actions.insert(actions.begin(), "eat");
    }

    std::vector<std::string> result;
    for (const auto& action : actions) {
        if (knowsAction(context, object.object_key, action)) continue;
        if (context.attempted_actions.count(attemptKey(object.object_key, action))) continue;
        result.push_back(action);
    }
    return result;
}

std::optional<V3AgentDecision> tryKnownNeedSolver(const V3DecisionContext& context) {
    if (context.hunger < context.critical_hunger) return std::nullopt;
    const auto* food = findUsefulKnowledge(context, "restore_hunger");
    if (!food) return std::nullopt;

    for (const auto& object : context.carried_objects) {
        if (object.object_key == food->object_key) {
            return V3AgentDecision{V3DecisionKind::TryCarriedObject, "", object.object_key, food->action_key};
        }
    }
    for (const auto& object : context.visible_objects) {
        if (object.object_key == food->object_key) {
            return V3AgentDecision{V3DecisionKind::TryVisibleObject, object.instance_id, object.object_key, food->action_key};
        }
    }
    return std::nullopt;
}
} // namespace

V3AgentDecision V3AgentDecisionPolicy::decide(const V3DecisionContext& context) const {
    if (auto decision = tryKnownNeedSolver(context)) return *decision;

    for (const auto& object : context.visible_objects) {
        const auto actions = orderedUnknownActions(context, object);
        if (!actions.empty()) return {V3DecisionKind::TryVisibleObject, object.instance_id, object.object_key, actions.front()};
    }

    for (const auto& object : context.visible_objects) {
        if (!object.can_pickup) continue;
        if (context.inventory_slots_used >= context.inventory_capacity_slots) continue;
        return {V3DecisionKind::PickupVisibleObject, object.instance_id, object.object_key, "pickup"};
    }

    for (const auto& object : context.carried_objects) {
        const auto actions = orderedUnknownActions(context, object);
        if (!actions.empty()) return {V3DecisionKind::TryCarriedObject, "", object.object_key, actions.front()};
    }

    return {V3DecisionKind::Wander, "", "", ""};
}

} // namespace pathfinder::v3_sandbox
