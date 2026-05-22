#include "pathfinder/world_region_state/world_region_lifecycle_service.h"
#include "pathfinder/world_generation/world_region_ensure_types.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_region_state {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_generation::WorldRegionEnsureRequest;
using pathfinder::world_generation::WorldRegionEnsureKind;
using pathfinder::world_generation::WorldRegionCoverageMode;
using pathfinder::world_generation::WorldRegionEnsureStatus;

WorldRegionLifecycleService::WorldRegionLifecycleService(
    IWorldRegionStateStore& store,
    IWorldRegionStateQueryPort& query_port,
    IWorldRegionStateApplyPort& apply_port,
    world_inventory::IWorldEntityLocationPort& location_port,
    world_runtime::IWorldRuntime& world_runtime,
    IWorldRegionGenerationEnsurePort* generation_ensure_port)
    : store_(store)
    , query_port_(query_port)
    , apply_port_(apply_port)
    , location_port_(location_port)
    , world_runtime_(world_runtime)
    , generation_ensure_port_(generation_ensure_port)
    , unload_guard_(world_runtime) {
}

WorldRegionLifecycleService::WorldRegionLifecycleService(
    IWorldRegionStateStore& store,
    IWorldRegionStateQueryPort& query_port,
    IWorldRegionStateApplyPort& apply_port,
    world_inventory::IWorldEntityLocationPort& location_port,
    world_runtime::IWorldRuntime& world_runtime)
    : store_(store)
    , query_port_(query_port)
    , apply_port_(apply_port)
    , location_port_(location_port)
    , world_runtime_(world_runtime)
    , unload_guard_(world_runtime) {
}

std::string WorldRegionLifecycleService::makeLifecycleKey(
    const world_generation::WorldRegionKey& key) const {
    return key.toString();
}

WorldRegionLifecycleState WorldRegionLifecycleService::getLifecycleState(
    const world_generation::WorldRegionKey& region_key) const {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);
    auto it = lifecycle_states_.find(makeLifecycleKey(region_key));
    if (it != lifecycle_states_.end()) {
        return it->second;
    }

    // If runtime has the region active, it's ActiveRuntime
    std::string region_id = region_key.regionRuntimeId();
    if (world_runtime_.isRegionGenerated(region_id)) {
        return WorldRegionLifecycleState::ActiveRuntime;
    }

    // If store has it, it's CachedSnapshot
    if (store_.has(region_key)) {
        return WorldRegionLifecycleState::CachedSnapshot;
    }

    return WorldRegionLifecycleState::NeverGenerated;
}

void WorldRegionLifecycleService::setLifecycleState(
    const world_generation::WorldRegionKey& region_key,
    WorldRegionLifecycleState state) {
    std::lock_guard<std::mutex> lock(lifecycle_mutex_);
    lifecycle_states_[makeLifecycleKey(region_key)] = state;
}

bool WorldRegionLifecycleService::isRegionActive(
    const world_generation::WorldRegionKey& region_key) const {
    std::string region_id = region_key.regionRuntimeId();
    return world_runtime_.isRegionGenerated(region_id);
}

std::vector<world_generation::WorldRegionKey> WorldRegionLifecycleService::cachedRegionKeys() const {
    return store_.keys();
}

