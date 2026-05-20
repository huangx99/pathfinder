#pragma once

#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/world_command/world_command_types.h"

namespace pathfinder::world_runtime {

class IWorldRuntime {
public:
    virtual ~IWorldRuntime() = default;

    virtual foundation::Result<void> initialize(const WorldRuntimeConfig& config) = 0;
    virtual uint64_t currentWorldTick() const = 0;
    virtual uint64_t stateVersion() const = 0;

    virtual foundation::Result<const WorldCellRuntime*> findCell(const WorldCellCoord& coord) const = 0;
    virtual foundation::Result<const WorldEntityInstance*> findEntity(const std::string& entity_id) const = 0;
    virtual foundation::Result<const WorldActorRuntime*> findActor(const std::string& actor_key) const = 0;

    virtual foundation::Result<void> generateInitialWorld(const WorldRuntimeConfig& config) = 0;
    virtual foundation::Result<MoveActorResult> moveActor(const std::string& actor_key, const WorldCellCoord& target) = 0;
    virtual foundation::Result<InspectWorldResult> inspect(const std::string& actor_key, const world_command::WorldCommandTargetDto& target) const = 0;
    virtual foundation::Result<AdvanceWorldTimeResult> advanceWorldTime(uint64_t tick_delta) = 0;

    virtual foundation::Result<WorldRuntimeSnapshot> snapshotForDebug() const = 0;
};

} // namespace pathfinder::world_runtime
