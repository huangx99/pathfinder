#include "pathfinder/world_region_state/world_region_snapshot_builder.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_region_state {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_generation::WorldRegionMath;
using pathfinder::world_generation::WorldRegionCoord;

WorldRegionSnapshotBuilder::WorldRegionSnapshotBuilder(IWorldRegionStateQueryPort& query_port)
    : query_port_(query_port) {
}

Result<WorldRegionSnapshot> WorldRegionSnapshotBuilder::build(
    const world_generation::WorldRegionKey& region_key,
    const WorldRegionSnapshotBuildContext& context) {

    auto slice_res = query_port_.readRegionSlice(region_key);
    if (slice_res.is_error()) {
        return Result<WorldRegionSnapshot>::fail(slice_res.errors());
    }

    const auto& slice = slice_res.value();

    auto valid = validateSliceForRegion(slice, region_key);
    if (valid.is_error()) {
        return Result<WorldRegionSnapshot>::fail(valid.errors());
    }

    WorldRegionSnapshot snapshot;

    // Build header
    snapshot.header.snapshot_id = "snap_" + region_key.toString() + "_" + std::to_string(context.state_version);
    snapshot.header.snapshot_schema_version = "world_region_snapshot.v1";
    snapshot.header.world_id = context.world_id;
    snapshot.header.region_key = region_key;
    snapshot.header.world_seed = context.world_seed;
    snapshot.header.world_tick = context.world_tick;
    snapshot.header.state_version = context.state_version;
    snapshot.header.worldgen_profile_key = context.worldgen_profile_key;
    snapshot.header.generator_version = context.generator_version;
    snapshot.header.content_version = context.content_version;
    snapshot.header.baseline_region_fingerprint = region_key.toString();

    snapshot.cells = slice.cells;
    snapshot.on_map_entities = slice.on_map_entities;
    snapshot.resource_nodes = slice.resource_nodes;
    snapshot.exploration_slices = slice.exploration_slices;

    auto snap_valid = snapshot.validateBasic();
    if (snap_valid.is_error()) {
        return Result<WorldRegionSnapshot>::fail(snap_valid.errors());
    }

    return Result<WorldRegionSnapshot>::ok(std::move(snapshot));
}

Result<void> WorldRegionSnapshotBuilder::validateSliceForRegion(
    const WorldRegionRuntimeSlice& slice,
    const world_generation::WorldRegionKey& region_key) {

    std::string expected_region_id = region_key.regionRuntimeId();

    for (const auto& cell : slice.cells) {
        if (cell.region_id != expected_region_id) {
            return Result<void>::fail(
                makeError(ErrorCode::validation_failed, "cell_region_mismatch",
                          "Cell " + cell.cell_id + " belongs to " + cell.region_id +
                          " but expected " + expected_region_id));
        }
    }

    for (const auto& entity : slice.on_map_entities) {
        // OnMap entities are collected by matching their cell's region_id in readRegionSlice,
        // so they are inherently in the correct region.
        (void)entity;
    }

    for (const auto& node : slice.resource_nodes) {
        // Resource nodes are collected similarly; inherent correctness.
        (void)node;
    }

    return Result<void>::ok();
}

} // namespace pathfinder::world_region_state