Result<WorldRegionSealResult> WorldRegionLifecycleService::sealRegion(
    const world_generation::WorldRegionKey& region_key,
    SealMode mode) {

    WorldRegionSealResult result;
    result.region_key = region_key;
    result.state_version_before = world_runtime_.stateVersion();

    // Check if already cached
    if (store_.has(region_key)) {
        result.status = WorldRegionSealStatus::AlreadyCached;
        return Result<WorldRegionSealResult>::ok(std::move(result));
    }

    // Check lifecycle state
    auto current_state = getLifecycleState(region_key);
    if (current_state != WorldRegionLifecycleState::ActiveRuntime) {
        result.status = WorldRegionSealStatus::RegionNotActive;
        result.failure_reason_keys.push_back("region_not_active:" + toString(current_state));
        return Result<WorldRegionSealResult>::ok(std::move(result));
    }

    // Unload guard check (only for DetachSafeOnly)
    if (mode == SealMode::DetachSafeOnly) {
        auto guard_result = unload_guard_.checkCanUnload(region_key);
        if (!guard_result.can_unload) {
            result.status = WorldRegionSealStatus::UnsafeOwnership;
            result.failure_reason_keys = guard_result.blocking_reason_keys;
            return Result<WorldRegionSealResult>::ok(std::move(result));
        }
    }

    // Set lifecycle to Sealing
    setLifecycleState(region_key, WorldRegionLifecycleState::Sealing);

    // Build snapshot
    WorldRegionSnapshotBuildContext build_context;
    build_context.world_id = region_key.world_id;
    build_context.world_seed = world_runtime_.worldSeed();
    build_context.world_tick = world_runtime_.currentWorldTick();
    build_context.state_version = world_runtime_.stateVersion();
    build_context.generator_version = world_runtime_.generatorVersion();
    build_context.content_version = world_runtime_.contentVersion();
    build_context.worldgen_profile_key = world_runtime_.worldgenProfileKey();

    WorldRegionSnapshotBuilder builder(query_port_);
    auto snap_res = builder.build(region_key, build_context);
    if (snap_res.is_error()) {
        setLifecycleState(region_key, WorldRegionLifecycleState::ActiveRuntime);
        result.status = WorldRegionSealStatus::SnapshotInvalid;
        result.failure_reason_keys.push_back("snapshot_build_failed");
        for (const auto& err : snap_res.errors()) {
            result.failure_reason_keys.push_back(err.message_key);
        }
        return Result<WorldRegionSealResult>::ok(std::move(result));
    }

    auto snapshot = snap_res.value();

    // Store snapshot
    auto store_res = store_.put(snapshot);
    if (store_res.is_error()) {
        setLifecycleState(region_key, WorldRegionLifecycleState::ActiveRuntime);
        result.status = WorldRegionSealStatus::StoreFailed;
        result.failure_reason_keys.push_back("store_put_failed");
        for (const auto& err : store_res.errors()) {
            result.failure_reason_keys.push_back(err.message_key);
        }
        return Result<WorldRegionSealResult>::ok(std::move(result));
    }

    // Mark lifecycle as CachedSnapshot
    setLifecycleState(region_key, WorldRegionLifecycleState::CachedSnapshot);

    // Detach if safe and mode allows. Snapshot is already persisted, so a detach
    // failure is reported as a warning and leaves the cached snapshot authoritative.
    if (mode == SealMode::DetachSafeOnly) {
        auto detach_res = world_runtime_.detachRegion(region_key.regionRuntimeId());
        if (detach_res.is_error()) {
            result.warning_keys.push_back("detach_failed_after_seal");
            for (const auto& err : detach_res.errors()) {
                result.warning_keys.push_back(err.message_key);
            }
        } else {
            for (const auto& removed_id : detach_res.value()) {
                result.warning_keys.push_back("detached:" + removed_id);
            }
        }
    }

    result.status = WorldRegionSealStatus::Sealed;
    result.snapshot_id = snapshot.header.snapshot_id;
    result.state_version_after = world_runtime_.stateVersion();
    result.changed_cell_ids.reserve(snapshot.cells.size());
    for (const auto& cell : snapshot.cells) {
        result.changed_cell_ids.push_back(cell.cell_id);
    }
    for (const auto& entity : snapshot.on_map_entities) {
        result.changed_entity_ids.push_back(entity.entity_id);
    }
    for (const auto& node : snapshot.resource_nodes) {
        result.changed_resource_node_ids.push_back(node.node_id);
    }

    return Result<WorldRegionSealResult>::ok(std::move(result));
}

Result<WorldRegionRestoreResult> WorldRegionLifecycleService::restoreRegion(
    const world_generation::WorldRegionKey& region_key) {

    WorldRegionRestoreResult result;
    result.region_key = region_key;
    result.state_version_before = world_runtime_.stateVersion();

    // Check if already active
    if (isRegionActive(region_key)) {
        result.status = WorldRegionRestoreStatus::AlreadyActive;
        result.availability_source = WorldRegionAvailabilitySource::ActiveRuntime;
        return Result<WorldRegionRestoreResult>::ok(std::move(result));
    }

    // Find snapshot
    auto snap_res = store_.find(region_key);
    if (snap_res.is_error()) {
        result.status = WorldRegionRestoreStatus::SnapshotMissing;
        result.failure_reason_keys.push_back("snapshot_not_found");
        return Result<WorldRegionRestoreResult>::ok(std::move(result));
    }

    const auto* snapshot = snap_res.value();

    // Set lifecycle to Restoring
    setLifecycleState(region_key, WorldRegionLifecycleState::Restoring);

    // Restore
    WorldRegionRestoreContext restore_context;
    restore_context.world_id = region_key.world_id;
    restore_context.world_seed = world_runtime_.worldSeed();
    restore_context.world_tick = world_runtime_.currentWorldTick();
    restore_context.current_state_version = world_runtime_.stateVersion();
    restore_context.generator_version = snapshot->header.generator_version;
    restore_context.content_version = snapshot->header.content_version;

    WorldRegionSnapshotRestorer restorer(apply_port_);
    auto restore_res = restorer.restore(*snapshot, restore_context);

    if (restore_res.is_error() || !restore_res.value().ok()) {
        setLifecycleState(region_key, WorldRegionLifecycleState::RestoreFailed);
        if (restore_res.is_error()) {
            result.status = WorldRegionRestoreStatus::ApplyFailed;
            for (const auto& err : restore_res.errors()) {
                result.failure_reason_keys.push_back(err.message_key);
            }
        } else {
            result = restore_res.value();
        }
        return Result<WorldRegionRestoreResult>::ok(std::move(result));
    }

    result = restore_res.value();
    setLifecycleState(region_key, WorldRegionLifecycleState::ActiveRuntime);
    return Result<WorldRegionRestoreResult>::ok(std::move(result));
}

