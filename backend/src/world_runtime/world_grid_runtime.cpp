#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/foundation/error.h"
#include <cmath>
#include <algorithm>
#include <set>

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
    resource_nodes_.clear();
    generated_regions_.clear();
    return Result<void>::ok();
}

uint64_t WorldGridRuntime::currentWorldTick() const {
    return world_tick_;
}

uint64_t WorldGridRuntime::stateVersion() const {
    return state_version_;
}

std::string WorldGridRuntime::worldId() const {
    return config_.world_id;
}

uint64_t WorldGridRuntime::worldSeed() const {
    return config_.seed;
}

int WorldGridRuntime::regionSize() const {
    return config_.region_size;
}

std::string WorldGridRuntime::generatorVersion() const {
    return config_.generator_version;
}

std::string WorldGridRuntime::contentVersion() const {
    return config_.content_version;
}

std::string WorldGridRuntime::worldgenProfileKey() const {
    return config_.worldgen_profile_key;
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

int numericStateAsInt(const std::map<std::string, double>& states, const std::string& key, int fallback) {
    auto it = states.find(key);
    if (it == states.end()) return fallback;
    return std::max(0, static_cast<int>(std::round(it->second)));
}

void syncActorHealthToEntity(WorldEntityInstance& entity, const WorldActorRuntime& actor) {
    entity.numeric_states["health"] = static_cast<double>(actor.health);
    entity.numeric_states["max_health"] = static_cast<double>(actor.max_health);
    entity.numeric_states["alive"] = actor.alive ? 1.0 : 0.0;
}
} // anonymous namespace

Result<void> WorldGridRuntime::generateInitialWorld(const WorldRuntimeConfig& config) {
    config_ = config;
    cells_.clear();
    entities_.clear();
    actors_.clear();
    exploration_.clear();
    resource_nodes_.clear();
    generated_regions_.clear();
    world_tick_ = 0;
    state_version_ = 1;

    // Use seed for deterministic generation
    uint64_t seed = config.seed;

    int radius = config.initial_region_radius;
    int size = config.region_size;

    for (int rx = -radius; rx <= radius; ++rx) {
        for (int ry = -radius; ry <= radius; ++ry) {
            std::string region_id = config.world_id + ":surface:region:" + std::to_string(rx) + ":" + std::to_string(ry) + ":" + std::to_string(size);
            int min_local = -(size / 2);
            int max_local = (size / 2) - (size % 2 == 0 ? 1 : 0);
            for (int local_x = min_local; local_x <= max_local; ++local_x) {
                for (int local_y = min_local; local_y <= max_local; ++local_y) {
                    int world_x = rx * size + local_x;
                    int world_y = ry * size + local_y;
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

                    // P62: content objects are instantiated by WorldGenerationService
                    // from ContentRegistry-backed worldgen rules. This legacy runtime
                    // initializer must not create inspect-only hardcoded entities.


                    cells_[cell.cell_id] = std::move(cell);
                }
            }
            generated_regions_.insert(region_id);
        }
    }

    // Create player actor at origin
    auto setup_res = setupPlayerActor(config);
    if (setup_res.is_error()) return setup_res;

    incrementStateVersion();
    return Result<void>::ok();
}

Result<void> WorldGridRuntime::setupPlayerActor(const WorldRuntimeConfig& config) {
    // Avoid duplicate player actor
    if (actors_.find("player") != actors_.end()) {
        return Result<void>::ok();
    }

    WorldActorRuntime player;
    player.actor_key = "player";
    player.coord = WorldCellCoord{0, 0, "surface"};
    player.entity_id = makeStableEntityId("player", player.coord);
    player.vision_radius = config.default_vision_radius;
    player.is_player_controlled = true;
    player.max_health = 10;
    player.health = 10;
    player.alive = true;
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
    player_entity.tag_keys = {"actor", "humanoid", "player", "prey"};
    syncActorHealthToEntity(player_entity, actors_.at("player"));
    entities_[player_entity.entity_id] = std::move(player_entity);

    // Add player to origin cell if it exists
    auto origin_it = cells_.find(player_coord.cellId());
    if (origin_it != cells_.end()) {
        auto& eids = origin_it->second.entity_ids;
        if (std::find(eids.begin(), eids.end(), player_entity_id) == eids.end()) {
            eids.push_back(player_entity_id);
        }
    }

    // Initialize exploration for player
    updateExplorationForActor("player");
    return Result<void>::ok();
}


