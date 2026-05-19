#pragma once

#include "pathfinder/agent_reasoning/reasoning_types.h"
#include "pathfinder/reaction/reaction_rule.h"

namespace pathfinder::agent_reasoning {

class ReactionPlanningAdapter {
public:
    pathfinder::foundation::Result<std::vector<PlanPrecondition>> preconditionsForEffect(
        const std::string& effect_key,
        const std::vector<pathfinder::reaction::ObjectReactionRule>& rules) const;

    pathfinder::foundation::Result<std::vector<PlanPrecondition>> preconditionsForEffect(
        const std::string& effect_key) const;
};

} // namespace pathfinder::agent_reasoning
