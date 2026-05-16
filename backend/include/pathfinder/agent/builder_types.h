#pragma once

#include "pathfinder/agent/binding.h"
#include "pathfinder/agent/observation.h"
#include "pathfinder/agent/action_space.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/error.h"
#include <vector>
#include <string>

// Forward declarations to avoid pulling in full state headers
namespace pathfinder::state {
class GameState;
}

namespace pathfinder::agent {

// ObservedThreatSeed: explicit threat input for observation construction
// P6 temporary input until map/perception system exists
struct ObservedThreatSeed {
    pathfinder::foundation::EntityId source_id;
    AgentThreatType threat_type = AgentThreatType::Unknown;
    double severity = 0.0;    // [0.0, 1.0]
    double confidence = 0.0;  // [0.0, 1.0]

    pathfinder::foundation::Result<void> validateBasic() const;
};

// VisibilityInput: explicit visible object/threat lists
// P6 temporary input; future: PerceptionSystem generates this
struct VisibilityInput {
    std::vector<pathfinder::foundation::ObjectId> visible_object_ids;
    std::vector<ObservedThreatSeed> threat_seeds;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ObservationBuildRequest: input for ObservationBuilder
struct ObservationBuildRequest {
    AgentBinding binding;
    const pathfinder::state::GameState* state = nullptr;
    VisibilityInput visibility;
    bool include_cognition_claims = true;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ObservationBuildTrace: debug trace for observation construction
struct ObservationBuildTrace {
    std::vector<pathfinder::foundation::ObjectId> included_object_ids;
    std::vector<pathfinder::foundation::ObjectId> skipped_object_ids;
    std::vector<std::string> reason_keys;
};

// ObservationBuildResult: output from ObservationBuilder
struct ObservationBuildResult {
    AgentObservation observation;
    ObservationBuildTrace trace;
    std::vector<pathfinder::foundation::ErrorDetail> warnings;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ActionSpaceBuildRequest: input for ActionSpaceBuilder
struct ActionSpaceBuildRequest {
    AgentObservation observation;
    bool include_explore_candidates = true;
    bool include_wait_candidate = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ActionSpaceBuildTrace: debug trace for action space construction
struct ActionSpaceBuildTrace {
    std::vector<pathfinder::foundation::ActionId> generated_action_ids;
    std::vector<std::string> reason_keys;
};

// ActionSpaceBuildResult: output from ActionSpaceBuilder
struct ActionSpaceBuildResult {
    AgentActionSpace action_space;
    ActionSpaceBuildTrace trace;
    std::vector<pathfinder::foundation::ErrorDetail> warnings;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::agent
