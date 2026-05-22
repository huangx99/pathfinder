#include "pathfinder/world_region_state/world_region_snapshot_restorer.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_region_state {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

WorldRegionSnapshotRestorer::WorldRegionSnapshotRestorer(IWorldRegionStateApplyPort& apply_port)
    : apply_port_(apply_port) {
}

Result<WorldRegionRestoreResult> WorldRegionSnapshotRestorer::restore(
    const WorldRegionSnapshot& snapshot,
    const WorldRegionRestoreContext& context) {

    WorldRegionRestoreResult result;
    result.region_key = snapshot.header.region_key;
    result.snapshot_id = snapshot.header.snapshot_id;
    result.state_version_before = context.current_state_version;

    // Validate snapshot
    auto valid = snapshot.validateBasic();
    if (valid.is_error()) {
        result.status = WorldRegionRestoreStatus::SnapshotInvalid;
        result.failure_reason_keys.push_back("snapshot_validation_failed");
        for (const auto& err : valid.errors()) {
            result.failure_reason_keys.push_back(err.message_key);
        }
        return Result<WorldRegionRestoreResult>::ok(std::move(result));
    }

    // Version compatibility check
    if (!snapshot.header.generator_version.empty() &&
        snapshot.header.generator_version != context.generator_version) {
        result.status = WorldRegionRestoreStatus::VersionIncompatible;
        result.failure_reason_keys.push_back(
            "generator_version_mismatch:" + snapshot.header.generator_version + "!=" + context.generator_version);
        return Result<WorldRegionRestoreResult>::ok(std::move(result));
    }

    if (!snapshot.header.content_version.empty() &&
        snapshot.header.content_version != context.content_version) {
        result.status = WorldRegionRestoreStatus::VersionIncompatible;
        result.failure_reason_keys.push_back(
            "content_version_mismatch:" + snapshot.header.content_version + "!=" + context.content_version);
        return Result<WorldRegionRestoreResult>::ok(std::move(result));
    }

    // Plan restore
    auto plan_res = apply_port_.planRestore(snapshot);
    if (plan_res.is_error()) {
        result.status = WorldRegionRestoreStatus::RuntimeConflict;
        result.failure_reason_keys.push_back("plan_restore_failed");
        for (const auto& err : plan_res.errors()) {
            result.failure_reason_keys.push_back(err.message_key);
        }
        return Result<WorldRegionRestoreResult>::ok(std::move(result));
    }

    // Apply restore
    auto apply_res = apply_port_.applyRestore(plan_res.value());
    if (apply_res.is_error()) {
        result.status = WorldRegionRestoreStatus::ApplyFailed;
        result.failure_reason_keys.push_back("apply_restore_failed");
        for (const auto& err : apply_res.errors()) {
            result.failure_reason_keys.push_back(err.message_key);
        }
        return Result<WorldRegionRestoreResult>::ok(std::move(result));
    }

    result = apply_res.value();
    // Do not override apply-level failures (RuntimeConflict / ApplyFailed) as Restored
    if (result.ok()) {
        result.status = WorldRegionRestoreStatus::Restored;
        result.availability_source = WorldRegionAvailabilitySource::RestoredSnapshot;
    }
    return Result<WorldRegionRestoreResult>::ok(std::move(result));
}

} // namespace pathfinder::world_region_state
