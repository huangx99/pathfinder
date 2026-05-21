#pragma once

#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_learning {

// ---------------------------------------------------------------------------
// WorldExperienceExtractor
// ---------------------------------------------------------------------------
// Extracts learnable experiences from a WorldCommandExecutionResult.
// Does not judge truth or falsity of experiences.
// ---------------------------------------------------------------------------

class WorldExperienceExtractor {
public:
    struct ExtractResult {
        std::vector<world_command::WorldExperienceDto> experiences;
        WorldLearningFailureKind failure_kind = WorldLearningFailureKind::None;
        std::vector<std::string> warning_keys;
    };

    ExtractResult extract(const world_command::WorldCommandExecutionResult& command_result) const;

private:
    bool isLearnable(const world_command::WorldExperienceDto& experience) const;
    bool containsUnsafeKeys(const world_command::WorldExperienceDto& experience) const;
};

} // namespace pathfinder::world_learning
