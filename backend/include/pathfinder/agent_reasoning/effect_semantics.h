#pragma once

#include "pathfinder/agent_reasoning/reasoning_types.h"
#include <unordered_map>

namespace pathfinder::agent_reasoning {

class EffectSemanticsRegistry {
public:
    EffectSemanticsRegistry();
    explicit EffectSemanticsRegistry(bool load_builtins);

    pathfinder::foundation::Result<void> registerDefinition(const EffectSemantics& definition);
    pathfinder::foundation::Result<EffectSemantics> findByEffectKey(const std::string& effect_key) const;
    pathfinder::foundation::Result<std::vector<EffectSemantics>> findByGoalKind(AgentGoalKind goal_kind) const;
    pathfinder::foundation::Result<void> validateAll() const;

private:
    std::unordered_map<std::string, EffectSemantics> definitions_by_effect_key_;
};

std::vector<EffectSemantics> builtInEffectSemantics();

} // namespace pathfinder::agent_reasoning
