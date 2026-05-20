#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/foundation/error.h"
#include <cmath>
#include <algorithm>

namespace pathfinder::world_runtime {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorDetail;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

WorldGridRuntime::WorldGridRuntime() = default;

Result<void> WorldGridRuntime::initialize(const WorldRuntimeConfig& config) {
    config_ = config;
    world_tick_ = 0;
    state_version_ = 1;
    cells_.clear();
    entities_.clear();
    actors_.clear();
    exploration_.clear();
    return Result<void>::ok();
}

uint64_t WorldGridRuntime::currentWorldTick() const {
    return world_tick_;
}

uint64_t WorldGridRuntime::stateVersion() const {
    return state_version_;
}

void WorldGridRuntime::incrementStateVersion() {
    ++state_version_;
}

Result<const WorldCellRuntime*> WorldGridRuntime::findCell(const WorldCellCoord& coord) const {
    auto it = cells_.find(coord.cellId());
    if (it == cells_.end()) {
        return Result<const WorldCellRuntime*>::fail(
            makeError(ErrorCode::id_not_found, "cell_not_found", "Cell not found: " + coord.cellId()));
    }
    return Result<const WorldCellRuntime*>::ok(&it->second);
}

Result<const WorldEntityInstance*> WorldGridRuntime::findEntity(const std::string& entity_id) const {
    auto it = entities_.find(entity_id);
    if (it == entities_.end()) {
        return Result<const WorldEntityInstance*>::fail(
            makeError(ErrorCode::id_not_found, "entity_not_found", "Entity not found: " + entity_id));
    }
    return Result<const WorldEntityInstance*>::ok(&it->second);
}

Result<const WorldActorRuntime*> WorldGridRuntime::findActor(const std::string& actor_key) const {
    auto it = actors_.find(actor_key);
    if (it == actors_.end()) {
        return Result<const WorldActorRuntime*>::fail(
            makeError(ErrorCode::id_not_found, "actor_not_found", "Actor not found: " + actor_key));
    }
    return Result<const WorldActorRuntime*>::ok(&it->second);
}

namespace {
std::string makeStableEntityId(const std::string& entity_key, const WorldCellCoord& coord, int local_index = 0) {
    return entity_key + "_" + coord.layer_key + "_" + std::to_string(coord.x) + "_" + std::to_string(coord.y)
           + (local_index > 0 ? "_" + std::to_string(local_index) : "");
}
} // anonymous namespace

Result<void> WorldGridRuntime::generateInitialWorld(const WorldRuntimeConfig& config) {
    config_ = config;
    cells_.clear();
    entities_.clear();
    actors_.clear();
    exploration_.clear();
    world_tick_ = 0;
    state_version_ = 1;

    // Use seed for deterministic generation
    uint64_t seed = config.seed;

    int radius = config.initial_region_radius;
    int size = config.region_size;

    for (int rx = -radius; rx <= radius; ++rx) {
        for (int ry = -radius; ry <= radius; ++ry) {
            std::string region_id = "region_" + std::to_string(rx) + "_" + std::to_string(ry);
            for (int cx = 0; cx < size; ++cx) {
                for (int cy = 0; cy < size; ++cy) {
                    int world_x = rx * size + cx;
                    int world_y = ry * size + cy;
                    WorldCellCoord coord{world_x, world_y, "surface"};
                    WorldCellRuntime cell;
                    cell.cell_id = coord.cellId();
                    cell.coord = coord;
                    cell.region_id = region_id;
                    cell.generated = true;

                    // Deterministic terrain based on position and seed
                    int terrain_roll = (world_x * 374761 + world_y * 668265 + static_cast<int>(config.seed)) % 100;
                    if (terrain_roll < 0) terrain_roll += 100;

                    if (terrain_roll < 40) {
                        cell.terrain_key = "plain";
                        cell.blocks_movement = false;
                        cell.movement_cost = 1;
                    } else if (terrain_roll < 65) {
                        cell.terrain_key = "forest";
                        cell.blocks_movement = false;
                        cell.movement_cost = 2;
                    } else if (terrain_roll < 80) {
                        cell.terrain_key = "water";
                        cell.blocks_movement = true;
                        cell.movement_cost = 99;
                    } else {
                        cell.terrain_key = "mountain";
                        cell.blocks_movement = true;
                        cell.movement_cost = 99;
                    }

                    // Sparse entities for inspect only (no gameplay logic)
                    int entity_roll = (world_x * 123456 + world_y * 789012 + static_cast<int>(config.seed)) % 100;
                    if (entity_roll < 0) entity_roll += 100;
                    if (entity_roll < 5 && !cell.blocks_movement) {
                        WorldEntityInstance entity;
                        if (entity_roll < 3) {
                            entity.entity_key = "unknown_bush";
                            entity.display_name_key = "entity.unknown_bush";
                        } else {
                            entity.entity_key = "loose_stone";
                            entity.display_name_key = "entity.loose_stone";
                        }
                        entity.entity_id = makeStableEntityId(entity.entity_key, coord);
                        entity.coord = coord;
                        entity.location_kind = WorldEntityLocationKind::OnMap;
                        entity.blocks_movement = false;
                        entity.visible_by_default = true;
                        entity.quantity = 1;
                        entity.stackable = true;
                        entity.stack_key = entity.entity_key + ":default";
                        cell.entity_ids.push_back(entity.entity_id);
                        entities_[entity.entity_id] = std::move(entity);
                    }

                    cells_[cell.cell_id] = std::move(cell);
                }
            }
        }
    }

    // Create player actor at origin
    WorldActorRuntime player;
    player.actor_key = "player";
    player.entity_id = makeStableEntityId("player", player.coord);
    player.coord = WorldCellCoord{0, 0, "surface"};
    player.vision_radius = config.default_vision_radius;
    player.is_player_controlled = true;
    std::string player_entity_id = player.entity_id;
    WorldCellCoord player_coord = player.coord;
    actors_[player.actor_key] = std::move(player);

    // Create player entity instance on map
    WorldEntityInstance player_entity;
    player_entity.entity_id = player_entity_id;
    player_entity.entity_key = "player";
    player_entity.display_name_key = "entity.player";
    player_entity.coord = player_coord;
    player_entity.location_kind = WorldEntityLocationKind::OnMap;
    player_entity.visible_by_default = true;
    entities_[player_entity.entity_id] = std::move(player_entity);

    // Add player to origin cell
    auto origin_it = cells_.find(WorldCellCoord{0, 0, "surface"}.cellId());
    if (origin_it != cells_.end()) {
        origin_it->second.entity_ids.push_back(player_entity_id);
    }

    // Initialize exploration for player
    updateExplorationForActor("player");

    incrementStateVersion();
    return Result<void>::ok();
}

bool WorldGridRuntime::isAdjacent(const WorldCellCoord& a, const WorldCellCoord& b) const {
    if (a.layer_key != b.layer_key) return false;
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    return (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
}

void WorldGridRuntime::updateExplorationForActor(const std::string& actor_key) {
    auto actor_it = actors_.find(actor_key);
    if (actor_it == actors_.end()) return;

    auto& actor = actor_it->second;
    auto& exp = exploration_[actor_key];
    exp.actor_key = actor_key;

    // Mark all previously visible as discovered
    for (auto& [cell_id, visibility] : exp.cell_visibility_by_id) {
        if (visibility == WorldCellVisibility::Visible) {
            visibility = WorldCellVisibility::Discovered;
        }
    }

    // Mark cells within vision radius as visible
    for (int dy = -actor.vision_radius; dy <= actor.vision_radius; ++dy) {
        for (int dx = -actor.vision_radius; dx <= actor.vision_radius; ++dx) {
            if (std::abs(dx) + std::abs(dy) > actor.vision_radius) continue;
            WorldCellCoord coord{actor.coord.x + dx, actor.coord.y + dy, actor.coord.layer_key};
            std::string cid = coord.cellId();
            if (cells_.find(cid) != cells_.end()) {
                exp.cell_visibility_by_id[cid] = WorldCellVisibility::Visible;
            }
        }
    }
}

WorldCellVisibility WorldGridRuntime::getCellVisibility(const std::string& actor_key, const std::string& cell_id) const {
    auto exp_it = exploration_.find(actor_key);
    if (exp_it == exploration_.end()) return WorldCellVisibility::Unknown;
    auto vis_it = exp_it->second.cell_visibility_by_id.find(cell_id);
    if (vis_it == exp_it->second.cell_visibility_by_id.end()) return WorldCellVisibility::Unknown;
    return vis_it->second;
}

std::vector<std::string> WorldGridRuntime::getVisibleCellIds(const std::string& actor_key) const {
    std::vector<std::string> result;
    auto exp_it = exploration_.find(actor_key);
    if (exp_it == exploration_.end()) return result;
    for (const auto& [cell_id, visibility] : exp_it->second.cell_visibility_by_id) {
        if (visibility == WorldCellVisibility::Visible) {
            result.push_back(cell_id);
        }
    }
    return result;
}

bool WorldGridRuntime::isCellVisibleToActor(const std::string& actor_key, const WorldCellCoord& coord) const {
    auto vis = getCellVisibility(actor_key, coord.cellId());
    return vis == WorldCellVisibility::Visible;
}

Result<MoveActorResult> WorldGridRuntime::moveActor(const std::string& actor_key, const WorldCellCoord& target) {
    MoveActorResult result;
    result.block_reason = WorldMoveBlockReason::None;

    auto actor_it = actors_.find(actor_key);
    if (actor_it == actors_.end()) {
        result.block_reason = WorldMoveBlockReason::ActorMissing;
        return Result<MoveActorResult>::ok(std::move(result));
    }

    auto& actor = actor_it->second;
    result.from = actor.coord;

    // Check target cell exists
    auto target_cell_it = cells_.find(target.cellId());
    if (target_cell_it == cells_.end()) {
        result.block_reason = WorldMoveBlockReason::OutOfBounds;
        return Result<MoveActorResult>::ok(std::move(result));
    }

    // Check layer
    if (target.layer_key != actor.coord.layer_key) {
        if (config_.layer_policy == WorldLayerPolicy::SurfaceOnly && target.layer_key != "surface") {
            result.block_reason = WorldMoveBlockReason::UnknownLayer;
            return Result<MoveActorResult>::ok(std::move(result));
        }
    }

    // Check adjacency
    if (!isAdjacent(actor.coord, target)) {
        result.block_reason = WorldMoveBlockReason::NotAdjacent;
        return Result<MoveActorResult>::ok(std::move(result));
    }

    // Check cell blocks movement
    if (target_cell_it->second.blocks_movement) {
        result.block_reason = WorldMoveBlockReason::CellBlocked;
        return Result<MoveActorResult>::ok(std::move(result));
    }

    // Check entity blocks movement
    for (const auto& entity_id : target_cell_it->second.entity_ids) {
        auto ent_it = entities_.find(entity_id);
        if (ent_it != entities_.end() && ent_it->second.blocks_movement) {
            result.block_reason = WorldMoveBlockReason::EntityBlocked;
            return Result<MoveActorResult>::ok(std::move(result));
        }
    }

    // Perform move
    WorldCellCoord old_coord = actor.coord;
    actor.coord = target;
    result.to = target;
    result.moved = true;

    // Update old cell entity list
    auto old_cell_it = cells_.find(old_coord.cellId());
    if (old_cell_it != cells_.end()) {
        auto& old_entities = old_cell_it->second.entity_ids;
        old_entities.erase(
            std::remove(old_entities.begin(), old_entities.end(), actor.entity_id),
            old_entities.end());
        result.changed_cell_ids.push_back(old_cell_it->second.cell_id);
    }

    // Update new cell entity list
    target_cell_it->second.entity_ids.push_back(actor.entity_id);
    result.changed_cell_ids.push_back(target_cell_it->second.cell_id);
    result.changed_entity_ids.push_back(actor.entity_id);

    // Update entity instance coordinate
    auto ent_it = entities_.find(actor.entity_id);
    if (ent_it != entities_.end()) {
        ent_it->second.coord = target;
    }

    // Update exploration
    updateExplorationForActor(actor_key);

    // Add newly discovered cells to changed list
    auto& exp = exploration_[actor_key];
    for (const auto& [cell_id, visibility] : exp.cell_visibility_by_id) {
        if (visibility == WorldCellVisibility::Visible) {
            if (std::find(result.changed_cell_ids.begin(), result.changed_cell_ids.end(), cell_id) == result.changed_cell_ids.end()) {
                result.changed_cell_ids.push_back(cell_id);
            }
        }
    }

    incrementStateVersion();
    return Result<MoveActorResult>::ok(std::move(result));
}

Result<InspectWorldResult> WorldGridRuntime::inspect(
    const std::string& actor_key,
    const world_command::WorldCommandTargetDto& target) const {
    InspectWorldResult result;
    result.found = false;

    auto actor_it = actors_.find(actor_key);
    if (actor_it == actors_.end()) {
        return Result<InspectWorldResult>::ok(std::move(result));
    }

    auto& actor = actor_it->second;

    if (target.target_kind == world_command::WorldCommandTargetKind::Coordinate && target.target_coord) {
        WorldCellCoord coord{target.target_coord->x, target.target_coord->y, target.target_coord->layer_key};
        std::string cid = coord.cellId();
        auto vis = getCellVisibility(actor_key, cid);
        if (vis == WorldCellVisibility::Unknown) {
            return Result<InspectWorldResult>::ok(std::move(result));
        }

        auto cell_it = cells_.find(cid);
        if (cell_it != cells_.end()) {
            result.found = true;
            result.inspected_cell_id = cid;
            result.description_text = "Terrain: " + cell_it->second.terrain_key;
            if (!cell_it->second.entity_ids.empty()) {
                result.description_text += "\nEntities here:";
                for (const auto& eid : cell_it->second.entity_ids) {
                    auto ent_it = entities_.find(eid);
                    if (ent_it != entities_.end()) {
                        result.description_text += "\n- " + ent_it->second.display_name_key;
                    }
                }
            }
        }
    } else if (target.target_kind == world_command::WorldCommandTargetKind::Entity) {
        auto ent_it = entities_.find(target.target_entity_id);
        if (ent_it != entities_.end() && ent_it->second.coord) {
            auto vis = getCellVisibility(actor_key, ent_it->second.coord->cellId());
            if (vis != WorldCellVisibility::Unknown) {
                result.found = true;
                result.inspected_entity_id = ent_it->second.entity_id;
                result.description_text = "Entity: " + ent_it->second.display_name_key;
            }
        }
    }

    return Result<InspectWorldResult>::ok(std::move(result));
}

Result<AdvanceWorldTimeResult> WorldGridRuntime::advanceWorldTime(uint64_t tick_delta) {
    AdvanceWorldTimeResult result;
    result.previous_tick = world_tick_;
    world_tick_ += tick_delta;
    result.new_tick = world_tick_;
    incrementStateVersion();
    result.new_state_version = state_version_;
    return Result<AdvanceWorldTimeResult>::ok(std::move(result));
}

Result<WorldRuntimeSnapshot> WorldGridRuntime::snapshotForDebug() const {
    WorldRuntimeSnapshot snapshot;
    snapshot.world_id = config_.world_id;
    snapshot.seed = config_.seed;
    snapshot.world_tick = world_tick_;
    snapshot.state_version = state_version_;
    snapshot.cells = cells_;
    snapshot.entities = entities_;
    snapshot.actors = actors_;
    snapshot.resource_nodes = resource_nodes_;
    return Result<WorldRuntimeSnapshot>::ok(std::move(snapshot));
}

// ---------------------------------------------------------------------------
// P46: Generated cell creation/updating
// ---------------------------------------------------------------------------

Result<void> WorldGridRuntime::createOrUpdateGeneratedCell(
    const WorldCellCoord& coord,
    const std::string& terrain_key,
    const std::string& region_id,
    bool blocks_movement,
    int movement_cost,
    const std::vector<std::string>& tag_keys) {
    std::string cell_id = coord.cellId();
    auto it = cells_.find(cell_id);
    if (it != cells_.end()) {
        it->second.terrain_key = terrain_key;
        it->second.region_id = region_id;
        it->second.blocks_movement = blocks_movement;
        it->second.movement_cost = movement_cost;
        it->second.tag_keys = tag_keys;
    } else {
        WorldCellRuntime cell;
        cell.cell_id = cell_id;
        cell.coord = coord;
        cell.terrain_key = terrain_key;
        cell.region_id = region_id;
        cell.generated = true;
        cell.blocks_movement = blocks_movement;
        cell.movement_cost = movement_cost;
        cell.tag_keys = tag_keys;
        cells_[cell_id] = std::move(cell);
    }
    incrementStateVersion();
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// P46: Resource node runtime
// ---------------------------------------------------------------------------

Result<void> WorldGridRuntime::upsertGeneratedResourceNode(const WorldResourceNodeRuntime& node) {
    resource_nodes_[node.node_id] = node;
    incrementStateVersion();
    return Result<void>::ok();
}

Result<const WorldResourceNodeRuntime*> WorldGridRuntime::findResourceNode(const std::string& node_id) const {
    auto it = resource_nodes_.find(node_id);
    if (it == resource_nodes_.end()) {
        return Result<const WorldResourceNodeRuntime*>::fail(
            makeError(ErrorCode::id_not_found, "resource_node_not_found", "Resource node not found: " + node_id));
    }
    return Result<const WorldResourceNodeRuntime*>::ok(&it->second);
}

Result<void> WorldGridRuntime::updateResourceNodeRuntime(const WorldResourceNodeRuntime& node) {
    // P47 semantic: same as upsert, but clearly marks harvest state updates
    resource_nodes_[node.node_id] = node;
    incrementStateVersion();
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// P46: Region generation tracking
// ---------------------------------------------------------------------------

bool WorldGridRuntime::isRegionGenerated(const std::string& region_id) const {
    return generated_regions_.find(region_id) != generated_regions_.end();
}

void WorldGridRuntime::markRegionGenerated(const std::string& region_id) {
    generated_regions_.insert(region_id);
}

// ---------------------------------------------------------------------------
// IWorldEntityLocationPort implementation (P45)
// ---------------------------------------------------------------------------

Result<void> WorldGridRuntime::removeEntityFromMap(const std::string& entity_id) {
    auto ent_it = entities_.find(entity_id);
    if (ent_it == entities_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "entity_not_found", "Entity not found: " + entity_id));
    }

    auto& entity = ent_it->second;
    if (entity.coord.has_value()) {
        auto cell_it = cells_.find(entity.coord.value().cellId());
        if (cell_it != cells_.end()) {
            auto& entity_ids = cell_it->second.entity_ids;
            entity_ids.erase(
                std::remove(entity_ids.begin(), entity_ids.end(), entity_id),
                entity_ids.end());
        }
        entity.coord = std::nullopt;
    }

    entity.location_kind = WorldEntityLocationKind::Nowhere;
    incrementStateVersion();
    return Result<void>::ok();
}

Result<void> WorldGridRuntime::placeEntityOnMap(const std::string& entity_id, const WorldCellCoord& coord) {
    auto ent_it = entities_.find(entity_id);
    if (ent_it == entities_.end()) {
        // Create a new entity instance if it doesn't exist (for dropped items that got new IDs)
        WorldEntityInstance new_entity;
        new_entity.entity_id = entity_id;
        // Try to extract entity_key from entity_id (format: key_layer_x_y or key_layer_x_y_drop_N)
        size_t first_underscore = entity_id.find('_');
        if (first_underscore != std::string::npos) {
            new_entity.entity_key = entity_id.substr(0, first_underscore);
        } else {
            new_entity.entity_key = entity_id;
        }
        new_entity.display_name_key = "entity." + new_entity.entity_key;
        new_entity.coord = coord;
        new_entity.location_kind = WorldEntityLocationKind::OnMap;
        new_entity.visible_by_default = true;
        entities_[entity_id] = std::move(new_entity);
    } else {
        auto& entity = ent_it->second;
        entity.coord = coord;
        entity.location_kind = WorldEntityLocationKind::OnMap;
    }

    auto cell_it = cells_.find(coord.cellId());
    if (cell_it != cells_.end()) {
        // Avoid duplicates
        auto& entity_ids = cell_it->second.entity_ids;
        if (std::find(entity_ids.begin(), entity_ids.end(), entity_id) == entity_ids.end()) {
            entity_ids.push_back(entity_id);
        }
    }

    incrementStateVersion();
    return Result<void>::ok();
}

Result<void> WorldGridRuntime::setEntityLocationKind(const std::string& entity_id, WorldEntityLocationKind kind) {
    auto ent_it = entities_.find(entity_id);
    if (ent_it == entities_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "entity_not_found", "Entity not found: " + entity_id));
    }
    ent_it->second.location_kind = kind;
    incrementStateVersion();
    return Result<void>::ok();
}

