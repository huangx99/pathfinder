#include "pathfinder/world_map_interaction/client_map_projection_adapter.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_map_interaction {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::client_protocol::ClientProjectionScope;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ClientMapProjectionAdapter::ClientMapProjectionAdapter(
    const pathfinder::client_protocol::ClientProjectionAdapter& base_adapter,
    world_runtime::IWorldRuntime& world_runtime)
    : base_adapter_(base_adapter)
    , world_runtime_(world_runtime) {}

// ---------------------------------------------------------------------------
// buildMapProjection
// ---------------------------------------------------------------------------

Result<ClientMapProjectionDto> ClientMapProjectionAdapter::buildMapProjection(
    const std::string& actor_key,
    const std::string& layer_key,
    const world_runtime::WorldCellCoord& viewport_center,
    int viewport_radius,
    uint64_t projection_version) const {

    // Build scoped projection via the base adapter
    std::vector<ClientProjectionScope> scopes = {
        ClientProjectionScope::Viewport,
        ClientProjectionScope::ActorSelf,
        ClientProjectionScope::Inventory,
        ClientProjectionScope::EventFeed
    };

    auto base_res = base_adapter_.buildScopedProjection(
        actor_key, layer_key, projection_version, scopes);
    if (base_res.is_error()) {
        return Result<ClientMapProjectionDto>::fail(base_res.errors());
    }

    const auto& base = base_res.value();

    // Convert base projection to map projection
    ClientMapProjectionDto projection;
    projection.projection_version = base.projection_version;
    projection.actor_key = base.actor_key;
    projection.active_layer_key = base.active_layer_key;
    projection.viewport_center = viewport_center;
    projection.viewport_radius = viewport_radius;
    projection.visible_cells = base.visible_cells;
    projection.visible_entities = base.visible_entities;
    projection.inventories = base.inventories;
    projection.knowledge = base.knowledge;
    projection.area_effects = base.area_effects;
    projection.safe_summary_keys = base.safe_summary_keys;

    return Result<ClientMapProjectionDto>::ok(std::move(projection));
}

} // namespace pathfinder::world_map_interaction
