#include "pathfinder/world_map_interaction/region_lifecycle_trigger_service.h"
#include "pathfinder/world_map_interaction/region_activity_window_service.h"
#include "pathfinder/foundation/error.h"
#include <chrono>

namespace pathfinder::world_map_interaction {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_region_state::SealMode;
using pathfinder::world_generation::WorldRegionKey;

namespace {
    uint64_t makeEventTick() {
        return static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
    }
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

RegionLifecycleTriggerService::RegionLifecycleTriggerService(
    world_region_state::WorldRegionLifecycleService& lifecycle_service)
    : lifecycle_service_(lifecycle_service) {}

void RegionLifecycleTriggerService::setActivityWindowService(
    RegionActivityWindowService* service) {
    activity_window_service_ = service;
}

// ---------------------------------------------------------------------------
// processWindow
// ---------------------------------------------------------------------------

RegionLifecycleTriggerService::TriggerResult RegionLifecycleTriggerService::processWindow(
    const RegionActivityWindow& window,
    const world_region_state::WorldRegionSnapshotBuildContext& context) {

    TriggerResult result;

    // 1. Ensure prewarm regions are available (non-blocking best effort)
    for (const auto& region : window.prewarm_regions) {
        auto avail_res = lifecycle_service_.ensureAvailable(region, context);
        if (avail_res.is_error()) {
            result.warning_keys.push_back("prewarm_failed:" + region.regionRuntimeId());
        }
    }

    // 2. Seal candidates
    for (const auto& region : window.seal_candidate_regions) {
        auto seal_res = lifecycle_service_.sealRegion(region, SealMode::DetachSafeOnly);
        if (seal_res.is_error()) {
            std::string err_msg = "lifecycle_seal_error";
            if (!seal_res.errors().empty()) {
                err_msg = seal_res.errors()[0].message_key;
            }
            result.events.push_back(makeEvent(
                "seal_failed", region,
                {err_msg},
                {}));
            result.failure_reason_keys.push_back("seal_failed:" + region.regionRuntimeId());
        } else {
            const auto& seal = seal_res.value();
            if (seal.ok()) {
                result.events.push_back(makeEvent("sealed", region));
                // P60: Notify activity window that this region has been sealed.
                if (activity_window_service_) {
                    activity_window_service_->markSealed(region, window.computed_tick);
                }
            } else {
                result.events.push_back(makeEvent(
                    "seal_blocked", region,
                    seal.failure_reason_keys, seal.warning_keys));
                result.warning_keys.insert(result.warning_keys.end(),
                    seal.warning_keys.begin(), seal.warning_keys.end());
            }
        }
    }

    // 3. Detach candidates (only if they are already sealed/cached)
    for (const auto& region : window.detach_candidate_regions) {
        auto state = lifecycle_service_.getLifecycleState(region);
        if (state == pathfinder::world_region_state::WorldRegionLifecycleState::CachedSnapshot) {
            // Already sealed and detached (or ready to detach)
            result.events.push_back(makeEvent("detached", region));
        } else if (state == pathfinder::world_region_state::WorldRegionLifecycleState::ActiveRuntime) {
            // Should have been sealed first; attempt seal then mark
            auto seal_res = lifecycle_service_.sealRegion(region, SealMode::DetachSafeOnly);
            if (seal_res.is_ok() && seal_res.value().ok()) {
                result.events.push_back(makeEvent("sealed_then_detached", region));
            } else {
                result.events.push_back(makeEvent(
                    "detach_blocked", region,
                    {"region_still_active"}));
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// ensureRegionAvailable
// ---------------------------------------------------------------------------

Result<bool> RegionLifecycleTriggerService::ensureRegionAvailable(
    const WorldRegionKey& region_key,
    const world_region_state::WorldRegionSnapshotBuildContext& context) {

    auto avail_res = lifecycle_service_.ensureAvailable(region_key, context);
    if (avail_res.is_error()) {
        std::string err_msg = "ensure_available_failed";
        if (!avail_res.errors().empty()) {
            err_msg = avail_res.errors()[0].message_key;
        }
        return Result<bool>::fail(makeError(ErrorCode::common_internal_invariant_broken,
            "ensureAvailable failed for region " + region_key.regionRuntimeId()
            + ": " + err_msg));
    }

    const auto& result = avail_res.value();
    if (!result.available) {
        // P60 红线: restore failed 不能偷偷 generate
        return Result<bool>::ok(false);
    }

    return Result<bool>::ok(true);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

ClientRegionLifecycleEventDto RegionLifecycleTriggerService::makeEvent(
    const std::string& event_kind,
    const WorldRegionKey& region_key,
    const std::vector<std::string>& failure_reason_keys,
    const std::vector<std::string>& warning_keys) {

    ClientRegionLifecycleEventDto event;
    event.event_id = "lifecycle_" + event_kind + "_" + region_key.regionRuntimeId() + "_" + std::to_string(makeEventTick());
    event.tick = makeEventTick();
    event.event_kind = event_kind;
    event.region_runtime_id = region_key.regionRuntimeId();
    event.label_zh = regionLabel(region_key) + " - " + event_kind;
    event.failure_reason_keys = failure_reason_keys;
    event.warning_keys = warning_keys;
    return event;
}

std::string RegionLifecycleTriggerService::regionLabel(const WorldRegionKey& key) const {
    return "区域(" + std::to_string(key.rx) + "," + std::to_string(key.ry) + ")";
}

} // namespace pathfinder::world_map_interaction
