#pragma once

#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"

namespace pathfinder::world_beast_ecology {

class BeastInstinctGate {
public:
    struct GateResult {
        bool allowed = false;
        std::string blocker_key;
        std::vector<std::string> reason_keys;
    };

    GateResult check(const BeastAgentProfile& profile, const BeastActionIntent& intent);
};

} // namespace pathfinder::world_beast_ecology