Result<void> WorldGridRuntime::spawnActor(
    const std::string& actor_key,
    const std::string& entity_key,
    const std::string& display_name_key,
    const WorldCellCoord& coord,
    int vision_radius,
    bool is_player_controlled,
    const std::vector<std::string>& tag_keys,
    const std::map<std::string, double>& numeric_states,
    const std::string& entity_id_override) {

    if (actor_key.empty() || entity_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "actor_key_or_entity_key_empty"));
    }
    if (actors_.find(actor_key) != actors_.end()) {
        return Result<void>::ok();
    }
    auto cell_it = cells_.find(coord.cellId());
    if (cell_it == cells_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "actor_spawn_cell_missing", "Actor spawn cell missing: " + coord.cellId()));
    }
    if (cell_it->second.blocks_movement) {
        return Result<void>::fail(
            makeError(ErrorCode::state_change_invalid, "actor_spawn_cell_blocked", "Actor spawn cell blocked: " + coord.cellId()));
    }

    WorldActorRuntime actor;
    actor.actor_key = actor_key;
    actor.coord = coord;
    actor.entity_id = entity_id_override.empty() ? makeStableEntityId(entity_key, coord) : entity_id_override;
    actor.vision_radius = std::max(1, vision_radius);
    actor.is_player_controlled = is_player_controlled;
    actor.max_health = std::max(1, numericStateAsInt(numeric_states, "max_health", 10));
    actor.health = std::clamp(numericStateAsInt(numeric_states, "health", actor.max_health), 0, actor.max_health);
    actor.alive = actor.health > 0;
    const auto entity_id = actor.entity_id;
    actors_[actor.actor_key] = actor;

    WorldEntityInstance entity;
    entity.entity_id = entity_id;
    entity.entity_key = entity_key;
    entity.display_name_key = display_name_key.empty() ? "entity." + entity_key : display_name_key;
    entity.coord = coord;
    entity.location_kind = WorldEntityLocationKind::OnMap;
    entity.visible_by_default = true;
    entity.tag_keys = tag_keys;
    entity.numeric_states = numeric_states;
    syncActorHealthToEntity(entity, actor);
    entities_[entity.entity_id] = std::move(entity);

    auto& entity_ids = cell_it->second.entity_ids;
    if (std::find(entity_ids.begin(), entity_ids.end(), entity_id) == entity_ids.end()) {
        entity_ids.push_back(entity_id);
    }

    updateExplorationForActor(actor_key);
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

    if (!actor.alive) {
        result.block_reason = WorldMoveBlockReason::ActorMissing;
        return Result<MoveActorResult>::ok(std::move(result));
    }

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
    std::set<std::string> visible_before_move;
    auto exp_before_it = exploration_.find(actor_key);
    if (exp_before_it != exploration_.end()) {
        for (const auto& [cell_id, visibility] : exp_before_it->second.cell_visibility_by_id) {
            if (visibility == WorldCellVisibility::Visible) {
                visible_before_move.insert(cell_id);
            }
        }
    }

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

    // Add visibility boundary changes to the patch without sending the whole
    // window every step. Newly visible cells must appear, and cells that leave
    // vision must be resent as Discovered so clients keep explored terrain but
    // stop treating it as currently visible.
    auto& exp = exploration_[actor_key];
    for (const auto& [cell_id, visibility] : exp.cell_visibility_by_id) {
        const bool was_visible = visible_before_move.find(cell_id) != visible_before_move.end();
        const bool became_visible = visibility == WorldCellVisibility::Visible && !was_visible;
        const bool became_discovered = visibility == WorldCellVisibility::Discovered && was_visible;
        if (became_visible || became_discovered) {
            if (std::find(result.changed_cell_ids.begin(), result.changed_cell_ids.end(), cell_id) == result.changed_cell_ids.end()) {
                result.changed_cell_ids.push_back(cell_id);
            }
        }
    }

    incrementStateVersion();
    return Result<MoveActorResult>::ok(std::move(result));
}


