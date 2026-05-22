#include "pathfinder/world_region_state/world_region_save_bridge.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_region_state {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

// ---------------------------------------------------------------------------
// WorldRegionSnapshotCollection
// ---------------------------------------------------------------------------

Result<void> WorldRegionSnapshotCollection::validateBasic() const {
    if (world_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_world_id"));
    }
    if (generator_version.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_generator_version"));
    }
    if (content_version.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_content_version"));
    }
    for (const auto& snap : region_snapshots) {
        auto valid = snap.validateBasic();
        if (valid.is_error()) {
            return valid;
        }
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// WorldRegionSaveSectionDraft
// ---------------------------------------------------------------------------

Result<void> WorldRegionSaveSectionDraft::validateBasic() const {
    if (section_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "missing_section_key"));
    }
    return collection.validateBasic();
}

// ---------------------------------------------------------------------------
// WorldRegionSaveBridge
// ---------------------------------------------------------------------------

Result<WorldRegionSaveSectionDraft> WorldRegionSaveBridge::buildSectionDraft(
    const IWorldRegionStateStore& store,
    const std::string& world_id,
    uint64_t world_tick,
    uint64_t state_version) const {

    WorldRegionSaveSectionDraft draft;
    draft.collection.world_id = world_id;
    draft.collection.world_tick = world_tick;
    draft.collection.state_version = state_version;
    draft.collection.generator_version = "1.0.0"; // Would come from runtime config
    draft.collection.content_version = "1.0.0";   // Would come from runtime config

    for (const auto& key : store.keys()) {
        auto snapshot_res = store.find(key);
        if (snapshot_res.is_error()) {
            draft.warning_keys.push_back("cached_key_missing_snapshot:" + key.toString());
            continue;
        }
        draft.collection.region_snapshots.push_back(*snapshot_res.value());
    }

    auto valid = draft.validateBasic();
    if (valid.is_error()) {
        return Result<WorldRegionSaveSectionDraft>::fail(valid.errors());
    }

    return Result<WorldRegionSaveSectionDraft>::ok(std::move(draft));
}

Result<std::vector<world_generation::WorldRegionKey>> WorldRegionSaveBridge::restoreStoreFromSection(
    IWorldRegionStateStore& store,
    const WorldRegionSaveSectionDraft& draft) const {

    auto valid = draft.validateBasic();
    if (valid.is_error()) {
        return Result<std::vector<world_generation::WorldRegionKey>>::fail(valid.errors());
    }

    std::vector<world_generation::WorldRegionKey> restored_keys;
    std::vector<std::string> restore_warnings;
    for (const auto& snap : draft.collection.region_snapshots) {
        auto put_res = store.put(snap);
        if (put_res.is_ok()) {
            restored_keys.push_back(snap.header.region_key);
        } else {
            restore_warnings.push_back("restore_skip:" + snap.header.region_key.toString());
        }
    }
    (void)restore_warnings;

    return Result<std::vector<world_generation::WorldRegionKey>>::ok(std::move(restored_keys));
}

Result<void> WorldRegionSaveBridge::validateCollection(
    const WorldRegionSnapshotCollection& collection) const {
    return collection.validateBasic();
}

} // namespace pathfinder::world_region_state
