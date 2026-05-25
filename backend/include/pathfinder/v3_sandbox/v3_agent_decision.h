#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace pathfinder::v3_sandbox {

struct V3DecisionObjectView {
    std::string instance_id;
    std::string object_key;
    std::string kind;
    std::vector<std::string> allowed_actions;
    bool can_pickup{false};
    bool carried{false};
};

struct V3DecisionKnowledgeView {
    std::string object_key;
    std::string action_key;
    std::string effect_key;
    int evidence_count{0};
    double utility_delta{0.0};
    double risk_delta{0.0};
};

struct V3DecisionContext {
    double hunger{0.0};
    double critical_hunger{60.0};
    int inventory_slots_used{0};
    int inventory_capacity_slots{8};
    std::vector<V3DecisionObjectView> visible_objects;
    std::vector<V3DecisionObjectView> carried_objects;
    std::vector<V3DecisionKnowledgeView> knowledge;
    std::set<std::string> attempted_actions;
};

enum class V3DecisionKind {
    TryVisibleObject,
    TryCarriedObject,
    PickupVisibleObject,
    Wander
};

struct V3AgentDecision {
    V3DecisionKind kind{V3DecisionKind::Wander};
    std::string object_instance_id;
    std::string object_key;
    std::string action_key;
};

class V3AgentDecisionPolicy {
public:
    V3AgentDecision decide(const V3DecisionContext& context) const;
};

} // namespace pathfinder::v3_sandbox
