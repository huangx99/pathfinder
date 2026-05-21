#pragma once

#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"

namespace pathfinder::world_beast_ecology {

class BeastProjectionBridge {
public:
    struct SafeBeastProjection {
        std::string actor_key;
        std::string intent_kind_str;
        std::string reason_kind_str;
        std::string safe_summary_zh_cn;
        std::vector<std::string> reason_keys;
    };

    SafeBeastProjection project(const BeastTickResult& result);
};

} // namespace pathfinder::world_beast_ecology