Result<void> WorldGridRuntime::setEntityStackData(const std::string& entity_id, int quantity, const std::string& stack_key, bool stackable) {
    auto ent_it = entities_.find(entity_id);
    if (ent_it == entities_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "entity_not_found", "Entity not found: " + entity_id));
    }
    ent_it->second.quantity = quantity;
    ent_it->second.stack_key = stack_key;
    ent_it->second.stackable = stackable;
    incrementStateVersion();
    return Result<void>::ok();
}

Result<void> WorldGridRuntime::destroyEntity(const std::string& entity_id) {
    auto ent_it = entities_.find(entity_id);
    if (ent_it == entities_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "entity_not_found", "Entity not found: " + entity_id));
    }

    // Remove from cell if on map
    if (ent_it->second.coord.has_value()) {
        auto cell_it = cells_.find(ent_it->second.coord.value().cellId());
        if (cell_it != cells_.end()) {
            auto& entity_ids = cell_it->second.entity_ids;
            entity_ids.erase(
                std::remove(entity_ids.begin(), entity_ids.end(), entity_id),
                entity_ids.end());
        }
    }

    entities_.erase(ent_it);
    incrementStateVersion();
    return Result<void>::ok();
}

WorldCellVisibility WorldGridRuntime::getCellVisibilityForActor(const std::string& actor_key, const std::string& cell_id) const {
    return getCellVisibility(actor_key, cell_id);
}

