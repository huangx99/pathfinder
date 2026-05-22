#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_region_state {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

// ---------------------------------------------------------------------------
// toString helpers
// ---------------------------------------------------------------------------

std::string toString(WorldRegionLifecycleState state) {
    switch (state) {
        case WorldRegionLifecycleState::Unknown: return "Unknown";
        case WorldRegionLifecycleState::NeverGenerated: return "NeverGenerated";
        case WorldRegionLifecycleState::ActiveRuntime: return "ActiveRuntime";
        case WorldRegionLifecycleState::Sealing: return "Sealing";
        case WorldRegionLifecycleState::CachedSnapshot: return "CachedSnapshot";
        case WorldRegionLifecycleState::Restoring: return "Restoring";
        case WorldRegionLifecycleState::RestoreFailed: return "RestoreFailed";
    }
    return "Unknown";
}

std::string toString(WorldRegionStateStoreKind kind) {
    switch (kind) {
        case WorldRegionStateStoreKind::Unknown: return "Unknown";
        case WorldRegionStateStoreKind::InMemory: return "InMemory";
        case WorldRegionStateStoreKind::SavePackageSection: return "SavePackageSection";
        case WorldRegionStateStoreKind::ExternalStorage: return "ExternalStorage";
    }
    return "Unknown";
}

std::string toString(WorldRegionSealStatus status) {
    switch (status) {
        case WorldRegionSealStatus::Sealed: return "Sealed";
        case WorldRegionSealStatus::AlreadyCached: return "AlreadyCached";
        case WorldRegionSealStatus::RegionNotActive: return "RegionNotActive";
        case WorldRegionSealStatus::UnsafeOwnership: return "UnsafeOwnership";
        case WorldRegionSealStatus::ContainsUnsupportedState: return "ContainsUnsupportedState";
        case WorldRegionSealStatus::SnapshotInvalid: return "SnapshotInvalid";
        case WorldRegionSealStatus::StoreFailed: return "StoreFailed";
    }
    return "Unknown";
}

std::string toString(WorldRegionRestoreStatus status) {
    switch (status) {
        case WorldRegionRestoreStatus::Restored: return "Restored";
        case WorldRegionRestoreStatus::AlreadyActive: return "AlreadyActive";
        case WorldRegionRestoreStatus::SnapshotMissing: return "SnapshotMissing";
        case WorldRegionRestoreStatus::SnapshotInvalid: return "SnapshotInvalid";
        case WorldRegionRestoreStatus::VersionIncompatible: return "VersionIncompatible";
        case WorldRegionRestoreStatus::RuntimeConflict: return "RuntimeConflict";
        case WorldRegionRestoreStatus::ApplyFailed: return "ApplyFailed";
    }
    return "Unknown";
}

std::string toString(WorldRegionAvailabilitySource source) {
    switch (source) {
        case WorldRegionAvailabilitySource::ActiveRuntime: return "ActiveRuntime";
        case WorldRegionAvailabilitySource::RestoredSnapshot: return "RestoredSnapshot";
        case WorldRegionAvailabilitySource::GeneratedBaseline: return "GeneratedBaseline";
        case WorldRegionAvailabilitySource::BlockedFailure: return "BlockedFailure";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// validateBasic
// ---------------------------------------------------------------------------

Result<void> WorldRegionSnapshotHeader::validateBasic() const {
    if (snapshot_schema_version != "world_region_snapshot.v1") {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "unsupported_snapshot_schema_version",
                      "Snapshot schema version '" + snapshot_schema_version + "' is not supported. "
                      "Expected 'world_region_snapshot.v1'"));
    }
    if (world_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_world_id", "World ID is required"));
    }
    if (region_key.world_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_region_key_world_id", "Region key world_id is required"));
    }
    if (world_id != region_key.world_id) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "world_id_mismatch",
                      "Header world_id '" + world_id + "' does not match region_key.world_id '" +
                      region_key.world_id + "'"));
    }
    if (region_key.layer_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_layer_key", "Region key layer_key is required"));
    }
    if (region_key.region_size <= 0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "invalid_region_size",
                      "Region size must be > 0, got " + std::to_string(region_key.region_size)));
    }
    if (worldgen_profile_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_worldgen_profile_key", "Worldgen profile key is required"));
    }
    if (generator_version.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_generator_version", "Generator version is required"));
    }
    if (content_version.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_content_version", "Content version is required"));
    }
    return Result<void>::ok();
}

