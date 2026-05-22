#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::world_generation {

enum class WorldRegionEnsureKind {
    Unknown,
    BootstrapWindow,
    RefreshProjectionWindow,
    AvailableCommandWindow,
    MoveTarget,
    CommandExecution,
    AgentPlanning,
    SystemPrewarm,
    TestOnly
};

std::string toString(WorldRegionEnsureKind kind);

enum class WorldRegionEnsureStatus {
    Unknown,
    AlreadyAvailable,
    GeneratedAndApplied,
    RestoredSnapshot,
    SkippedByPolicy,
    GenerationFailed,
    ApplyFailed,
    InvalidRequest,
    LimitExceeded,
    TestOnly
};

std::string toString(WorldRegionEnsureStatus status);

enum class WorldRegionCoverageMode {
    Unknown,
    ExactCoords,
    ActorVisionWindow,
    ActorNeighborMoves,
    RectWindow,
    RegionList,
    TestOnly
};

std::string toString(WorldRegionCoverageMode mode);

struct WorldRegionKey {
    std::string world_id;
    std::string layer_key;
    int rx = 0;
    int ry = 0;
    int region_size = 16;

    std::string toString() const {
        return world_id + ":" + layer_key + ":region:" + std::to_string(rx) + ":" + std::to_string(ry) + ":" + std::to_string(region_size);
    }
    // P59: Runtime-stable region identity used for cell.region_id and generated_regions_ tracking.
    std::string regionRuntimeId() const {
        return world_id + ":" + layer_key + ":region:" + std::to_string(rx) + ":" + std::to_string(ry) + ":" + std::to_string(region_size);
    }
    bool operator==(const WorldRegionKey& other) const;
    bool operator<(const WorldRegionKey& other) const;
};

struct WorldRegionBounds {
    WorldRegionKey region_key;
    int min_x = 0;
    int max_x = 0;
    int min_y = 0;
    int max_y = 0;
};

struct WorldRegionEnsureRequest {
    std::string world_id;
    uint64_t world_seed = 0;
    std::string generator_version;
    std::string content_version;
    std::string worldgen_profile_key;
    std::string layer_key;
    int region_size = 16;
    WorldRegionEnsureKind ensure_kind = WorldRegionEnsureKind::Unknown;
    WorldRegionCoverageMode coverage_mode = WorldRegionCoverageMode::Unknown;

    std::optional<world_runtime::WorldCellCoord> center_coord;
    std::vector<world_runtime::WorldCellCoord> target_coords;
    std::vector<WorldRegionCoord> explicit_regions;

    int radius_cells = 0;
    int preload_margin_regions = 0;
    int max_regions_to_generate = 0;
    bool allow_generate = true;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldRegionEnsureItemResult {
    WorldRegionKey region_key;
    WorldRegionEnsureStatus status = WorldRegionEnsureStatus::Unknown;
    int generated_cell_count = 0;
    int generated_entity_count = 0;
    int generated_resource_node_count = 0;
    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> warning_keys;
    std::string failure_reason_key;
};

struct WorldRegionEnsureResult {
    bool ok = false;
    std::string request_id;
    WorldRegionEnsureKind ensure_kind = WorldRegionEnsureKind::Unknown;
    WorldRegionCoverageMode coverage_mode = WorldRegionCoverageMode::Unknown;
    std::vector<WorldRegionEnsureItemResult> item_results;
    int generated_region_count = 0;
    int already_available_count = 0;
    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> warning_keys;
    std::vector<std::string> failure_reason_keys;
};

} // namespace pathfinder::world_generation
