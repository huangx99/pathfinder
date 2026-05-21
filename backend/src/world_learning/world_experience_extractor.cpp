#include "pathfinder/world_learning/world_experience_extractor.h"
#include "pathfinder/world_command/world_command_types.h"
#include <algorithm>

namespace pathfinder::world_learning {

WorldExperienceExtractor::ExtractResult WorldExperienceExtractor::extract(
    const world_command::WorldCommandExecutionResult& command_result) const {
    ExtractResult result;

    if (command_result.experiences.empty()) {
        result.failure_kind = WorldLearningFailureKind::NoExperience;
        return result;
    }

    for (const auto& exp : command_result.experiences) {
        if (!isLearnable(exp)) {
            result.warning_keys.push_back("experience_not_learnable:" + exp.experience_id);
            continue;
        }
        if (containsUnsafeKeys(exp)) {
            result.failure_kind = WorldLearningFailureKind::UnsafeExperience;
            result.warning_keys.push_back("unsafe_experience:" + exp.experience_id);
            continue;
        }
        result.experiences.push_back(exp);
    }

    if (result.experiences.empty() && result.failure_kind == WorldLearningFailureKind::None) {
        result.failure_kind = WorldLearningFailureKind::NoExperience;
    }

    return result;
}

bool WorldExperienceExtractor::isLearnable(const world_command::WorldExperienceDto& experience) const {
    // An experience must have a valid experience_id and at least an effect_key or command_key
    if (experience.experience_id.empty()) return false;
    if (experience.effect_key.empty() && experience.command_key.empty()) return false;
    if (experience.actor_key.empty()) return false;
    return true;
}

bool WorldExperienceExtractor::containsUnsafeKeys(const world_command::WorldExperienceDto& experience) const {
    if (containsWorldLearningForbiddenKey(experience.effect_key)) return true;
    if (containsWorldLearningForbiddenKey(experience.subject_entity_key)) return true;
    if (containsWorldLearningForbiddenKey(experience.target_entity_key)) return true;
    if (containsWorldLearningForbiddenKey(experience.command_key)) return true;
    if (containsWorldLearningForbiddenKey(experience.condition_keys)) return true;
    if (containsWorldLearningForbiddenKey(experience.reason_keys)) return true;
    return false;
}

} // namespace pathfinder::world_learning
