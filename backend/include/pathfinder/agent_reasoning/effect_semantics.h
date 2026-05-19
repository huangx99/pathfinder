#pragma once

#include "pathfinder/agent_reasoning/reasoning_types.h"

namespace pathfinder::agent_reasoning {

class EffectSemanticsRegistry {
public:
    pathfinder::foundation::Result<EffectSemantics> findByEffectKey(const std::string& effect_key) const;
    pathfinder::foundation::Result<std::vector<EffectSemantics>> findByGoalKind(AgentGoalKind goal_kind) const;
    pathfinder::foundation::Result<void> validateAll() const;
};

std::vector<EffectSemantics> builtInEffectSemantics();

} // namespace pathfinder::agent_reasoning
