#pragma once

#include "pathfinder/world_reaction/world_reaction_types.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_reaction {

// ---------------------------------------------------------------------------
// WorldReactionService
// ---------------------------------------------------------------------------
// Orchestrates validate + apply + execute for world reactions.
// Must not be used directly by command handlers; go through execute().
// ---------------------------------------------------------------------------

class WorldReactionService {
public:
    WorldReactionService(
        const content::ContentRegistry& content_registry,
        world_runtime::IWorldRuntime& world_runtime,
        world_inventory::IInventoryRuntime& inventory_runtime,
        world_inventory::IWorldEntityLocationPort& location_port);

    pathfinder::foundation::Result<WorldReactionDraft> validate(
        const WorldReactionRequest& request) const;

    pathfinder::foundation::Result<WorldReactionResult> apply(
        WorldReactionDraft& draft);

    pathfinder::foundation::Result<WorldReactionResult> execute(
        const WorldReactionRequest& request);

private:
    const content::ContentRegistry& content_registry_;
    world_runtime::IWorldRuntime& world_runtime_;
    world_inventory::IInventoryRuntime& inventory_runtime_;
    world_inventory::IWorldEntityLocationPort& location_port_;

    // Validation helpers
    pathfinder::foundation::Result<void> resolveInputs(
        const WorldReactionRequest& request,
        WorldReactionDraft& draft) const;

    pathfinder::foundation::Result<void> buildConsumeDrafts(
        const WorldReactionRequest& request,
        WorldReactionDraft& draft) const;

    pathfinder::foundation::Result<void> buildOutputDrafts(
        const WorldReactionRequest& request,
        WorldReactionDraft& draft) const;

    // Apply helpers
    pathfinder::foundation::Result<void> applyConsumes(
        WorldReactionDraft& draft,
        WorldReactionResult& result);

    pathfinder::foundation::Result<void> applySpawns(
        WorldReactionDraft& draft,
        WorldReactionResult& result);

    // Input resolution by source
    bool findInputInActorInventory(
        const std::string& actor_key,
        const std::string& object_key,
        const std::string& required_state,
        int required_quantity,
        WorldReactionInputProof& out_proof) const;

    bool findInputOnMapAtCoord(
        const world_runtime::WorldCellCoord& coord,
        const std::string& object_key,
        const std::string& required_state,
        int required_quantity,
        WorldReactionInputProof& out_proof) const;
};

} // namespace pathfinder::world_reaction