Result<WorldRegionAvailabilityResult> WorldRegionLifecycleService::ensureAvailable(
    const world_generation::WorldRegionKey& region_key,
    const WorldRegionSnapshotBuildContext& context) {

    WorldRegionAvailabilityResult result;
    result.region_key = region_key;
    result.lifecycle_state = getLifecycleState(region_key);

    // 1. If runtime active -> ActiveRuntime
    if (isRegionActive(region_key)) {
        result.available = true;
        result.source = WorldRegionAvailabilitySource::ActiveRuntime;
        result.lifecycle_state = WorldRegionLifecycleState::ActiveRuntime;
        return Result<WorldRegionAvailabilityResult>::ok(std::move(result));
    }

    // 2. If store has snapshot -> restore -> RestoredSnapshot
    if (store_.has(region_key)) {
        auto restore_res = restoreRegion(region_key);
        if (restore_res.is_ok() && restore_res.value().ok()) {
            result.available = true;
            result.source = WorldRegionAvailabilitySource::RestoredSnapshot;
            result.lifecycle_state = WorldRegionLifecycleState::ActiveRuntime;
            result.changed_cell_ids = restore_res.value().changed_cell_ids;
            result.changed_entity_ids = restore_res.value().changed_entity_ids;
            result.changed_resource_node_ids = restore_res.value().changed_resource_node_ids;
            result.warning_keys = restore_res.value().warning_keys;
            return Result<WorldRegionAvailabilityResult>::ok(std::move(result));
        } else {
            result.available = false;
            result.source = WorldRegionAvailabilitySource::BlockedFailure;
            result.failure_reason_keys.push_back("restore_failed");
            if (restore_res.is_ok()) {
                for (const auto& key : restore_res.value().failure_reason_keys) {
                    result.failure_reason_keys.push_back(key);
                }
            } else {
                for (const auto& err : restore_res.errors()) {
                    result.failure_reason_keys.push_back(err.message_key);
                }
            }
            return Result<WorldRegionAvailabilityResult>::ok(std::move(result));
        }
    }

    // 3. Delegate to P58 generation -> GeneratedBaseline
    if (generation_ensure_port_ != nullptr) {
        WorldRegionEnsureRequest gen_request;
        gen_request.world_id = region_key.world_id;
        gen_request.world_seed = context.world_seed;
        gen_request.generator_version = context.generator_version;
        gen_request.content_version = context.content_version;
        gen_request.worldgen_profile_key = context.worldgen_profile_key;
        gen_request.layer_key = region_key.layer_key;
        gen_request.region_size = region_key.region_size;
        gen_request.ensure_kind = WorldRegionEnsureKind::SystemPrewarm;
        gen_request.coverage_mode = WorldRegionCoverageMode::RegionList;
        gen_request.explicit_regions.push_back(
            world_generation::WorldRegionCoord{region_key.rx, region_key.ry});
        gen_request.allow_generate = true;

        auto ensure_res = generation_ensure_port_->ensureRegions(gen_request);
        if (ensure_res.is_error()) {
            result.available = false;
            result.source = WorldRegionAvailabilitySource::BlockedFailure;
            for (const auto& err : ensure_res.errors()) {
                result.failure_reason_keys.push_back(err.message_key);
            }
            return Result<WorldRegionAvailabilityResult>::ok(std::move(result));
        }

        const auto& ensure_result = ensure_res.value();
        if (ensure_result.ok && ensure_result.generated_region_count > 0) {
            result.available = true;
            result.source = WorldRegionAvailabilitySource::GeneratedBaseline;
            result.lifecycle_state = WorldRegionLifecycleState::ActiveRuntime;
            result.changed_cell_ids = ensure_result.changed_cell_ids;
            result.changed_entity_ids = ensure_result.changed_entity_ids;
        } else if (ensure_result.ok) {
            result.available = true;
            result.source = WorldRegionAvailabilitySource::GeneratedBaseline;
            result.lifecycle_state = WorldRegionLifecycleState::ActiveRuntime;
        } else {
            result.available = false;
            result.source = WorldRegionAvailabilitySource::BlockedFailure;
            for (const auto& key : ensure_result.failure_reason_keys) {
                result.failure_reason_keys.push_back(key);
            }
        }
        return Result<WorldRegionAvailabilityResult>::ok(std::move(result));
    }

    // Fallback: if no generation ensure port is provided, cannot generate.
    result.available = false;
    result.source = WorldRegionAvailabilitySource::BlockedFailure;
    result.failure_reason_keys.push_back("generation_ensure_port_missing");
    return Result<WorldRegionAvailabilityResult>::ok(std::move(result));
}

} // namespace pathfinder::world_region_state
