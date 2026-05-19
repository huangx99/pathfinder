#pragma once

#include "pathfinder/danger/danger_types.h"

namespace pathfinder::danger {

class ThreatProfileBuilder {
public:
    pathfinder::foundation::Result<ThreatProfile> fromRuntimeObject(
        const pathfinder::reaction::ReactionRuntimeObject& object) const;

    pathfinder::foundation::Result<ThreatProfile> fromCreatureProjection(
        pathfinder::foundation::EntityId creature_id,
        std::string creature_key,
        DangerSeverity severity = DangerSeverity::Harm,
        double base_pressure = 0.6) const;
};

} // namespace pathfinder::danger
