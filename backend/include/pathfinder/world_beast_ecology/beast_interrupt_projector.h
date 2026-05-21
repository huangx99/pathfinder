#pragma once

#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"

namespace pathfinder::world_beast_ecology {

class BeastInterruptProjector {
public:
    std::vector<pathfinder::goal_execution::ExternalInterruptSignal> project(
        const BeastActionIntent& intent,
        const std::vector<BeastPerceptionItem>& perceptions,
        uint64_t tick);
};

} // namespace pathfinder::world_beast_ecology
