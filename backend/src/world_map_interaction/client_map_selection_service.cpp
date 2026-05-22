#include "pathfinder/world_map_interaction/client_map_selection_service.h"
#include "pathfinder/foundation/error.h"
#include <chrono>

namespace pathfinder::world_map_interaction {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_command::WorldCommandOptionDto;
using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_runtime::WorldCellCoord;
using pathfinder::world_runtime::WorldCellRuntime;
using pathfinder::world_runtime::WorldEntityInstance;
using pathfinder::world_runtime::WorldResourceNodeRuntime;

namespace {
    std::string makeSelectionId() {
        uint64_t ts = static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        return "sel_" + std::to_string(ts);
    }
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ClientMapSelectionService::ClientMapSelectionService(
    world_runtime::IWorldRuntime& world_runtime,
    world_inventory::IWorldEntityLocationPort& location_port,
    const pathfinder::client_protocol::ClientAvailableCommandAdapter& available_command_adapter)
    : world_runtime_(world_runtime)
    , location_port_(location_port)
    , available_command_adapter_(available_command_adapter) {}

// ---------------------------------------------------------------------------
// select
// ---------------------------------------------------------------------------

Result<ClientMapSelectionResponse> ClientMapSelectionService::select(
    const ClientMapSelectionRequest& request,
    const std::string& actor_key) {

    // Validate request
    auto val = request.validateBasic();
    if (val.is_error()) {
        return Result<ClientMapSelectionResponse>::fail(val.errors());
    }

    // Find the selected cell
    auto cell_res = world_runtime_.findCell(request.cell_coord);
    if (cell_res.is_error()) {
        auto resp = buildEmptyResponse(request.session_id, request.cell_coord);
        resp.warning_keys.push_back("cell_not_found");
        return Result<ClientMapSelectionResponse>::ok(std::move(resp));
    }

    const auto* cell = cell_res.value();
    if (!cell) {
        auto resp = buildEmptyResponse(request.session_id, request.cell_coord);
        resp.warning_keys.push_back("cell_is_null");
        return Result<ClientMapSelectionResponse>::ok(std::move(resp));
    }

    ClientMapSelectionResponse response;
    response.session_id = request.session_id;
    response.selection_id = makeSelectionId();
    response.valid = true;
    // P60: Mark stale if client's known projection version is behind runtime state
    response.stale = (request.known_projection_version > 0 &&
                      request.known_projection_version != world_runtime_.stateVersion());
    response.requires_full_refresh = response.stale;
    response.selected_cell = cell->coord;
    response.selected_cell_terrain_key = cell->terrain_key;

    if (response.stale) {
        response.warning_keys.push_back("projection_version_stale");
        return Result<ClientMapSelectionResponse>::ok(std::move(response));
    }

    // Summarize objects on this cell
    auto objects_res = summarizeCellObjects(*cell, request.layer_key);
    if (objects_res.is_ok()) {
        response.selected_objects = std::move(objects_res.value());
    }

    // Build available commands for the actor
    auto options_res = available_command_adapter_.buildOptions(actor_key, request.layer_key);
    if (options_res.is_ok()) {
        auto filtered_res = filterOptionsForSelection(options_res.value(), request, world_runtime_);
        if (filtered_res.is_ok()) {
            response.available_options = std::move(filtered_res.value());
        }
    }

    // Allowed target kinds based on what's on the cell
    response.allowed_target_kinds.push_back("Coordinate");
    if (!cell->entity_ids.empty()) {
        response.allowed_target_kinds.push_back("Entity");
    }

    return Result<ClientMapSelectionResponse>::ok(std::move(response));
}

// ---------------------------------------------------------------------------
// buildEmptyResponse
// ---------------------------------------------------------------------------

ClientMapSelectionResponse ClientMapSelectionService::buildEmptyResponse(
    const std::string& session_id,
    const WorldCellCoord& cell) const {

    ClientMapSelectionResponse resp;
    resp.session_id = session_id;
    resp.selection_id = makeSelectionId();
    resp.valid = false;
    resp.stale = false;
    resp.selected_cell = cell;
    return resp;
}

// ---------------------------------------------------------------------------
// summarizeCellObjects
// ---------------------------------------------------------------------------

Result<std::vector<ClientMapSelectedObjectSummary>> ClientMapSelectionService::summarizeCellObjects(
    const WorldCellRuntime& cell,
    const std::string& layer_key) {

    std::vector<ClientMapSelectedObjectSummary> summaries;

    // Entities on the cell
    for (const auto& entity_id : cell.entity_ids) {
        auto ent_res = world_runtime_.findEntity(entity_id);
        if (ent_res.is_error()) continue;
        const auto* entity = ent_res.value();
        if (!entity) continue;

        ClientMapSelectedObjectSummary sum;
        sum.object_id = entity_id;
        sum.object_kind = "entity";
        sum.display_name_key = entity->display_name_key;
        sum.display_name_zh = entity->display_name_key; // TODO: localize via ContentRegistry
        sum.tag_keys = entity->tag_keys;
        summaries.push_back(std::move(sum));
    }

    // Resource nodes on the cell (query via formal runtime port, not snapshotForDebug)
    auto nodes_res = world_runtime_.queryResourceNodesAtCell(cell.coord);
    if (nodes_res.is_ok()) {
        for (const auto& node : nodes_res.value()) {
            if (node.coord.layer_key != layer_key) continue;

            ClientMapSelectedObjectSummary sum;
            sum.object_id = node.node_id;
            sum.object_kind = "resource_node";
            sum.display_name_key = node.resource_key;
            sum.display_name_zh = node.resource_key; // TODO: localize
            sum.tag_keys = node.tag_keys;
            summaries.push_back(std::move(sum));
        }
    }

    return Result<std::vector<ClientMapSelectedObjectSummary>>::ok(std::move(summaries));
}

// ---------------------------------------------------------------------------
// filterOptionsForSelection
// ---------------------------------------------------------------------------

Result<std::vector<WorldCommandOptionDto>> ClientMapSelectionService::filterOptionsForSelection(
    const std::vector<WorldCommandOptionDto>& all_options,
    const ClientMapSelectionRequest& request,
    const world_runtime::IWorldRuntime& runtime) {

    std::vector<WorldCommandOptionDto> filtered;

    for (const auto& opt : all_options) {
        // Filter by target relevance
        switch (request.selection_kind) {
            case MapSelectionKind::Cell: {
                // Allow Move, Inspect commands targeting this exact cell
                if (opt.command_kind != WorldCommandKind::Move &&
                    opt.command_kind != WorldCommandKind::Inspect) {
                    break;
                }
                if (!opt.target.target_coord.has_value()) {
                    break;
                }
                if (opt.target.target_coord->x == request.cell_coord.x &&
                    opt.target.target_coord->y == request.cell_coord.y &&
                    opt.target.target_coord->layer_key == request.cell_coord.layer_key) {
                    filtered.push_back(opt);
                }
                break;
            }
            case MapSelectionKind::Entity:
            case MapSelectionKind::GroundItem: {
                // Allow Inspect, Pickup commands targeting this exact entity
                if (opt.command_kind != WorldCommandKind::Inspect &&
                    opt.command_kind != WorldCommandKind::Pickup) {
                    break;
                }
                if (request.entity_id.empty() ||
                    opt.target.target_entity_id == request.entity_id) {
                    // Verify the entity actually exists at the selected cell
                    auto ent_res = runtime.findEntity(request.entity_id);
                    if (ent_res.is_ok() && ent_res.value() != nullptr) {
                        const auto* ent = ent_res.value();
                        if (ent->coord.has_value() &&
                            ent->coord->x == request.cell_coord.x &&
                            ent->coord->y == request.cell_coord.y &&
                            ent->coord->layer_key == request.cell_coord.layer_key) {
                            filtered.push_back(opt);
                        }
                    }
                }
                break;
            }
            case MapSelectionKind::ResourceNode: {
                // Allow Gather/Chop/Mine/Dig commands targeting this exact node
                if (opt.command_kind != WorldCommandKind::Gather &&
                    opt.command_kind != WorldCommandKind::Chop &&
                    opt.command_kind != WorldCommandKind::Mine &&
                    opt.command_kind != WorldCommandKind::Dig) {
                    break;
                }
                if (!request.resource_node_id.empty() &&
                    opt.target.target_entity_id != request.resource_node_id) {
                    break;
                }
                // Verify the node actually exists at the selected cell
                auto node_res = runtime.findResourceNode(request.resource_node_id);
                if (node_res.is_ok() && node_res.value() != nullptr) {
                    const auto* node = node_res.value();
                    if (node->coord.x == request.cell_coord.x &&
                        node->coord.y == request.cell_coord.y &&
                        node->coord.layer_key == request.cell_coord.layer_key) {
                        filtered.push_back(opt);
                    }
                }
                break;
            }
            case MapSelectionKind::InventoryItem: {
                // Allow Drop commands targeting this exact inventory item
                if (opt.command_kind != WorldCommandKind::Drop) {
                    break;
                }
                if (!request.inventory_item_id.empty() &&
                    opt.target.target_entity_id != request.inventory_item_id) {
                    break;
                }
                filtered.push_back(opt);
                break;
            }
            default:
                break;
        }
    }

    return Result<std::vector<WorldCommandOptionDto>>::ok(std::move(filtered));
}

} // namespace pathfinder::world_map_interaction