Result<void> WorldGridRuntime::spawnEntityOnMap(
    const std::string& entity_id,
    const std::string& entity_key,
    const std::string& display_name_key,
    const WorldCellCoord& coord,
    int quantity,
    const std::string& stack_key,
    bool stackable,
    const std::vector<std::string>& state_keys,
    const std::map<std::string, double>& numeric_states,
    const std::vector<std::string>& tag_keys) {

    // P46: Reject spawning on non-existent cells to prevent ghost entities
    auto cell_it = cells_.find(coord.cellId());
    if (cell_it == cells_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "cell_not_found", "Cannot spawn entity on missing cell: " + coord.cellId()));
    }

    auto ent_it = entities_.find(entity_id);
    if (ent_it != entities_.end()) {
        // Entity already exists: update
        auto& entity = ent_it->second;
        entity.entity_key = entity_key;
        entity.display_name_key = display_name_key;
        entity.coord = coord;
        entity.location_kind = WorldEntityLocationKind::OnMap;
        entity.quantity = quantity;
        entity.stack_key = stack_key;
        entity.stackable = stackable;
        entity.state_keys = state_keys;
        entity.numeric_states = numeric_states;
        entity.tag_keys = tag_keys;
    } else {
        // Create new entity
        WorldEntityInstance entity;
        entity.entity_id = entity_id;
        entity.entity_key = entity_key;
        entity.display_name_key = display_name_key;
        entity.coord = coord;
        entity.location_kind = WorldEntityLocationKind::OnMap;
        entity.quantity = quantity;
        entity.stack_key = stack_key;
        entity.stackable = stackable;
        entity.state_keys = state_keys;
        entity.numeric_states = numeric_states;
        entity.tag_keys = tag_keys;
        entity.visible_by_default = true;
        entities_[entity_id] = std::move(entity);
    }

    auto& entity_ids = cell_it->second.entity_ids;
    if (std::find(entity_ids.begin(), entity_ids.end(), entity_id) == entity_ids.end()) {
        entity_ids.push_back(entity_id);
    }

    incrementStateVersion();
    return Result<void>::ok();
}

} // namespace pathfinder::world_runtime
