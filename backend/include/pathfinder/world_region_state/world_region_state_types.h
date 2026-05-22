#pragma once

#include "pathfinder/world_generation/world_region_ensure_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/foundation/result.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace pathfinder::world_region_state {

// ---------------------------------------------------------------------------
// 9.1 WorldRegionLifecycleState
// ---------------------------------------------------------------------------
enum class WorldRegionLifecycleState {
    Unknown,
    NeverGenerated,
    ActiveRuntime,
    Sealing,
    CachedSnapshot,
    Restoring,
    RestoreFailed
};

std::string toString(WorldRegionLifecycleState state);

// ---------------------------------------------------------------------------
// 9.2 WorldRegionStateStoreKind
// ---------------------------------------------------------------------------
enum class WorldRegionStateStoreKind {
    Unknown,
    InMemory,
    SavePackageSection,
    ExternalStorage
};

std::string toString(WorldRegionStateStoreKind kind);

// ---------------------------------------------------------------------------
// 9.3 WorldRegionSealStatus
// ---------------------------------------------------------------------------
enum class WorldRegionSealStatus {
    Sealed,
    AlreadyCached,
    RegionNotActive,
    UnsafeOwnership,
    ContainsUnsupportedState,
    SnapshotInvalid,
    StoreFailed
};

std::string toString(WorldRegionSealStatus status);

// ---------------------------------------------------------------------------
// 9.4 WorldRegionRestoreStatus
// ---------------------------------------------------------------------------
enum class WorldRegionRestoreStatus {
    Restored,
    AlreadyActive,
    SnapshotMissing,
    SnapshotInvalid,
    VersionIncompatible,
    RuntimeConflict,
    ApplyFailed
};

std::string toString(WorldRegionRestoreStatus status);

// ---------------------------------------------------------------------------
// 9.5 WorldRegionAvailabilitySource
// ---------------------------------------------------------------------------
enum class WorldRegionAvailabilitySource {
    ActiveRuntime,
    RestoredSnapshot,
    GeneratedBaseline,
    BlockedFailure
};

std::string toString(WorldRegionAvailabilitySource source);

// ---------------------------------------------------------------------------
// 10. Core DTO / Data Structures
// ---------------------------------------------------------------------------

struct WorldRegionSnapshotHeader {
    std::string snapshot_id;
    std::string snapshot_schema_version; // world_region_snapshot.v1
    std::string world_id;
    world_generation::WorldRegionKey region_key;
    uint64_t world_seed = 0;
    uint64_t world_tick = 0;
    uint64_t state_version = 0;
    std::string worldgen_profile_key;
    std::string generator_version;
    std::string content_version;
    std::string baseline_region_fingerprint;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldRegionCellStateRecord {
    std::string cell_id;
    world_runtime::WorldCellCoord coord;
    std::string terrain_key;
    std::string region_id;
    bool generated = false;
    bool blocks_movement = false;
    int movement_cost = 1;
    std::vector<std::string> entity_ids;
    std::vector<std::string> tag_keys;
};

struct WorldRegionEntityStateRecord {
    std::string entity_id;
    std::string entity_key;
    std::string display_name_key;
    world_runtime::WorldEntityLocationKind location_kind = world_runtime::WorldEntityLocationKind::Nowhere;
    world_runtime::WorldCellCoord coord;
    std::string owner_ref;
    std::string container_ref;
    bool blocks_movement = false;
    bool visible_by_default = true;
    std::vector<std::string> tag_keys;
    int quantity = 1;
    bool stackable = true;
    std::string stack_key;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
};

struct WorldRegionResourceNodeStateRecord {
    std::string node_id;
    std::string resource_key;
    world_runtime::WorldCellCoord coord;
    std::string node_kind_str;
    std::string node_state_str;
    int remaining_charges = 1;
    int max_charges = 1;
    std::string required_action_key;
    std::string required_tool_key;
    std::vector<std::string> output_entity_keys;
    std::vector<std::string> tag_keys;
};

struct WorldRegionExplorationStateSlice {
    std::string actor_key;
    std::map<std::string, world_runtime::WorldCellVisibility> cell_visibility_by_id;
};

struct WorldRegionSnapshot {
    WorldRegionSnapshotHeader header;
    std::vector<WorldRegionCellStateRecord> cells;
    std::vector<WorldRegionEntityStateRecord> on_map_entities;
    std::vector<WorldRegionResourceNodeStateRecord> resource_nodes;
    std::vector<WorldRegionExplorationStateSlice> exploration_slices;
    std::vector<std::string> unsupported_state_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 10.7 Seal / Restore Result
// ---------------------------------------------------------------------------

enum class SealMode {
    DebugOnly,
    DetachSafeOnly
};

struct WorldRegionSealResult {
    world_generation::WorldRegionKey region_key;
    WorldRegionSealStatus status = WorldRegionSealStatus::Sealed;
    std::string snapshot_id;
    uint64_t state_version_before = 0;
    uint64_t state_version_after = 0;
    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> changed_resource_node_ids;
    std::vector<std::string> failure_reason_keys;
    std::vector<std::string> warning_keys;

