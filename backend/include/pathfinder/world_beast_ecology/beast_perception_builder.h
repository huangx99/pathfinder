#pragma once

#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"

namespace pathfinder::world_beast_ecology {

class BeastPerceptionBuilder {
public:
    pathfinder::foundation::Result<std::vector<BeastPerceptionItem>> build(
        const std::string& actor_key,
        const BeastAgentProfile& profile,
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        const std::vector<pathfinder::world_runtime::WorldActorRuntime>& nearby_actors,
        const std::vector<pathfinder::world_runtime::WorldEntityInstance>& nearby_entities,
        const std::vector<pathfinder::world_runtime::WorldResourceNodeRuntime>& nearby_resources,
        const std::vector<BeastPerceptionItem>& effects_and_areas);
};

} // namespace pathfinder::world_beast_ecology
