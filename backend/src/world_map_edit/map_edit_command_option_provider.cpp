#include "pathfinder/world_map_edit/map_edit_command_option_provider.h"
#include "pathfinder/world_map_edit/map_edit_content_policy.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>

namespace pathfinder::world_map_edit {
namespace {

using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandOptionDto;
using pathfinder::world_command::WorldCommandTargetKind;
using pathfinder::world_runtime::WorldCellVisibility;

std::atomic<uint64_t> g_map_edit_option_counter{0};

std::string makeOptionId(const std::string& prefix) {
    const auto n = ++g_map_edit_option_counter;
    const auto ts = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    return prefix + "_" + std::to_string(ts) + "_" + std::to_string(n);
}

bool canEditCell(
    const pathfinder::world_runtime::WorldActorRuntime& actor,
    const pathfinder::world_runtime::WorldCellRuntime& cell) {
    if (cell.coord.layer_key != actor.coord.layer_key) return false;
    return std::abs(cell.coord.x - actor.coord.x) <= actor.vision_radius &&
           std::abs(cell.coord.y - actor.coord.y) <= actor.vision_radius;
}

} // namespace

MapEditCommandOptionProvider::MapEditCommandOptionProvider(
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry,
    const pathfinder::content::ContentRegistry& content_registry)
    : registry_(registry)
    , content_registry_(content_registry) {}

std::vector<WorldCommandOptionDto> MapEditCommandOptionProvider::buildOptions(
    const pathfinder::world_runtime::WorldActorRuntime& actor,
    const pathfinder::world_runtime::IWorldRuntime& runtime) const {
    std::vector<WorldCommandOptionDto> options;
    const bool can_paint = registry_.findByKind(WorldCommandKind::PaintTerrain) != nullptr;
    const bool can_spawn = registry_.findByKind(WorldCommandKind::SpawnEntity) != nullptr;
    if (!can_paint && !can_spawn) return options;

    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_error()) return options;

    const auto terrains = buildPaintableTerrains();
    const auto objects = buildRawPlaceableObjects(content_registry_);
    for (const auto& [cell_id, cell] : snap_res.value().cells) {
        if (!canEditCell(actor, cell)) continue;

        if (can_paint) {
            for (const auto& terrain : terrains) {
                WorldCommandOptionDto opt;
                opt.option_id = makeOptionId("opt_paint_terrain");
                opt.command_kind = WorldCommandKind::PaintTerrain;
                opt.command_key = "paint_terrain";
                opt.label_text = "绘制" + terrain.display_name;
                opt.target.target_kind = WorldCommandTargetKind::Coordinate;
                opt.target.target_coord = pathfinder::world_command::WorldCoordinateDto{cell.coord.x, cell.coord.y, cell.coord.layer_key};
                opt.target.target_item_key = terrain.terrain_key;
                opt.enabled = true;
                opt.priority = 30;
                options.push_back(std::move(opt));
            }
        }

        if (can_spawn && !cell.blocks_movement) {
            for (const auto& object : objects) {
                WorldCommandOptionDto opt;
                opt.option_id = makeOptionId("opt_place_raw_object");
                opt.command_kind = WorldCommandKind::SpawnEntity;
                opt.command_key = "place_raw_object";
                opt.label_text = "投放" + object.display_name;
                opt.target.target_kind = WorldCommandTargetKind::Coordinate;
                opt.target.target_coord = pathfinder::world_command::WorldCoordinateDto{cell.coord.x, cell.coord.y, cell.coord.layer_key};
                opt.target.target_item_key = object.object_key;
                opt.enabled = true;
                opt.priority = 31;
                options.push_back(std::move(opt));
            }
        }
    }
    return options;
}

} // namespace pathfinder::world_map_edit
