#pragma once

#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/world_teaching/npc_basic_action_knowledge_gate.h"
#include "pathfinder/knowledge/knowledge_repository.h"

namespace pathfinder::world_agent_execution {

class AgentActionKnowledgeGuard {
public:
    explicit AgentActionKnowledgeGuard(const pathfinder::world_teaching::NpcBasicActionKnowledgeGate& gate);

    pathfinder::foundation::Result<pathfinder::world_teaching::NpcActionKnowledgeGateResult> checkStep(
        const std::string& actor_key,
        const pathfinder::agent_reasoning::AgentPlanStep& step,
        const std::vector<pathfinder::knowledge::KnowledgeClaim>& actor_claims,
        bool allow_hypothesis,
        bool allow_risk_action);

private:
    const pathfinder::world_teaching::NpcBasicActionKnowledgeGate& gate_;
};

} // namespace pathfinder::world_agent_execution