    bool ok() const {
        return status == WorldRegionSealStatus::Sealed || status == WorldRegionSealStatus::AlreadyCached;
    }
};

struct WorldRegionRestoreResult {
    world_generation::WorldRegionKey region_key;
    WorldRegionRestoreStatus status = WorldRegionRestoreStatus::Restored;
    std::string snapshot_id;
    uint64_t state_version_before = 0;
    uint64_t state_version_after = 0;
    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> changed_resource_node_ids;
    std::vector<std::string> failure_reason_keys;
    std::vector<std::string> warning_keys;
    WorldRegionAvailabilitySource availability_source = WorldRegionAvailabilitySource::RestoredSnapshot;

    bool ok() const {
        return status == WorldRegionRestoreStatus::Restored || status == WorldRegionRestoreStatus::AlreadyActive;
    }
};

struct WorldRegionAvailabilityResult {
    world_generation::WorldRegionKey region_key;
    bool available = false;
    WorldRegionAvailabilitySource source = WorldRegionAvailabilitySource::BlockedFailure;
    WorldRegionLifecycleState lifecycle_state = WorldRegionLifecycleState::Unknown;
    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> changed_resource_node_ids;
    std::vector<std::string> failure_reason_keys;
    std::vector<std::string> warning_keys;
};

// ---------------------------------------------------------------------------
// Context structs for builder/restorer
// ---------------------------------------------------------------------------

struct WorldRegionSnapshotBuildContext {
    std::string world_id;
    uint64_t world_seed = 0;
    uint64_t world_tick = 0;
    uint64_t state_version = 0;
    std::string worldgen_profile_key;
    std::string generator_version;
    std::string content_version;
};

struct WorldRegionRestoreContext {
    std::string world_id;
    uint64_t world_seed = 0;
    uint64_t world_tick = 0;
    uint64_t current_state_version = 0;
    std::string generator_version;
    std::string content_version;
};

struct WorldRegionRuntimeSlice {
    std::vector<WorldRegionCellStateRecord> cells;
    std::vector<WorldRegionEntityStateRecord> on_map_entities;
    std::vector<WorldRegionResourceNodeStateRecord> resource_nodes;
    std::vector<WorldRegionExplorationStateSlice> exploration_slices;
};

struct WorldRegionApplyPlan {
    world_generation::WorldRegionKey region_key;
    std::vector<WorldRegionCellStateRecord> cells_to_apply;
    std::vector<WorldRegionEntityStateRecord> entities_to_apply;
    std::vector<WorldRegionResourceNodeStateRecord> resource_nodes_to_apply;
    std::vector<WorldRegionExplorationStateSlice> exploration_slices_to_apply;
    std::vector<std::string> entity_ids_to_remove; // conflict resolution
    std::vector<std::string> resource_node_ids_to_remove;
    std::vector<std::string> warning_keys;
};

} // namespace pathfinder::world_region_state