Result<ActorHealthChangeResult> WorldGridRuntime::applyActorHealthDelta(
    const std::string& actor_key,
    int delta,
    const std::vector<std::string>& reason_keys) {

    ActorHealthChangeResult result;
    result.actor_key = actor_key;
    result.reason_keys = reason_keys;

    auto actor_it = actors_.find(actor_key);
    if (actor_it == actors_.end()) {
        return Result<ActorHealthChangeResult>::fail(
            makeError(ErrorCode::id_not_found, "actor_not_found", "Actor not found: " + actor_key));
    }

    auto& actor = actor_it->second;
    if (actor.max_health <= 0) {
        actor.max_health = 1;
    }
    result.entity_id = actor.entity_id;
    result.previous_health = actor.health;
    actor.health = std::clamp(actor.health + delta, 0, actor.max_health);
    actor.alive = actor.health > 0;

    result.ok = true;
    result.new_health = actor.health;
    result.max_health = actor.max_health;
    result.alive = actor.alive;

    auto ent_it = entities_.find(actor.entity_id);
    if (ent_it != entities_.end()) {
        syncActorHealthToEntity(ent_it->second, actor);
        result.changed_entity_ids.push_back(actor.entity_id);
    }

    incrementStateVersion();
    return Result<ActorHealthChangeResult>::ok(std::move(result));
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

Result<void> WorldGridRuntime::refreshActorExploration(const std::string& actor_key) {
    if (actors_.find(actor_key) == actors_.end()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "actor_not_found", "Actor not found: " + actor_key));
    }
    updateExplorationForActor(actor_key);
    return Result<void>::ok();
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
    bool is_new = (it == cells_.end());
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

    // P57: when a new cell is created within an actor's vision, update exploration
    if (is_new) {
        for (const auto& [actor_key, actor] : actors_) {
            if (coord.layer_key != actor.coord.layer_key) continue;
            int dx = std::abs(actor.coord.x - coord.x);
            int dy = std::abs(actor.coord.y - coord.y);
            if (dx + dy <= actor.vision_radius) {
                exploration_[actor_key].cell_visibility_by_id[cell_id] = WorldCellVisibility::Visible;
            }
        }
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

Result<std::vector<WorldResourceNodeRuntime>> WorldGridRuntime::queryResourceNodesAtCell(const WorldCellCoord& coord) const {
    std::vector<WorldResourceNodeRuntime> result;
    std::string target_cell_id = coord.cellId();
    for (const auto& [node_id, node] : resource_nodes_) {
        if (node.coord.cellId() == target_cell_id) {
            result.push_back(node);
        }
    }
    return Result<std::vector<WorldResourceNodeRuntime>>::ok(std::move(result));
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

    for (auto actor_it = actors_.begin(); actor_it != actors_.end();) {
        if (actor_it->second.entity_id == entity_id) {
            exploration_.erase(actor_it->first);
            actor_it = actors_.erase(actor_it);
        } else {
            ++actor_it;
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

Result<void> WorldGridRuntime::spawnEntityInInventory(
    const std::string& entity_id,
    const std::string& entity_key,
    const std::string& display_name_key,
    const std::string& owner_ref,
    int quantity,
    const std::string& stack_key,
    bool stackable,
    const std::vector<std::string>& state_keys,
    const std::map<std::string, double>& numeric_states,
    const std::vector<std::string>& tag_keys) {

    auto ent_it = entities_.find(entity_id);
    if (ent_it != entities_.end()) {
        // Entity already exists: update stack data and location
        auto& entity = ent_it->second;
        entity.entity_key = entity_key;
        entity.display_name_key = display_name_key;
        entity.coord = std::nullopt;
        entity.location_kind = WorldEntityLocationKind::InInventory;
        entity.owner_ref = owner_ref;
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
        entity.coord = std::nullopt;
        entity.location_kind = WorldEntityLocationKind::InInventory;
        entity.owner_ref = owner_ref;
        entity.quantity = quantity;
        entity.stack_key = stack_key;
        entity.stackable = stackable;
        entity.state_keys = state_keys;
        entity.numeric_states = numeric_states;
        entity.tag_keys = tag_keys;
        entity.visible_by_default = true;
        entities_[entity_id] = std::move(entity);
    }

    // Remove from any cell entity list if previously on map
    for (auto& [cid, cell] : cells_) {
        auto& eids = cell.entity_ids;
        eids.erase(std::remove(eids.begin(), eids.end(), entity_id), eids.end());
    }

    incrementStateVersion();
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// P59: IWorldRegionStateQueryPort implementation
// ---------------------------------------------------------------------------

Result<world_region_state::WorldRegionRuntimeSlice> WorldGridRuntime::readRegionSlice(
    const world_generation::WorldRegionKey& key) const {

    world_region_state::WorldRegionRuntimeSlice slice;
    std::string expected_region_id = key.regionRuntimeId();

    // Collect cells
    for (const auto& [cell_id, cell] : cells_) {
        if (cell.region_id == expected_region_id) {
            world_region_state::WorldRegionCellStateRecord record;
            record.cell_id = cell.cell_id;
            record.coord = cell.coord;
            record.terrain_key = cell.terrain_key;
            record.region_id = cell.region_id;
            record.generated = cell.generated;
            record.blocks_movement = cell.blocks_movement;
            record.movement_cost = cell.movement_cost;
            record.entity_ids = cell.entity_ids;
            record.tag_keys = cell.tag_keys;
            slice.cells.push_back(std::move(record));
        }
    }

    // Collect OnMap entities in this region
    // P59: Actors are cross-region runtime objects; exclude them from region snapshot.
    for (const auto& [entity_id, entity] : entities_) {
        if (entity.location_kind != WorldEntityLocationKind::OnMap || !entity.coord.has_value()) {
            continue;
        }
        // Skip actor entities (player, NPCs, beasts)
        if (actors_.find(entity.entity_key) != actors_.end()) {
            continue;
        }
        auto cell_it = cells_.find(entity.coord.value().cellId());
        if (cell_it != cells_.end() && cell_it->second.region_id == expected_region_id) {
            world_region_state::WorldRegionEntityStateRecord record;
            record.entity_id = entity.entity_id;
            record.entity_key = entity.entity_key;
            record.display_name_key = entity.display_name_key;
            record.location_kind = entity.location_kind;
            record.coord = entity.coord.value();
            record.owner_ref = entity.owner_ref;
            record.container_ref = entity.container_ref;
            record.blocks_movement = entity.blocks_movement;
            record.visible_by_default = entity.visible_by_default;
            record.tag_keys = entity.tag_keys;
            record.quantity = entity.quantity;
            record.stackable = entity.stackable;
            record.stack_key = entity.stack_key;
            record.state_keys = entity.state_keys;
            record.numeric_states = entity.numeric_states;
            slice.on_map_entities.push_back(std::move(record));
        }
    }

    // Collect resource nodes in this region
    for (const auto& [node_id, node] : resource_nodes_) {
        auto cell_it = cells_.find(node.coord.cellId());
        if (cell_it != cells_.end() && cell_it->second.region_id == expected_region_id) {
            world_region_state::WorldRegionResourceNodeStateRecord record;
            record.node_id = node.node_id;
            record.resource_key = node.resource_key;
            record.coord = node.coord;
            record.node_kind_str = node.node_kind_str;
            record.node_state_str = node.node_state_str;
            record.remaining_charges = node.remaining_charges;
            record.max_charges = node.max_charges;
            record.required_action_key = node.required_action_key;
            record.required_tool_key = node.required_tool_key;
            record.output_entity_keys = node.output_entity_keys;
            record.tag_keys = node.tag_keys;
            slice.resource_nodes.push_back(std::move(record));
        }
    }

    // Collect exploration slices restricted to this region
    for (const auto& [actor_key, exp] : exploration_) {
        world_region_state::WorldRegionExplorationStateSlice exp_slice;
        exp_slice.actor_key = actor_key;
        for (const auto& [cell_id, visibility] : exp.cell_visibility_by_id) {
            auto cell_it = cells_.find(cell_id);
            if (cell_it != cells_.end() && cell_it->second.region_id == expected_region_id) {
                exp_slice.cell_visibility_by_id[cell_id] = visibility;
            }
        }
        if (!exp_slice.cell_visibility_by_id.empty()) {
            slice.exploration_slices.push_back(std::move(exp_slice));
        }
    }

    return Result<world_region_state::WorldRegionRuntimeSlice>::ok(std::move(slice));
}

Result<void> WorldGridRuntime::applyExplorationVisibility(
    const std::string& actor_key,
    const std::map<std::string, WorldCellVisibility>& cell_visibility_by_id) {
    auto& exp = exploration_[actor_key];
    exp.actor_key = actor_key;
    for (const auto& [cell_id, visibility] : cell_visibility_by_id) {
        if (cells_.find(cell_id) != cells_.end()) {
            exp.cell_visibility_by_id[cell_id] = visibility;
        }
    }
    incrementStateVersion();
    return Result<void>::ok();
}

Result<std::vector<std::string>> WorldGridRuntime::detachRegion(const std::string& region_id) {
    std::vector<std::string> removed_cell_ids;
    std::vector<std::string> removed_entity_ids;
    std::vector<std::string> removed_node_ids;

    // Find all cells in this region
    std::set<std::string> region_cell_ids;
    for (const auto& [cell_id, cell] : cells_) {
        if (cell.region_id == region_id) {
            region_cell_ids.insert(cell_id);
        }
    }

    // Remove OnMap entities in this region (but not actors)
    for (auto it = entities_.begin(); it != entities_.end(); ) {
        const auto& entity = it->second;
        if (entity.location_kind == WorldEntityLocationKind::OnMap && entity.coord.has_value()) {
            if (region_cell_ids.find(entity.coord.value().cellId()) != region_cell_ids.end()) {
                // Do not remove player or actor entities
                bool is_actor = (actors_.find(entity.entity_key) != actors_.end());
                if (!is_actor) {
                    removed_entity_ids.push_back(entity.entity_id);
                    it = entities_.erase(it);
                    continue;
                }
            }
        }
        ++it;
    }

    // Remove resource nodes in this region
    for (auto it = resource_nodes_.begin(); it != resource_nodes_.end(); ) {
        if (region_cell_ids.find(it->second.coord.cellId()) != region_cell_ids.end()) {
            removed_node_ids.push_back(it->second.node_id);
            it = resource_nodes_.erase(it);
        } else {
            ++it;
        }
    }

    // Remove cells
    for (const auto& cell_id : region_cell_ids) {
        cells_.erase(cell_id);
        removed_cell_ids.push_back(cell_id);
    }

    // Remove from generated_regions_
    generated_regions_.erase(region_id);

    // Clean up exploration references to removed cells
    for (auto& [actor_key, exp] : exploration_) {
        for (const auto& cell_id : removed_cell_ids) {
            exp.cell_visibility_by_id.erase(cell_id);
        }
    }

    incrementStateVersion();

    // Combine all removed IDs for return
    std::vector<std::string> all_removed;
    all_removed.insert(all_removed.end(), removed_cell_ids.begin(), removed_cell_ids.end());
    all_removed.insert(all_removed.end(), removed_entity_ids.begin(), removed_entity_ids.end());
    all_removed.insert(all_removed.end(), removed_node_ids.begin(), removed_node_ids.end());
    return Result<std::vector<std::string>>::ok(std::move(all_removed));
}

} // namespace pathfinder::world_runtime
