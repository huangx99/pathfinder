#pragma once

#include "pathfinder/world_map_interaction/world_map_interaction_types.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/foundation/result.h"
#include <string>

namespace pathfinder::world_map_interaction {

// ---------------------------------------------------------------------------
// ClientMapSelectionService
// ---------------------------------------------------------------------------
// P60: Converts player click on map cell/object into a safe selection context
// with backend-authorized available actions. Does not modify world state.
// ---------------------------------------------------------------------------

class ClientMapSelectionService {
public:
    ClientMapSelectionService(
        world_runtime::IWorldRuntime& world_runtime,
        world_inventory::IWorldEntityLocationPort& location_port,
        const pathfinder::client_protocol::ClientAvailableCommandAdapter& available_command_adapter);

    pathfinder::foundation::Result<ClientMapSelectionResponse> select(
        const ClientMapSelectionRequest& request,
        const std::string& actor_key);

private:
    world_runtime::IWorldRuntime& world_runtime_;
    world_inventory::IWorldEntityLocationPort& location_port_;
    const pathfinder::client_protocol::ClientAvailableCommandAdapter& available_command_adapter_;

    ClientMapSelectionResponse buildEmptyResponse(
        const std::string& session_id,
        const world_runtime::WorldCellCoord& cell) const;

    pathfinder::foundation::Result<std::vector<ClientMapSelectedObjectSummary>> summarizeCellObjects(
        const world_runtime::WorldCellRuntime& cell,
        const std::string& layer_key);

    pathfinder::foundation::Result<std::vector<pathfinder::world_command::WorldCommandOptionDto>> filterOptionsForSelection(
        const std::vector<pathfinder::world_command::WorldCommandOptionDto>& all_options,
        const ClientMapSelectionRequest& request,
        const world_runtime::IWorldRuntime& runtime);
};

} // namespace pathfinder::world_map_interaction
