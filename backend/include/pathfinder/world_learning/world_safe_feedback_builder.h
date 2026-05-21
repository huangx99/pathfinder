#pragma once

#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/learning/learning_loop.h"
#include "pathfinder/content/content_types.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_learning {

// ---------------------------------------------------------------------------
// WorldSafeFeedbackBuilder
// ---------------------------------------------------------------------------
// Builds LearningSafeFeedbackInput from a world experience + knowledge template.
// Does not leak hidden state.
// ---------------------------------------------------------------------------

class WorldSafeFeedbackBuilder {
public:
    struct BuildResult {
        pathfinder::learning::LearningSafeFeedbackInput feedback;
        WorldLearningFailureKind failure_kind = WorldLearningFailureKind::None;
        std::vector<std::string> warning_keys;
    };

    BuildResult build(
        const world_command::WorldExperienceDto& experience,
        const pathfinder::content::KnowledgeTemplateContent& tmpl,
        const pathfinder::content::EffectDefinitionContent* effect,
        const std::vector<world_command::WorldEventDto>& source_events,
        uint64_t tick) const;

    BuildResult build(
        const world_command::WorldExperienceDto& experience,
        const pathfinder::content::KnowledgeTemplateContent& tmpl,
        const std::vector<world_command::WorldEventDto>& source_events,
        uint64_t tick) const;

private:
    pathfinder::cognition::CognitionOutcomeSignal mapEffectToOutcomeSignal(
        const pathfinder::content::EffectDefinitionContent* effect,
        const std::string& command_key) const;

    pathfinder::cognition::CognitionActionContextKind mapCommandToActionContext(
        const std::string& command_key) const;

    double computeUtilityDelta(
        const pathfinder::content::EffectDefinitionContent* effect) const;

    double computeRiskDelta(
        const pathfinder::content::EffectDefinitionContent* effect) const;
};

} // namespace pathfinder::world_learning