Result<void> WorldRegionSnapshot::validateBasic() const {
    auto header_valid = header.validateBasic();
    if (header_valid.is_error()) {
        return header_valid;
    }

    std::string expected_region_id = header.region_key.regionRuntimeId();
    world_generation::WorldRegionCoord expected_region{header.region_key.rx, header.region_key.ry};

    // Validate cells: all must belong to the region and layer declared by header.
    for (const auto& cell : cells) {
        if (cell.region_id != expected_region_id) {
            return Result<void>::fail(
                makeError(ErrorCode::validation_failed, "cell_region_mismatch",
                          "Cell " + cell.cell_id + " belongs to region " + cell.region_id +
                          " but snapshot is for region " + expected_region_id));
        }
        if (!world_generation::WorldRegionMath::coordBelongsToRegion(
                cell.coord, expected_region, header.region_key.region_size, header.region_key.layer_key)) {
            return Result<void>::fail(
                makeError(ErrorCode::validation_failed, "cell_coord_out_of_region",
                          "Cell " + cell.cell_id + " coord does not belong to snapshot region"));
        }
    }

    // Validate entities: all must be OnMap and must belong to the region/layer.
    for (const auto& entity : on_map_entities) {
        if (entity.location_kind != world_runtime::WorldEntityLocationKind::OnMap) {
            return Result<void>::fail(
                makeError(ErrorCode::validation_failed, "entity_not_on_map",
                          "Entity " + entity.entity_id + " has location_kind " +
                          world_runtime::toString(entity.location_kind) + ", expected OnMap"));
        }
        if (!world_generation::WorldRegionMath::coordBelongsToRegion(
                entity.coord, expected_region, header.region_key.region_size, header.region_key.layer_key)) {
            return Result<void>::fail(
                makeError(ErrorCode::validation_failed, "entity_coord_out_of_region",
                          "Entity " + entity.entity_id + " coord does not belong to snapshot region"));
        }
    }

    // Validate resource nodes
    for (const auto& node : resource_nodes) {
        if (node.node_id.empty()) {
            return Result<void>::fail(
                makeError(ErrorCode::validation_failed, "missing_node_id",
                          "Resource node has empty node_id"));
        }
        if (node.max_charges < 0) {
            return Result<void>::fail(
                makeError(ErrorCode::validation_failed, "invalid_max_charges",
                          "Resource node " + node.node_id + " has max_charges " +
                          std::to_string(node.max_charges) + ", must be >= 0"));
        }
        if (node.remaining_charges < 0 || node.remaining_charges > node.max_charges) {
            return Result<void>::fail(
                makeError(ErrorCode::validation_failed, "invalid_remaining_charges",
                          "Resource node " + node.node_id + " has remaining_charges " +
                          std::to_string(node.remaining_charges) + ", must be in [0, " +
                          std::to_string(node.max_charges) + "]"));
        }
        if (!world_generation::WorldRegionMath::coordBelongsToRegion(
                node.coord, expected_region, header.region_key.region_size, header.region_key.layer_key)) {
            return Result<void>::fail(
                makeError(ErrorCode::validation_failed, "node_coord_out_of_region",
                          "Resource node " + node.node_id + " coord does not belong to snapshot region"));
        }
    }

    return Result<void>::ok();
}

} // namespace pathfinder::world_region_state
