#pragma once

#include "pathfinder/agent/types.h"

namespace pathfinder::agent {

// AgentProfile: behavioral tendencies of an agent
struct AgentProfile {
    double curiosity = 0.0;
    double caution = 0.0;
    double aggression = 0.0;
    double sociality = 0.0;
    double cooperation = 0.0;
    double learning_rate_hint = 0.0;

    // Validate all values are in [0.0, 1.0]
    pathfinder::foundation::Result<void> validateBasic() const;
};

// AgentDefinition: template describing an agent type
struct AgentDefinition {
    AgentDefinitionId definition_id;
    std::string display_name_key;
    AgentScale scale = AgentScale::Unknown;
    AgentCognitionBand cognition_band = AgentCognitionBand::Unknown;
    AgentEmbodiment embodiment = AgentEmbodiment::Unknown;
    AgentControllerType default_controller = AgentControllerType::Unknown;
    AgentProfile profile;

    // Validate all fields are set and valid
    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::agent
