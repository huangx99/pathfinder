#pragma once

#include "pathfinder/agent_reasoning/effect_semantics.h"
#include "pathfinder/world_interaction/world_types.h"

namespace pathfinder::agent_reasoning {

class AgentGoalGenerator {
public:
    pathfinder::foundation::Result<std::vector<AgentGoal>> generateGoals(const GoalGenerationInput& input) const;
};

class KnowledgeActionCandidateBuilder {
public:
    pathfinder::foundation::Result<std::vector<ActionCandidate>> buildCandidates(const CandidateBuildInput& input) const;
};

class PlanPreconditionResolver {
public:
    pathfinder::foundation::Result<AgentPlan> resolvePreconditions(const PreconditionResolveInput& input) const;
};

class PlanUtilityScorer {
public:
    pathfinder::foundation::Result<PlanScore> score(const PlanScoreInput& input) const;
};

class AgentReasoner {
public:
    pathfinder::foundation::Result<ReasoningResult> reason(const ReasoningRequest& request) const;
};

class AgentPlanExecutorAdapter {
public:
    pathfinder::foundation::Result<pathfinder::world_interaction::AgentAutonomyResult> toAutonomyResult(
        const AgentPlan& plan,
        const pathfinder::world_interaction::WorldSnapshot& snapshot) const;

    pathfinder::foundation::Result<pathfinder::world_interaction::WorldInteractionInput> toWorldInteractionInput(
        const AgentPlanStep& step,
        const pathfinder::world_interaction::WorldSnapshot& snapshot) const;
};

class ReasoningProjectionMapper {
public:
    pathfinder::foundation::Result<ReasoningProjection> buildProjection(const ReasoningResult& result) const;
};

} // namespace pathfinder::agent_reasoning
