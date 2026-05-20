#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include <memory>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// WorldGenerationApplier - applies generation drafts to runtime
// ---------------------------------------------------------------------------

class WorldGenerationApplier {
public:
    WorldGenerationApplier(
        world_runtime::IWorldRuntime& world_runtime,
        world_inventory::IWorldEntityLocationPort& location_port);

    // Apply generation result to runtime
    WorldGenerationApplyResult apply(const WorldGenerationResult& result);

    // Apply just a set of cell drafts (for region generation)
    WorldGenerationApplyResult applyCells(const std::vector<GeneratedCellDraft>& cells);

    // Apply entity drafts as OnMap ground items (P45 compliant)
    WorldGenerationApplyResult applyEntities(const std::vector<GeneratedEntityDraft>& entities);

    // Apply resource node drafts to runtime (P46)
    WorldGenerationApplyResult applyResourceNodes(const std::vector<GeneratedResourceNodeDraft>& nodes);

    // Apply spawn points (creates actors)
    WorldGenerationApplyResult applySpawnPoints(const std::vector<GeneratedSpawnPointDraft>& spawns);

private:
    world_runtime::IWorldRuntime& world_runtime_;
    world_inventory::IWorldEntityLocationPort& location_port_;
};

} // namespace pathfinder::world_generation
