#include "pathfinder/world_agent_execution/agent_action_knowledge_guard.h"

namespace pathfinder::world_agent_execution {

using pathfinder::foundation::Result;
using pathfinder::world_teaching::NpcActionKnowledgeGateRequest;
using pathfinder::world_teaching::NpcActionKnowledgeGateResult;

AgentActionKnowledgeGuard::AgentActionKnowledgeGuard(const pathfinder::world_teaching::NpcBasicActionKnowledgeGate& gate)
    : gate_(gate) {
}

Result<NpcActionKnowledgeGateResult> AgentActionKnowledgeGuard::checkStep(
    const std::string& actor_key,
    const pathfinder::agent_reasoning::AgentPlanStep& step,
    const std::vector<pathfinder::knowledge::KnowledgeClaim>& actor_claims,
    bool allow_hypothesis,
    bool allow_risk_action) {

    // Move and Wait do not require object knowledge
    if (step.action_key == "move" || step.action_key == "wait") {
        NpcActionKnowledgeGateResult allowed;
        allowed.allowed = true;
        allowed.decision = pathfinder::world_teaching::NpcActionKnowledgeGateDecision::AllowedByKnowledge;
        return Result<NpcActionKnowledgeGateResult>::ok(allowed);
    }

    NpcActionKnowledgeGateRequest request;
    request.actor_key = actor_key;
    request.subject_object_key = step.object_key;
    request.action_key = step.action_key;
    request.effect_key = step.effect_key;
    request.target_object_key = step.target_key.value_or("");
    request.allow_hypothesis = allow_hypothesis;
    request.allow_risk_action = allow_risk_action;
    request.actor_claims = actor_claims;

    auto result = gate_.check(request);
    return Result<NpcActionKnowledgeGateResult>::ok(result);
}

} // namespace pathfinder::world_agent_execution
