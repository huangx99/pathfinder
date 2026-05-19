#pragma once

#include "pathfinder/story/story_types.h"

namespace pathfinder::story {

class StoryScenarioRegistry {
public:
    pathfinder::foundation::Result<StoryScenarioDefinition> firstDaySurvival() const;
    pathfinder::foundation::Result<StoryScenarioDefinition> getScenarioByKey(const std::string& key) const;
};

class StoryRuntimeFactory {
public:
    pathfinder::foundation::Result<StoryRuntimeState> createInitialState(const StoryScenarioDefinition& scenario) const;
};

class StoryObjectiveEvaluator {
public:
    pathfinder::foundation::Result<std::vector<StoryObjectiveState>> evaluate(
        const StoryScenarioDefinition& scenario,
        const StoryRuntimeState& state,
        const StoryEvaluationContext& context) const;
};

class StoryProgressionService {
public:
    pathfinder::foundation::Result<StoryTurnResult> applyTurn(
        const StoryScenarioDefinition& scenario,
        const StoryRuntimeState& current_state,
        const StoryEvaluationContext& context) const;

private:
    StoryObjectiveEvaluator evaluator_;
};

class StoryProjectionAdapter {
public:
    pathfinder::foundation::Result<StoryProjection> composeProjection(
        const StoryScenarioDefinition& scenario,
        const StoryRuntimeState& state,
        const StoryEvaluationContext& context,
        pathfinder::h5_projection::ProjectionAudience audience) const;
};

} // namespace pathfinder::story
