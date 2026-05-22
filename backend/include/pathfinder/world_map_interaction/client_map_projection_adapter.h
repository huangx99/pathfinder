#pragma once

#include "pathfinder/world_map_interaction/world_map_interaction_types.h"
#include "pathfinder/client_protocol/client_projection_adapter.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/foundation/result.h"
#include <string>

namespace pathfinder::world_map_interaction {

// ---------------------------------------------------------------------------
// ClientMapProjectionAdapter
// ---------------------------------------------------------------------------
// P60: Builds a safe map projection for the formal client.
// Does not leak hidden truth. Does not return raw runtime state.
// ---------------------------------------------------------------------------

class ClientMapProjectionAdapter {
public:
    ClientMapProjectionAdapter(
        const pathfinder::client_protocol::ClientProjectionAdapter& base_adapter,
        world_runtime::IWorldRuntime& world_runtime);

    pathfinder::foundation::Result<ClientMapProjectionDto> buildMapProjection(
        const std::string& actor_key,
        const std::string& layer_key,
        const world_runtime::WorldCellCoord& viewport_center,
        int viewport_radius,
        uint64_t projection_version) const;

private:
    const pathfinder::client_protocol::ClientProjectionAdapter& base_adapter_;
    world_runtime::IWorldRuntime& world_runtime_;
};

} // namespace pathfinder::world_map_interaction
