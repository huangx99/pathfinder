#pragma once

#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"

namespace pathfinder::world_beast_ecology {

class BeastCommandCompiler {
public:
    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandDto> compile(
        const BeastActionIntent& intent,
        const std::string& request_id,
        const std::string& actor_key);
};

} // namespace pathfinder::world_beast_ecology
